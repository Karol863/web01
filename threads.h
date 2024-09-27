#ifndef THREADS_H
#define THREADS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define QUEUE_SIZE 69

typedef struct {
	void * (*function)(void *arg);
	void *arg;
} task;

typedef struct {
	task items[QUEUE_SIZE];
	int num_threads;
	int front;
	int rear;
	int stop;
	pthread_t *threads;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} threads;

threads *init_queue(threads *t, int num_threads);
int isFull(threads *t);
int isEmpty(threads *t);
void enqueue(threads *t, task data);
task dequeue(threads *t);
void *worker(void *arg);
void destroy(threads *t);

#endif
