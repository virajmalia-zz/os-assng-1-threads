// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server: 
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#define MAXTHREADS 20

typedef uint my_pthread_t;

typedef struct threadControlBlock {
	/* add something here */
  my_pthread_t thread_id;
  ucontext_t thread_context;
  int isActive;
  int isExecuted;
  int isBlocked;
  int isMain;
  struct threadControlBlock *next;
  struct BlockedThreadList *blockedThreads;
} tcb, *tcb_ptr; 

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */
  int lock;
  int count;
  volatile my_pthread_t owner;
} my_pthread_mutex_t;

/* define your data structures here: */

// Feel free to add your own auxiliary data structures
typedef struct threadQueue {
  tcb_ptr head;
  tcb_ptr tail;
  long count;
}*thread_Queue;

typedef struct blockedThreadList {
  tcb_ptr thread;
  struct blockedThreadList *next;
}*blockedThreadList_ptr;

typedef struct finishedThread {
  my_pthread_t thread_id;
  void **returnValue;
  struct finishedThread *next;
}*finishedThread_ptr;

typedef struct finishedControlBlockQueue {
  struct finishedThread *thread;
  long count;
}*finishedQueue;

tcb_ptr getControlBlock_Main();
tcb_ptr getControlBlock();
tcb_ptr getCurrentBlockByThread(thread_Queue,my_thread_t);
tcb_ptr getCurrentBlock(thread_Queue queue);
int getQueueSize(thread_Queue queue);
thread_Queue getQueue();
void freeControlBlock(tcb_ptr);
int next(thread_Queue);
int enqueueToCompletedList(finishedQueue,finishedThread_ptr);
finishedThread_ptr getFinishedThread(finishedQueue,my_pthread_t,int);
blockedThreadList_ptr getBlockedThreadList();
int addToBlockedThreadList(tcb_ptr,tcb_ptr);
finishedThread_ftr getCompletedThread();
finishedQueue getFinishedQueue();

/* Function Declarations: */

// init process
void my_pthread_init(long period);

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
