// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include <malloc.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  my_pthread_t thread_id;
  ucontext_t thread_context;
  int active;
  int executed;
  int exited;
  int isMain;
  void *ret;
}thread;

static bool first_create = true;
static int threadid;
ucontext_t common_context;
struct sigaction scheduler_interrupt_handler;
struct itimerval timeslice;
sigset_t signalMask;
void scheduler();
void *helper(void *(*function)(void*), void *arg);
thread_Queue queue = NULL;
finished_Queue finishedQueue = NULL;
tcb_ptr getCurrentControlBlock_Safe();
long millisec;

/*
Main Queue Implementation
*/
void max_heapify(tcb_ptr data, int loc, int count) {
	int left, right, largest;
    tcb temp;
	left = 2*(loc) + 1;
	right = left + 1;
	largest = loc;


	if ( left <= count && data[left].priority > data[largest].priority ) {
		largest = left;
	}
	if ( right <= count && data[right].priority > data[largest].priority ) {
		largest = right;
	}

    // Timestamps
    clock_t now = clock();
    clock_t parent_age = (now - data[largest].start_time) / CLOCKS_PER_SEC;

    // If priority of parent == priority of left child
    if ( left <= count && data[left].priority == data[largest].priority ) {
		// timestamp will decide the largest
        clock_t left_age = (now - data[left].start_time) / CLOCKS_PER_SEC;

        if( left_age < parent_age ){
            largest = left;
        }
	}

    // If priority of parent == priority of right child
    if ( right <= count && data[right].priority == data[largest].priority ) {
		// timestamp will decide the largest
        clock_t right_age = (now - data[right].start_time) / CLOCKS_PER_SEC;

        if( right_age < parent_age ){
            largest = right;
        }
    }

	if(largest != loc) {
		temp = data[loc];
		data[loc] = data[largest];
		data[largest] = temp;
		max_heapify(data, largest, count);
	}
}

int enqueue(thread_Queue queue, tcb_ptr value) {
    int index, parent;
    index = queue->count; // First insert at last of array

    // Find out where to put the element and put it
    while(index > 0){
        parent = (index - 1) / 2;
        if( queue->heaparr[parent]->priority > value->priority )
                break;
        if( queue->heaparr[parent]->priority == value->priority ){
            // If priorities are same, check for timestamp
            clock_t now = clock();
            clock_t value_age = (now - value->start_time) / CLOCKS_PER_SEC;
            clock_t parent_age = (now - queue->heaparr[parent]->start_time) / CLOCKS_PER_SEC;
            //printf("value_age:%Lf\t parent_age:%Lf\n", (long double)value_age, (long double)parent_age );
            // Newer thread at top
            if( parent_age < value_age ){
                // Insert value at this index
                break;
            }
            // else keep moving value up the heap
        }
        queue->heaparr[index] = queue->heaparr[parent];
        index = parent;
    }

    printf("Pushing:%d\n", value->thread_id);
    queue->heaparr[index] = value;
    queue->count++;

    int i;
    for(i=0; i < queue->count-1; i++){
        queue->heaparr[i]->next = queue->heaparr[i+1];
    }
    queue->heaparr[i]->next = NULL;

  return 0;
}

tcb_ptr dequeue(thread_Queue queue) {

  if(queue == NULL)
    return NULL;
  else {
    printf("\ndequeing blocks");
    tcb_ptr removed;
    queue->count--;
    tcb_ptr last = queue->heaparr[queue->count];

    // Last becomes top of heap; then max_heapify
    removed = queue->heaparr[0];
    queue->heaparr[0] = last;
    max_heapify(queue->heaparr[0], 0, queue->count);

    return removed;
  }
}

int next(thread_Queue queue){
  if(queue!= NULL) {
      // next function takes the heap into the next state,
      // where the top is popped and next thread is queued
      tcb_ptr popped = dequeue(queue);

      // Here, the popped thread has not completed execution,
      // hence, add it to the back of the queue
      queue->heaparr[queue->count] = popped;

      // Reduce priority of popped thread before max_heapify
      // so that it is not scheduled next
      queue->heaparr[queue->count]->priority = 0;
      max_heapify(queue->heaparr[0], 0, queue->count);
  }
  printf("\n Returning from next");
  return 0;
}

int getQueueSize(thread_Queue queue) {
  return queue->count;
}

thread_Queue getQueue() {
  thread_Queue queue = (thread_Queue)malloc(sizeof(struct threadQueue)*MAXTHREADS);
  queue->count = 0;
  queue->size = MAXTHREADS;
  return queue;
}

void heap_delete(thread_Queue h){
    free(h);
}
/////////////////////////////////////

