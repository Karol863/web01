#ifndef BASE_H
#define BASE_H

#include <pthread.h>
#include <stddef.h>

typedef size_t usize;

#define QUEUE 256 
#define THREADS 4

#define RESERVED_SIZE 34359738368
#define PAGE_SIZE 4096

typedef struct {
	void *memory;
	usize offset;
	usize commited;
	usize available;
} Arena;

typedef struct {
	int client_socket;
} Task;

typedef struct {
	Task data[QUEUE];
	pthread_t thread[THREADS];
	usize tail;
	usize head;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} Queue;

void arena_init(Arena *a);
void *arena_alloc(Arena *a, usize size);
void arena_free(Arena *a);

void enqueue(Queue *q, Task t);
Task dequeue(Queue *q);

#endif
