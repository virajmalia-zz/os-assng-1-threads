#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

void puts_thread_scheduling(char *who)
{
  struct sched_param thread_param;
  pthread_attr_t thread_attr;
  int thread_policy = 0;

  pthread_attr_init(&thread_attr);
  pthread_attr_getschedparam(&thread_attr, &thread_param);
  pthread_attr_getschedpolicy(&thread_attr, &thread_policy);
  printf("[%s] priority: %d\n", who, thread_param.sched_priority);
  printf("[%s] schedule: ", who);
  switch(thread_policy){
  case SCHED_FIFO:  printf("FIFO");   break;
  case SCHED_RR:    printf("RR");     break;
  case SCHED_OTHER: printf("OTHER");  break;
  default:          printf("UNKONW"); break;
  }
  printf("\n");

}

void set_policy(char *who)
{
  struct sched_param param;
  int priority = 0;
  int rc = 0;

  memset(&param, 0, sizeof(param));
  priority = sched_get_priority_max(SCHED_FIFO);
  if(priority == -1)
    printf("error get_priority_max\n");
  else
    printf("priority max %d\n", priority);

  param.sched_priority = priority;
  rc = sched_setscheduler(syscall(SYS_gettid), SCHED_FIFO, &param);
  if(rc == -1)
    printf("error sched_setscheduler\n");

  rc = sched_getscheduler(syscall(SYS_gettid));
  if(rc == -1)
    printf("error sched_setscheduler\n");
  else{
    switch(rc){
    case SCHED_FIFO:  printf("FIFO");   break;
    case SCHED_RR:    printf("RR");     break;
    case SCHED_OTHER: printf("OTHER");  break;
    default:          printf("UNKONW"); break;
    }
    printf("\n");
  }

  rc = sched_getparam(syscall(SYS_gettid), &param);
  if(rc == -1)
    printf("error sched_getparam\n");
  else
    printf("priority %d\n", param.sched_priority);

}

void *func1(void *arg)
{
  unsigned long i = 50000000;
  unsigned int j = 0;
  cpu_set_t mask;
  int rc;

  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  rc = sched_setaffinity(syscall(SYS_gettid), sizeof(mask), &mask);
  if(rc == -1)
    printf("error: sched_setaffinity\n");

  set_policy("func1");
  while(i--){
    if(i == 1){
      printf("func1 %d\n", j);
      i = 50000000;
      j++;
      sched_yield();
    }
  }

  return NULL;
}

void *func2(void *arg)
{
  unsigned long i = 50000000;
  unsigned int j = 0;
  while(i--){
    if(i == 1){
      printf("func2 %d\n", j);
      i = 50000000;
      j++;
      //sched_yield();
    }
  }
  return NULL;
}


int main()
{
  pthread_attr_t thread_attr;
  pthread_t id[2];

  printf("FIFO min: %d\n", sched_get_priority_min(SCHED_FIFO));
  printf("FIFO max: %d\n", sched_get_priority_max(SCHED_FIFO));
  printf("RR   min: %d\n", sched_get_priority_min(SCHED_RR));
  printf("RR   max: %d\n", sched_get_priority_max(SCHED_RR));

  //set_policy("main");

  my_pthread_attr_init(&thread_attr);
  my_pthread_create(&id[0], &thread_attr, func1, NULL);
  my_pthread_create(&id[1], &thread_attr, func2, NULL);

  my_pthread_join(id[0], NULL);
  my_pthread_join(id[1], NULL);

  return 0;
}