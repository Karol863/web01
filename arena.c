#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/mman.h>

#include "base.h"

void arena_init(Arena *a, Queue *q) {
	pthread_mutex_lock(&q->mutex);

	a->memory = mmap(NULL, RESERVED_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (a->memory == MAP_FAILED) {
		fputs("Error: failed to reserve memory for the arena.\n", stderr);
		exit(1);
	}
	a->offset = 0;
	a->commited = 0;
	a->available = RESERVED_SIZE;

	pthread_mutex_unlock(&q->mutex);
}

void *arena_alloc(Arena *a, Queue *q, u64 size) {
	pthread_mutex_lock(&q->mutex);

	if (a->offset + size >= a->commited) {
		u64 mem_to_commit = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
		assert(mem_to_commit > 0);

		if (a->commited + mem_to_commit > a->available) {
			fputs("Error: arena is full.\n", stderr);
			exit(1);
		}

		if (mprotect(a->memory + a->commited, mem_to_commit, PROT_READ | PROT_WRITE) == -1) {
			fputs("Error: failed to commit memory for the arena.\n", stderr);
			exit(1);
		}
		a->commited += mem_to_commit;

		char *p = a->memory + a->offset;
		a->offset += size;
		assert(p != NULL);

		pthread_mutex_unlock(&q->mutex);
		return p;
	}
	return NULL;
}
