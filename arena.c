#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/mman.h>

#include "base.h"

void arena_init(Arena *a) {
	a->memory = mmap(NULL, RESERVED_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (a->memory == MAP_FAILED) {
		fputs("Error: Failed to reserve memory for the arena.\n", stderr);
		exit(1);
	}
	a->offset = 0;
	a->commited = 0;
	a->available = RESERVED_SIZE;
}

void *arena_alloc(Arena *a, usize size) {
	if (a->offset + size >= a->commited) {
		usize mem_to_commit = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
		assert(mem_to_commit > 0);

		if (a->commited + mem_to_commit > a->available) {
			fputs("Error: Arena is full.\n", stderr);
			exit(1);
		}

		if (mprotect((char *)a->memory + a->commited, mem_to_commit, PROT_READ | PROT_WRITE) == -1) {
			fputs("Error: Failed to commit memory for the arena.\n", stderr);
			exit(1);
		}
		a->commited += mem_to_commit;
	}
	void *p = (char *)a->memory + a->offset;
	a->offset += size;

	assert(p != NULL);
	return p;
}

void arena_free(Arena *a) {
	a->offset = 0;
}
