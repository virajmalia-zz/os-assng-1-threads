// File:	parallelCal.c
// Author:	Yujie REN
// Date:	09/23/2017

#include <stdio.h>
#include <unistd.h>

#include "../my_pthread_t.h"

#define THREAD_NUM 10

#define C_SIZE 100000
#define R_SIZE 1000

my_pthread_mutex_t   mutex;

int thread[THREAD_NUM];

int*    a[R_SIZE];
int	 pSum[R_SIZE];
int  sum;

/* A CPU-bound task to do parallel array addition */
void parallel_calculate(void* arg) {
	int i = 0, j = 0;
	char *t_name = (char *) arg;
	int n = atoi(t_name) - 1;
	for (j = n; j < R_SIZE; j += THREAD_NUM) {
		for (i = 0; i < C_SIZE; ++i) {
			pSum[j] += a[j][i] * i;
		}
	}
	for (j = n; j < R_SIZE; j += THREAD_NUM) {
		my_pthread_mutex_lock(&mutex);
		sum += pSum[j];
		my_pthread_mutex_unlock(&mutex);
	}
}

int main() {
	int i = 0, j = 0;
	char name[2];

	// initialize data array
	for (i = 0; i < R_SIZE; ++i)
		a[i] = (int*)malloc(C_SIZE*sizeof(int));

	for (i = 0; i < R_SIZE; ++i)
		for (j = 0; j < C_SIZE; ++j)
			a[i][j] = j;

	for (i = 0; i < THREAD_NUM; ++i) {
		sprintf(name, "%d", i+1);
		my_pthread_create(&thread[i], NULL, &parallel_calculate, name);
	}

	for (i = 0; i < THREAD_NUM; ++i)
		my_pthread_join(thread[i], NULL);

	return 0;
}
