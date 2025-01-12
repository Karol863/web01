#ifndef BASE_H
#define BASE_H

#include <stdint.h>
#include <pthread.h>

#define QUEUE 256 
#define THREADS 4

#define RESERVED_SIZE 34359738368
#define PAGE_SIZE 4096

typedef uint16_t u16;
typedef uint64_t u64;

typedef struct {
	void (*func) (int client_socket);
	int client_socket;
} Task;

typedef struct {
	Task data[QUEUE];
	pthread_t thread[THREADS];
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	u16 tail;
	u16 head;
} Queue;

typedef struct {
	char *memory;
	u64 offset;
	u64 commited;
	u64 available;
} Arena;

#endif
