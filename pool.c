#include <stdio.h>
#include <unistd.h>

#include "pool.h"

void enqueue(Queue *q, Task t) {
	pthread_mutex_lock(&q->mutex);

	if (unlikely((q->tail + 1) % QUEUE == q->head)) {
		fputs("Queue is full!\n", stderr);
		return;
	}

	q->data[q->tail] = t;
	q->tail = (q->tail + 1) % QUEUE;

	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->mutex);
}

Task dequeue(Queue *q, Task *t) {
	pthread_mutex_lock(&q->mutex);

	while (q->head == q->tail) {
		pthread_cond_wait(&q->cond, &q->mutex);
	}

	*t = q->data[q->head];
	q->head = (q->head + 1) % QUEUE;

	pthread_mutex_unlock(&q->mutex);
	return *t;
}

void *worker(void *arg) {
	Queue *q = (Queue *)arg;
	Task t;

	for (;;) {
		dequeue(q, &t);
		t.func(t.client_socket);
		close(t.client_socket);
	}
}
