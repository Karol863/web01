#include <stdio.h>
#include <assert.h>

#include <sys/mman.h>

#include "pool.h"

bool ispoweroftwo(usize x) {
	return (x & (x - 1)) == 0;
}

void arena_init(Arena *a, Queue *q) {
	pthread_mutex_lock(&q->mutex);

	a->memory = mmap(NULL, RESERVED_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (a->memory == MAP_FAILED) {
		fputs("Failed to reserve memory for the arena!\n", stderr);
		pthread_mutex_unlock(&q->mutex);
		return;
	}
	a->offset = 0;
	a->commited = 0;
	a->available = RESERVED_SIZE;

	pthread_mutex_unlock(&q->mutex);
}

void *arena_alloc(Arena *a, Queue *q, usize size) {
	pthread_mutex_lock(&q->mutex);

	assert(ispoweroftwo(size));

	if (a->offset + size >= a->commited) {
		usize mem_to_commit = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

		if (a->commited += mem_to_commit > a->available) {
			fputs("Arena is full!\n", stderr);
			pthread_mutex_unlock(&q->mutex);
			return NULL;
		}

		if (mprotect(a->memory, mem_to_commit, PROT_READ | PROT_WRITE) == -1) {
			fputs("Failed to commit memory for the arena!\n", stderr);
			pthread_mutex_unlock(&q->mutex);
			return NULL;
		}
	}
	void *p = a->memory + a->offset;
	a->offset += size;

	pthread_mutex_unlock(&q->mutex);
	return p;
}

void arena_free(Arena *a, Queue *q) {
	pthread_mutex_lock(&q->mutex);

	if (munmap(a->memory, a->available) == -1) {
		fputs("Failed to free the arena!\n", stderr);
		pthread_mutex_unlock(&q->mutex);
		return;
	}

	pthread_mutex_unlock(&q->mutex);
}
