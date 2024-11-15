#ifndef POOL_H
#define POOL_H

#include <pthread.h>

#include "mbbase.h"

#define QUEUE 69
#define THREADS 4

#define RESERVED_SIZE 1ULL << 40
#define PAGE_SIZE 4096

typedef struct {
	void (*func) (int client_socket);
	int client_socket;
} Task;

typedef struct {
	Task data[QUEUE];
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	pthread_t thread[THREADS];
	u8 tail;
	u8 head;
} Queue;

typedef struct {
	void *memory;
	usize offset;
	usize commited;
	usize available;
} Arena;

void enqueue(Queue *q, Task t);
Task dequeue(Queue *q, Task *t);
void *worker(void *arg);

void arena_init(Arena *a, Queue *q);
void *arena_alloc(Arena *a, Queue *q, usize size);
void arena_free(Arena *a, Queue *q);

#endif
