#include "memory.h"

void init_array(a *arr) {
	arr->data = malloc(INITIAL_CAPACITY);
	arr->size = 0;
	arr->capacity = INITIAL_CAPACITY;
}

void resize_array(a *arr) {
	if (arr->capacity == arr->size) {
		arr->capacity *= 2;
		arr->data = realloc(arr->data, arr->capacity);
	}
}

void free_array(a *arr) {
	free(arr->data);
	arr->size = 0;
	arr->capacity = 0;
}
