// File:	externalMerge.c
// Author:	Yujie REN
// Date:	09/23/2017

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread_t.h>

#define THREAD_NUM 10
#define RAM_SIZE 160
#define RECORD_NUM 10
#define RECORD_SIZE 1024

pthread_mutex_t   mutex;

int thread[THREAD_NUM];

int *mem = NULL;

int sum = 0;

void external_calculate(void* arg) {
	int i = 0, j = 0;
	char *t_name = (char *) arg;
	int n = atoi(t_name) - 1;
	int itr = RECORD_SIZE / (RAM_SIZE / THREAD_NUM);

	int fd = open(strcat("./record/", t_name), O_RDONLY);

	for (i = 0; i < itr; ++i) {
		// read 16B from nth record into mem[n]
		read(fd, mem + n*16, 16);
		for (j = 0; j < 40; ++j) {
			pthread_mutex_lock(&mutex);
			sum += mem[j];
			pthread_mutex_unlock(&mutex);
		}
	}
	close(fd);
}


int main() {
	int i = 0;
	char name[2];

	mem = (int*)malloc(RAM_SIZE);

	for (i = 0; i < THREAD_NUM; ++i) {
		sprintf(name, "%d", i+1);
		pthread_create(&thread[i], NULL, &external_calculate, name);
	}

	for (i = 0; i < THREAD_NUM; ++i)
		pthread_join(thread[i], NULL);

	return 0;
}
