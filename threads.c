#include "threads.h"

threads *init_queue(threads *t, int num_threads) {
	pthread_mutex_init(&t->mutex, NULL);
	pthread_cond_init(&t->cond, NULL);

	t->stop = 0;
	t->threads = malloc(num_threads);

	for (int i = 0; i < num_threads; i++) {
		pthread_create(&t->threads[i], NULL, worker, t);
	}

	return t;
}

int isFull(threads *t) {
	return (t->rear + 1) % QUEUE_SIZE == t->front;
}

int isEmpty(threads *t) {
	return t->rear == t->front;
}

void enqueue(threads *t, task data) {
	pthread_mutex_lock(&t->mutex);

	if (isFull(t)) {
		pthread_cond_wait(&t->cond, &t->mutex);
	}

	t->rear = (t->rear + 1) % QUEUE_SIZE;
	t->items[t->rear] = data;

	pthread_cond_signal(&t->cond);
	pthread_mutex_unlock(&t->mutex);
}

task dequeue(threads *t) {
	pthread_mutex_lock(&t->mutex);

	if (isEmpty(t)) {
		pthread_cond_wait(&t->cond, &t->mutex);
	}

	task data = t->items[t->front];

	if (t->front == t->rear) {
		t->front = 0;
		t->rear = 0;
	}

	t->front = (t->front + 1) % QUEUE_SIZE;

	pthread_cond_signal(&t->cond);
	pthread_mutex_unlock(&t->mutex);

	return data;
}

void *worker(void *arg) {
	threads *t = (threads *)arg;
	task data;

	while (t->stop == 0) {
		data = dequeue(t);

		if (data.function) {
			data.function(data.arg);
		}
	}
	return NULL;
}

void destroy(threads *t) {
	t->stop = 1;

	for (int i = 0; i < t->num_threads; i++) {
		pthread_cond_signal(&t->cond);
	}

	for (int i = 0; i < t->num_threads; i++) {
		pthread_join(t->threads[i], NULL);
	}

	enqueue(t, (task) {NULL, NULL});
	free(t->threads);
	free(t);
}
