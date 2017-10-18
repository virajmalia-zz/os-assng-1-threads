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
void scheduler(int signum);
void *helper(void *(*function)(void*), void *arg);
//thread_Queue queue[4] = NULL;   // 4 priority level queues
thread_HQ queue = NULL;
finished_Queue finishedQueue = NULL;
tcb_ptr getCurrentControlBlock_Safe();
long millisec;
clock_t threadBegin[22];
clock_t threadEnd[22];
//timer array - size = maz thread + 2
extern double threadTime[22];
clock_t begin;
clock_t end;

/********************************
*
*   Binary Heap Priority Queue
*
*********************************/

int size, count;

thread_HQ heap_init(){
    thread_HQ h;
	h->count = 0;
	h->size = MAXTHREADS;
	//h->heaparr = (tcb_ptr) malloc(sizeof(tcb_ptr) * MAXTHREADS);
	if(!h->heaparr) {
		printf("Error allocatinga memory...\n");
		exit(-1);
	}
    return h;
}

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

	if(largest != loc) {
		temp = data[loc];
		data[loc] = data[largest];
		data[largest] = temp;
		max_heapify(data, largest, count);
	}
}

void heap_push(thread_HQ h, tcb_ptr value){
    sigprocmask(SIG_BLOCK, &signalMask, NULL);
	int index, parent;
    /*
	// Resize the heap if it is too small to hold all the data
	if (h->count == h->size){
		h->size += h->size;
		h->heaparr = realloc(h->heaparr, sizeof(tcb_ptr) * h->size);
		if (!h->heaparr)
            exit(-1); // Exit if the memory allocation fails
	}
    */
 	index = h->count; // First insert at last of array

 	// Find out where to put the element and put it
	for(;index; index = parent){
		parent = (index - 1) / 2;
		if( h->heaparr[parent]->priority >= value->priority )
            break;
		h->heaparr[index] = h->heaparr[parent];
	}
	h->heaparr[index] = value;
    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
}

tcb_ptr heap_pop(thread_HQ h){
    sigprocmask(SIG_BLOCK, &signalMask, NULL);
	tcb_ptr removed;
    h->count--;
	tcb_ptr temp = h->heaparr[h->count];

    /*
	if ((h->count <= (h->size + 2)) && (h->size > initial_size))
	{
		h->size -= 1;
		h->heaparr = realloc(h->heaparr, sizeof(tcb_ptr) * h->size);
		if (!h->heaparr)
            exit(-1); // Exit if the memory allocation fails
	}
    */
    // Swap last and
    removed = h->heaparr[0];
 	h->heaparr[0] = temp;
 	max_heapify(h->heaparr[0], 0, h->count);

    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
 	return removed;
}

tcb_ptr heap_peek(thread_HQ h){
    return h->heaparr[0];
}

int getQSize(thread_HQ h){
    return h->count;
}

void heap_delete(thread_HQ h){
    free(h);
}

/********* End BHPQ *********/

/*****************************
*
*   Thread Context Functions
*
*****************************/

tcb_ptr getCurrentBlock(thread_HQ queue){

  if(queue != NULL) {
      printf("In getCurrentBlock\n");
    printf("%d\n", queue->heaparr[0]->thread_id);  // Seg fault here
    return queue->heaparr[0];
  }
  printf("Returning NULL");
  return NULL;
}

tcb_ptr getCurrentBlockByThread(thread_HQ queue, my_pthread_t threadid) {
    printf("In getCurrentBlockByThread: %d\n", threadid);
    tcb_ptr headBlock = getCurrentBlock(queue);

    printf("headBlock: %d", headBlock->thread_id);
  //if this is the required node
  if(headBlock!=NULL && headBlock->thread_id == threadid){
      printf("headblock NULL");
    return headBlock;
}

  tcb_ptr dummyThread=NULL;

  if(headBlock!=NULL){
      printf("headblock not NULL");
    dummyThread = headBlock->next;
}

    printf("Dummy thread: %d\n", dummyThread->thread_id);
  while(headBlock != dummyThread) {

    if(dummyThread->thread_id == threadid){
        printf("dummyThread");
      return dummyThread;
    }

    dummyThread = dummyThread->next;
  }

  return NULL;
}