/*
Auxiliary Queue Implementation
*/
int enqueueToCompletedList(finished_Queue queue,finishedThread_ptr finishedThread ) {
  if(queue != NULL && finishedThread !=NULL) {
    finishedThread->next=queue->thread;
    queue->thread = finishedThread;
  }
  return 0;
}

finishedThread_ptr getFinishedThread(finished_Queue queue,my_pthread_t thread_id,int flag) {

  if(queue!=NULL) {
    finishedThread_ptr thread= queue->thread;
    finishedThread_ptr previous_thread = NULL;
    while((thread!=NULL)&& (thread->thread_id!=thread_id)) {
      previous_thread =thread;
      thread = thread ->next;
    }
    if(flag && thread!=NULL) {
      if(previous_thread == NULL)
	     queue->thread  = thread->next;
      else
	     previous_thread->next = thread->next;
    }
    return thread;
  }

  return NULL;
}

blockedThreadList_ptr getBlockedThreadList() {

  blockedThreadList_ptr newList = (blockedThreadList_ptr)malloc(sizeof(struct blockedThreadList));
  if(newList!=NULL) {
    newList->thread=NULL;
    newList->next=NULL;
  }
  return newList;
}

int addToBlockedThreadList(tcb_ptr fromNode,tcb_ptr toNode ) {

  blockedThreadList_ptr list = getBlockedThreadList();
  if(fromNode != NULL) {
    list->thread = toNode;
    list->next = fromNode->blockedThreads;
    fromNode->blockedThreads = list;
    toNode->isBlocked=1;
  }
  return 0;
}

finishedThread_ptr getCompletedThread() {
  finishedThread_ptr finishedThread = (finishedThread_ptr)malloc(sizeof(struct finishedThread));
  if(finishedThread == NULL) {
    return NULL;
  }
  finishedThread->returnValue=(void**)malloc(sizeof(void*));
  if(finishedThread->returnValue ==NULL) {
    free(finishedThread);
    return NULL;
  }
  finishedThread->thread_id= -1;
  *(finishedThread->returnValue)= NULL;
  finishedThread->next =NULL;

  return finishedThread;
}

finished_Queue getFinishedQueue() {
  finished_Queue finishedQueue = (finished_Queue)malloc(sizeof(struct finishedControlBlockQueue));
  finishedQueue->thread = NULL;
  finishedQueue->count = 0;

  return finishedQueue;
}

void threadCompleted() {

  tcb_ptr currentNode = getCurrentControlBlock_Safe();
  blockedThreadList_ptr blockedThread = currentNode->blockedThreads;

  while(blockedThread != NULL){
    blockedThread->thread->isBlocked = 0;
    blockedThread = blockedThread->next;
  }

  printf("\n Thread completed : %d\n",currentNode->thread_id );
  currentNode->isExecuted=1;

  // Add thread to finishedQueue
  //enqueueToCompletedList()

  //removeFromHeap(currentNode->thread_id);
  raise(SIGVTALRM);
}

////////////////////////////////////

/*
Scheduler
*/
void scheduler(int signum){
    sigprocmask(SIG_BLOCK,&signalMask,NULL);
    int q_size = getQueueSize(queue);
    bool to_be_removed = 0;

    if(q_size == 1){
        if( getCurrentBlock(queue)->isExecuted ){
            // If current context has finished execution, dequeue
            dequeue(queue);
        }
    }
    else if(q_size > 1){
            tcb_ptr curr_context = getCurrentBlock(queue);
            if( curr_context != NULL ){
                if( curr_context->isExecuted ){
                    to_be_removed = 1;
                    // dequeue
                    dequeue(queue);
                }
                else{
                    next(queue);
                }

                tcb_ptr next_context = getCurrentBlock(queue);
                while( next_context != NULL && ( next_context->isBlocked || next_context->isExecuted ) ){
                    if( next_context->isExecuted ){
                        // dequeue
                        dequeue(queue);
                    }
                    else{
                        next(queue);
                    }
                    next_context = getCurrentBlock(queue);
                }

                if( next_context == NULL ){
                    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
                    return;
                }
                if( next_context != curr_context ){
                    if( to_be_removed ){
                        // Set next thread as active, discard current thread
                        setcontext( &(next_context->thread_context) );
                    }
                    else{
                        // Swap current thread with next thread
                        swapcontext(&(curr_context->thread_context), &(next_context->thread_context) );
                    }
                }
            }
        }
        sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
}

/////////////////////////////

/*
Context Functions
*/
void *helper(void *(*function)(void*), void *arg){

  void *returnValue;
  tcb_ptr currentThread = getCurrentControlBlock_Safe();
  printf("In Helper");
  returnValue = (*function)(arg);
  sigprocmask(SIG_BLOCK,&signalMask,NULL);
  finishedThread_ptr finishedThread = getCompletedThread();
  if(finishedThread != NULL) {
    *(finishedThread->returnValue) = returnValue;
    finishedThread->thread_id = currentThread->thread_id;
    enqueueToCompletedList(finishedQueue,finishedThread);
  }
  sigprocmask(SIG_UNBLOCK,&signalMask,NULL);

  return returnValue;
  // set this value to the completed nodes return value
}

