#ifndef MEMORY_H
#define MEMORY_H

#define INITIAL_CAPACITY 1024

#include <stdlib.h>

typedef struct {
	char *data;
	size_t capacity;
	size_t size;
} a;

void init_array(a *arr);
void resize_array(a *arr);
void free_array(a *arr);

#endif
