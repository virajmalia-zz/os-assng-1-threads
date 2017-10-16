#ifndef NULL
#define NULL   ((void *) 0)
#endif
#include<ucontext.h>
#include "my_pthread_t.h"


tcb_ptr getControlBlock_Main(){
  tcb_ptr controlBlock = (tcb_ptr)malloc(sizeof(tcb));
  controlBlock->thread_context.uc_stack.ss_flags = 0;
  controlBlock->thread_context.uc_link =0;
  controlBlock->isActive =0;
  controlBlock->isBlocked =0;
  controlBlock->isExecuted =0;
  controlBlock->isMain =1 ;
  controlBlock->priority = 0;
  controlBlock->t_count = 0;
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

int enqueue(thread_Queue queue,tcb_ptr tcb) {

  //check if queue or tcb is null
  printf("Enqueing the thread\n");

  if(queue->head == NULL) {
    //this is the first node
    printf("\nThis is first node\n");
    tcb->next= tcb;
    queue->head =tcb;
    queue->tail=tcb;
  }
  else {
    printf("Not first\n");
    tcb->next =queue->head; //inserts tcb behinf the head in a circular queue
    queue->tail->next= tcb; //the existing tail should point to this tcb
    queue->tail =tcb; //the tail is the new tcb hence update it
  }
  queue ->count ++;

  return 0;
}


int dequeue(thread_Queue queue) {

  if(queue == NULL)
    return -1;
  else {
    printf("\ndequeing blocks");
    tcb_ptr head,tail,temp;
    head = queue -> head;
    tail = queue -> tail;

    if(head != NULL) {
      temp = queue->head->next; //removing the head hence storing next block address in temp
      if(queue ->count ==1) {
	     queue->head = queue->tail= NULL;
      }
      else {
	     printf("\n queue has more than 1 elements hence dequeing");
	     queue->head=temp; //temp is next block which is new head
	     tail->next=queue->head;  //tail next block is new head
      }
      freeControlBlock(head); //free the old head
      printf("\nFreed a block on queue");
      queue->count--;
    }
    else {
      return 0;
    }

  }
  return 0;
}

void freeControlBlock(tcb_ptr controlBlock) {
  if(!(controlBlock->isMain))
    free(controlBlock->thread_context.uc_stack.ss_sp);

  free(controlBlock);
}

int next(thread_Queue queue){

  if(queue!= NULL) {
    tcb_ptr current = queue -> head;
    if(current != NULL) {
      queue->tail = current;
      queue->head=current->next;
    }
  }
  printf("\n Returning from next");
  return 0;
}

tcb_ptr getCurrentBlock(thread_Queue queue){

  if(queue !=NULL && queue->head != NULL) {
    printf("\n Returning CurrentBlock\n");
    return queue->head;
  }
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

int getQueueSize(thread_Queue queue) {

  return queue->count;
}

thread_Queue getQueue() {

  thread_Queue queue = (thread_Queue)malloc(sizeof(struct threadQueue));
  queue->count=0;
  queue->head=queue->tail= NULL;
  return queue;
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