tcb_ptr getCurrentControlBlock_Safe() {

  tcb_ptr currentControlBlock = NULL;
  sigprocmask(SIG_BLOCK,&signalMask,NULL);
  currentControlBlock = getCurrentBlock(queue);
  sigprocmask(SIG_UNBLOCK,&signalMask,NULL);

  return currentControlBlock;

}

tcb_ptr getControlBlock_Main(){
  tcb_ptr controlBlock = (tcb_ptr)malloc(sizeof(tcb));
  controlBlock->thread_context.uc_stack.ss_flags = 0;
  controlBlock->thread_context.uc_link =0;
  controlBlock->isActive =0;
  controlBlock->isBlocked =0;
  controlBlock->isExecuted =0;
  controlBlock->isMain = 1 ;
  controlBlock->priority = 4;
  controlBlock->next = NULL ;

  return controlBlock;

}

tcb_ptr getControlBlock(){
  tcb_ptr controlBlock = (tcb_ptr)malloc(sizeof(tcb_ptr));
  controlBlock->thread_context.uc_stack.ss_sp = malloc(STACKSIZE);
  controlBlock->thread_context.uc_stack.ss_size = STACKSIZE;
  controlBlock->thread_context.uc_stack.ss_flags = 0;
  controlBlock->thread_context.uc_link =0;
  controlBlock->isActive =0;
  controlBlock->isBlocked =0;
  controlBlock->isExecuted =0;
  controlBlock->isMain =0 ;
  controlBlock->priority = 4;
  controlBlock->next = NULL ;

  return controlBlock;

}

tcb_ptr getCurrentBlock(thread_Queue queue){
  if(queue != NULL && getQueueSize(queue) != 0) {
    printf("\n Returning CurrentBlock\n");
    return queue->heaparr[0];
  }
  printf("Returning Null from current block\n");
  return NULL;
}

tcb_ptr getCurrentBlockByThread(thread_Queue queue,my_pthread_t threadid) {
  tcb_ptr headBlock = getCurrentBlock(queue);
  //if this is the required node
  if(headBlock!=NULL && headBlock->thread_id == threadid)
    return headBlock;
  tcb_ptr dummyThread=NULL;
  if(headBlock!=NULL)
    dummyThread = headBlock->next;

  while((headBlock != dummyThread)) {
    if(dummyThread ->thread_id == threadid)
      return dummyThread;

    dummyThread = dummyThread->next;
  }
  return NULL;
}

void freeControlBlock(tcb_ptr controlBlock) {
  if(!(controlBlock->isMain))
    free(controlBlock->thread_context.uc_stack.ss_sp);

  free(controlBlock);
}

ucontext_t getCommonContext() {
  static int contextAlreadySet = 0;
  if(!contextAlreadySet)
  {
    getcontext(&common_context);
    common_context.uc_link = 0;
    common_context.uc_stack.ss_sp = malloc(STACKSIZE);
    common_context.uc_stack.ss_size = STACKSIZE;
    common_context.uc_stack.ss_flags= 0;
    makecontext( &common_context, (void (*) (void))&threadCompleted, 0);
    contextAlreadySet = 1;
  }
}

//////////////////////////////////////////////////

