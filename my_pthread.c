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

typedef struct {
  my_pthread_t thread_id;
  ucontext_t thread_context;
  int active;
  int executed;
  int exited;
  int isMain;
  void *ret;
}thread;

static int threadid;
extern ucontext_t common_context;
struct sigaction scheduler_interrupt_handler;
struct itimerval timeslice;
sigset_t signalMask;
void scheduler(int signum);
void *helper(void *(*function)(void*), void *arg);
thread_Queue queue[4] = NULL;   // 4 priority level queues
finished_Queue finishedQueue = NULL;
tcb_ptr getCurrentControlBlock_Safe();
long millisec;

// init process
void my_pthread_init(long period){
  threadid = 1;
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGVTALRM);
  //intializing the context of the scheduler
  finishedQueue = getFinishedQueue();
  for(int i=0; i<4; i++){
      queue[i] = getQueue();
  }
  millisec = period;
  tcb_ptr mainThread = getControlBlock_Main();
  //getcontext(&(MainThread->thread_context));
  printf("in init \n");
  getCommonContext();
  mainThread->thread_context.uc_link = &common_context;
  mainThread->thread_id = threadid;
  enqueue(queue[0],mainThread);
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
  int temp;
  if(queue[0] != NULL) {
    sigprocmask(SIG_BLOCK,&signalMask,NULL);
    tcb_ptr  threadCB= getControlBlock_Main();
    getcontext((&threadCB->thread_context));
    threadCB->thread_context.uc_stack.ss_sp=malloc(STACKSIZE);
    threadCB->thread_context.uc_stack.ss_size=STACKSIZE;
    threadCB->thread_context.uc_stack.ss_flags=0;
    threadCB->isMain=0;
    threadCB->priority = 0;
    threadCB->t_count = 0;
    threadCB->thread_context.uc_link = &common_context;
    //temp =rand();
    threadCB->thread_id= ++threadid;
    *thread = threadCB->thread_id;

    makecontext(&(threadCB->thread_context),(void (*)(void))&helper,2,function,arg);

    printf("Thread is created %ld\n", *thread);
    enqueue(queue[0],threadCB);
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
  currentControlBlock = getCurrentBlock(queue[]);    // How to handle multiple queues??
  sigprocmask(SIG_UNBLOCK,&signalMask,null);

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
  tcb_ptr currentThread= getCurrentBlock(queue4);    // How to handle multiple queues??
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
  tcb_ptr callingThread = getCurrentBlock(queue[]);
  tcb_ptr joinThread = getCurrentBlockByThread(queue[],thread); //How do you handle multiple queues

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
  printf("Mutex lock called \n");
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGVTALRM);
  sigprocmask(SIG_BLOCK,&signalMask, NULL);
  tcb_ptr currentBlock = getCurrentBlock(queue[]);    // How to handle multiple queues??
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
  tcb_ptr currentThread = getCurrentBlock(queue[]);   // How to handle multiple queues??
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
  mutex->lock = NULL;
  mutex->owner = NULL;
  mutex->count = NULL;
	return 0;
};