tcb_ptr getControlBlock_Main(){
  tcb_ptr controlBlock = (tcb_ptr)malloc(sizeof(tcb));
  controlBlock->thread_context.uc_stack.ss_flags = 0;
  controlBlock->thread_context.uc_link =0;
  controlBlock->isActive =0;
  controlBlock->isBlocked =0;
  controlBlock->isExecuted =0;
  controlBlock->isMain =1 ;
  controlBlock->priority = 4;
  controlBlock->t_count = 0;
  controlBlock->max_count = 1;
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
  controlBlock->isMain =0;
  controlBlock->next = NULL;

  return controlBlock;

}

void freeControlBlock(tcb_ptr controlBlock) {
  if(!(controlBlock->isMain))
    free(controlBlock->thread_context.uc_stack.ss_sp);

  free(controlBlock);
}

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

  while(blockedThread != NULL)
  {
    blockedThread->thread->isBlocked =0;
    blockedThread = blockedThread->next;
  }

  printf("\n Thread completed : %d",currentNode->thread_id );
  currentNode->isExecuted=1;
  raise(SIGVTALRM);
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

/******* End Context Functions *******/

/******************
*
*    Scheduler
*
******************/

void setMaxCount(tcb_ptr cc){
    switch(cc->priority){
        case 4:
            cc->max_count = 1;
            break;
        case 3:
            cc->max_count = 3;
            break;
        case 2:
            cc->max_count = 7;
            break;
        case 1:
            cc->max_count = 15;
            break;
    }
}

void scheduler(int signum){
    sigprocmask(SIG_BLOCK, &signalMask, NULL);

    int q_size = getQSize(queue);
    int to_be_removed = 0;

    if(q_size == 1){
        tcb_ptr curr_context = getCurrentBlock(queue);
        if( curr_context->isExecuted ){
            // If current context has finished execution, dequeue
            heap_pop(queue);
        }
        else{
            // Thread timer check, decrease priority if uses full quanta
            if( curr_context->t_count == curr_context->max_count ){
                tcb_ptr cc = heap_pop(queue);
                curr_context->priority--;
                setMaxCount(curr_context);
                heap_push(queue, cc);
            }
            curr_context->t_count++;
        }
    }   // end if (q_size == 1)
    else if(q_size > 1){

            tcb_ptr curr_context = getCurrentBlock(queue);

            if( curr_context != NULL ){
                // Current context check
                if( curr_context->isExecuted ){
                    to_be_removed = 1;
                    // dequeue
                    heap_pop(queue);
                }
                else{
                    // Thread timer check, decrease priority if uses full quanta
                    if( curr_context->t_count == curr_context->max_count ){
                        tcb_ptr cc = heap_pop(queue);
                        curr_context->priority--;
                        setMaxCount(curr_context);
                        heap_push(queue, cc);
                    }
                    tcb_ptr temp_head = heap_pop(queue);    // For next context
                    heap_push(queue, temp_head);
                }

                tcb_ptr next_context = getCurrentBlock(queue);

                while( next_context != NULL && next_context != curr_context && ( next_context->isBlocked || next_context->isExecuted ) ){

                    if( next_context->isExecuted ){
                        // dequeue
                        heap_pop(queue);
                    }
                    else{
                        tcb_ptr temp_head = heap_pop(queue);    // For next context
                        heap_push(queue, temp_head);
                    }
                    next_context = getCurrentBlock(queue);

                }

                // If !next_context, Go to next priority queue
                if( next_context == NULL ){
                    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
                    exit(0);
                }

                if( next_context != curr_context ){
                    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
                    if( to_be_removed ){
                        // Set next thread as active, discard current thread
                        setcontext( &(next_context->thread_context) );
                        next_context->t_count++;
                    }
                    else{
                        // Swap current thread with next thread
                        swapcontext( &(curr_context->thread_context), &(next_context->thread_context) );
                        next_context->t_count++;
                    }
                }

            }   // end curr_context == NULL

        }
        sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
}

