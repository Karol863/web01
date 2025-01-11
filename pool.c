#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "base.h"

void enqueue(Queue *q, Task t) {
	pthread_mutex_lock(&q->mutex);

	if ((q->tail + 1) % QUEUE == q->head) {
		fputs("Error: queue is full.\n", stderr);
		exit(1);
	}

	q->data[q->tail] = t;
	q->tail = (q->tail + 1) % QUEUE;

	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->mutex);
}

Task dequeue(Queue *q) {
	pthread_mutex_lock(&q->mutex);

	while (q->head == q->tail) {
		pthread_cond_wait(&q->cond, &q->mutex);
	}

	Task t = q->data[q->head];
	q->head = (q->head + 1) % QUEUE;

	pthread_mutex_unlock(&q->mutex);
	return t;
}

void *worker(void *arg) {
	Queue *q = arg;
	Task t;

	for (;;) {
		t = dequeue(q);
		t.func(t.client_socket);
		close(t.client_socket);
	}
}