/*
Pthread API
*/
void my_pthread_init(long period){
  threadid = 1;
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGVTALRM);
  //intializing the context of the scheduler
  finishedQueue = getFinishedQueue();
  queue = getQueue();
  millisec = period;
  tcb_ptr mainThread = getControlBlock_Main();
  //getcontext(&(MainThread->thread_context));
  printf("in init \n");
  getCommonContext();
  mainThread->thread_context.uc_link = &common_context;
  mainThread->thread_id = threadid;
  enqueue(queue,mainThread);
  memset(&scheduler_interrupt_handler, 0, sizeof (scheduler_interrupt_handler));
  scheduler_interrupt_handler.sa_handler= &scheduler;
  sigaction(SIGVTALRM,&scheduler_interrupt_handler,NULL);
  millisec = period;
  timeslice.it_value.tv_sec = 0;
  timeslice.it_interval.tv_sec = 0;
  timeslice.it_value.tv_usec = millisec; // timer start decrementing from here to 0
  timeslice.it_interval.tv_usec = millisec; //timer after 0 resets to this value
  setitimer(ITIMER_VIRTUAL, &timeslice, NULL);
  printf("Exiting init");
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
  //my_pthread_init(1000L);
  if(first_create){
    first_create = false;
    my_pthread_init(25000);
    }
  int temp;
  if(queue != NULL) {
    sigprocmask(SIG_BLOCK,&signalMask,NULL);
    tcb_ptr  threadCB= getControlBlock_Main();
    getcontext((&threadCB->thread_context));
    threadCB->thread_context.uc_stack.ss_sp=malloc(STACKSIZE);
    threadCB->thread_context.uc_stack.ss_size=STACKSIZE;
    threadCB->thread_context.uc_stack.ss_flags=0;
    threadCB->isMain=0;
    threadCB->thread_context.uc_link = &common_context;
    //temp =rand();
    threadCB->thread_id= ++threadid;
    *thread = threadCB->thread_id;

    makecontext(&(threadCB->thread_context),(void (*)(void))&helper,2,function,arg);

    printf("Thread is created %d\n", *thread);
    enqueue(queue,threadCB);
    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
    sigemptyset(&(threadCB->thread_context.uc_sigmask));
    return 0;
  }
  printf("Error: init() function not executed/n");
  return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
  printf("\n-----Yield called-----\n");
  raise(SIGVTALRM);
  return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
  printf("\n-----Exit called-----\n");
  sigprocmask(SIG_BLOCK,&signalMask,NULL);
  tcb_ptr currentThread= getCurrentBlock(queue);
  finishedThread_ptr finishedThread = getCompletedThread();
  if(finishedThread !=NULL && currentThread != NULL) {
    *(finishedThread->returnValue) = value_ptr;
    finishedThread->thread_id = currentThread->thread_id;
    enqueueToCompletedList(finishedQueue,finishedThread);
  }
  threadCompleted();
  sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
  raise(SIGVTALRM);
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
  sigprocmask(SIG_BLOCK,&signalMask,NULL);
  tcb_ptr callingThread = getCurrentBlock(queue);
  tcb_ptr joinThread = getCurrentBlockByThread(queue,thread);

  //check if callingthread is blocking on itself or is null
  if(callingThread == NULL || callingThread == joinThread) {
    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
    return -1;
  }
  if(joinThread == NULL) {
    //The thread is finished hence can be found in finished Queue
    finishedThread_ptr finishedThread = getFinishedThread(finishedQueue,thread,1);
    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
    if(finishedThread) {
      if(value_ptr)
	*value_ptr =*(finishedThread->returnValue);
      free(finishedThread);
      return 0;
    }
    else
      return -1;

  }

  //printf("\n Value is %d :",(joinThread->blockedThreads==NULL));
  if(joinThread->blockedThreads == NULL) {
    addToBlockedThreadList(joinThread,callingThread);
    int isBlocked=callingThread->isBlocked;
    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
    while(isBlocked){
      isBlocked=callingThread->isBlocked;
    }
    sigprocmask(SIG_BLOCK,&signalMask,NULL);
    finishedThread_ptr finishedThread = getFinishedThread(finishedQueue,thread,1);
    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
    if(finishedThread != NULL && value_ptr != NULL) {
      if(value_ptr)
	*value_ptr=*(finishedThread->returnValue);
      free(finishedThread);
    }
    return 0;
    }
  else {
    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
    return -1;
  }
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
  printf("Mutex init \n");
  mutex->lock=0;
  mutex->owner =0;
  mutex->count=1;

  return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
  //printf("Mutex lock called \n");
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGVTALRM);
  sigprocmask(SIG_BLOCK,&signalMask, NULL);
  tcb_ptr currentBlock = getCurrentBlock(queue);
  sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
  if(mutex->owner ==0 && (mutex->owner != currentBlock->thread_id) && mutex->lock==0) {
    while(mutex->count<=0);
    sigprocmask(SIG_BLOCK,&signalMask, NULL);
    mutex->count--;
    mutex->lock=1;
    mutex->owner = currentBlock->thread_id;
    sigprocmask(SIG_UNBLOCK,&signalMask, NULL);
    return 0;
  }
  else {
    //sigprocmask(SIG_BLOCK,&signalMask,NULL);
    while(1) {
      printf("\n Spinning \n");
      if(mutex->owner==0)
	break;
    }
    //sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
  }

  return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
  //printf("Mutex unlock called \n");
  sigprocmask(SIG_BLOCK,&signalMask,NULL);
  tcb_ptr currentThread = getCurrentBlock(queue);
  if(mutex->owner == currentThread->thread_id) {
    mutex->count++;
    mutex->lock=0;
    mutex->owner =0;
  }

  sigprocmask(SIG_UNBLOCK,&signalMask, NULL);

  return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
  printf("Mutex destroy \n");
  mutex->lock = -1;
  mutex->owner = -1;
  mutex->count = -1;
  return 0;
};
///////////////////////////////////////////////////////