/***** End Scheduler *****/

/**********************
*
*   My Pthread API
*
**********************/

// init process
void my_pthread_init(long period){
  threadid = 1;
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGVTALRM);
  //intializing the context of the scheduler
  finishedQueue = getFinishedQueue();
  queue = heap_init();
  millisec = period;
  tcb_ptr mainThread = getControlBlock_Main();
  //getcontext(&(MainThread->thread_context));
  printf("in init \n");
  getCommonContext();
  mainThread->thread_context.uc_link = &common_context;
  mainThread->thread_id = threadid;
  heap_push(queue, mainThread);
  memset(&scheduler_interrupt_handler, 0, sizeof (scheduler_interrupt_handler));
  scheduler_interrupt_handler.sa_handler= &scheduler;
  sigaction(SIGVTALRM,&scheduler_interrupt_handler,NULL);
  millisec = period;
  timeslice.it_value.tv_sec = 0;
  timeslice.it_interval.tv_sec = 0;
  timeslice.it_value.tv_usec = millisec; // timer start decrementing from here to 0
  timeslice.it_interval.tv_usec = millisec; //timer after 0 resets to this value
  setitimer(ITIMER_VIRTUAL, &timeslice, NULL);
  printf("Exiting init\n");
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
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
    threadCB->priority = 4;
    threadCB->t_count = 0;
    threadCB->max_count = 1;
    threadCB->thread_context.uc_link = &common_context;
    //temp =rand();
    threadCB->thread_id= ++threadid;
    *thread = threadCB->thread_id;

    makecontext(&(threadCB->thread_context),(void (*)(void))&helper,2,function,arg);

    printf("Thread is created %d\n", *thread);
    threadBegin[*thread] = clock();
    heap_push(queue, threadCB);
    printf("Pushed\n");
    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
    sigemptyset(&(threadCB->thread_context.uc_sigmask));
    return 0;
  }
  printf("Error: init() function not executed/n");
  return 0;
};

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
  sigprocmask(SIG_UNBLOCK, &signalMask, NULL);

  return currentControlBlock;

}

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
  tcb_ptr currentThread= getCurrentBlock(queue);    // How to handle multiple queues??
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
    printf("Joining thread %d\n", thread );
  sigprocmask(SIG_BLOCK,&signalMask,NULL);
  tcb_ptr callingThread = getCurrentBlock(queue);
  tcb_ptr joinThread = getCurrentBlockByThread(queue, thread); //How do you handle multiple queues
  // *thread = joinThread->thread_id;

  //check if callingthread is blocking on itself or is null
  if(callingThread == NULL || callingThread == joinThread) {
    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
    printf("calling thread NULL");
    return -1;
  }
  if(joinThread == NULL) {
      printf("join thread NULL");
    //The thread is finished hence can be found in finished Queue
    finishedThread_ptr finishedThread = getFinishedThread(finishedQueue,thread,1);
    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
    if(finishedThread) {
      if(value_ptr)
	*value_ptr =*(finishedThread->returnValue);
      free(finishedThread);
      threadEnd[joinThread->thread_id]=clock();
      threadTime[joinThread->thread_id] = (double) (threadEnd[joinThread->thread_id]-threadBegin[joinThread->thread_id])/CLOCKS_PER_SEC;
      printf("printing join 0");
      return 0;
    }
    else{
        printf("printing join -1");
      return -1;
  }

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
      threadEnd[joinThread->thread_id]=clock();
      threadTime[joinThread->thread_id] = (double) (threadEnd[joinThread->thread_id]-threadBegin[joinThread->thread_id])/CLOCKS_PER_SEC;
    }
    return 0;
    }
  else {
    sigprocmask(SIG_UNBLOCK,&signalMask,NULL);
    return -1;
  }
};

double * get_thread_time() {
  return threadTime;
}

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
  printf("Mutex lock called \n");
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
  printf("Mutex unlock called \n");
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

/******* End API *******/
