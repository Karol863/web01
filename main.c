#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "base.h"

#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 6969
#define HTTP_HEADER_OK "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HTTP_HEADER_POST "HTTP/1.1 201 Created\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n"
#define HTTP_HEADER_ERROR "HTTP/1.1 404 Not Found\r\n\r\n"

static char *get_filename(Arena *arena, char *buf_recv) {
	char *start = strchr(buf_recv, '/');
	start = strchr(start + 1, '/');
	char *end = strchr(start, ' ');
	assert(start != NULL);
	assert(end != NULL);

	usize length = end - start;
	char *filename = arena_alloc(arena, length);

	memcpy(filename, start + 1, length - 1);
	filename[length - 1] = '\0';

	assert(filename != NULL);
	return filename;
}

static char *get_message(char *buf_recv) {
	const char str[] = "Content-Type: application/x-www-form-urlencoded";

	char *message = strstr(buf_recv, str);
	message += (sizeof(str) - 1) + 2;

	assert(message != NULL);
	return message;
}

static void get(Arena *arena, int client_socket, char *buf_recv) {
	char *filename = get_filename(arena, buf_recv);
	assert(filename != NULL);

	FILE *f = fopen(filename, "r");
	if (f == NULL) {
		fputs("Error: Failed to open a file.\n", stderr);
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	usize file_len = ftell(f);
	fseek(f, 0, SEEK_SET);

	usize buf_get_len = (sizeof(HTTP_HEADER_OK) - 1) + file_len;
	assert(buf_get_len > 0);

	char *buf_get = arena_alloc(arena, buf_get_len);
	memcpy(buf_get, HTTP_HEADER_OK, sizeof(HTTP_HEADER_OK) - 1);

	usize read_size = fread(buf_get + sizeof(HTTP_HEADER_OK), 1, buf_get_len, f);
	if (ferror(f) || read_size >= buf_get_len) {
		fprintf(stderr, "Error: Failed to read from the %s file.\n", filename);
		exit(1);
	}
	fclose(f);

	if (send(client_socket, buf_get, buf_get_len, 0) == -1) {
		fputs("Error: Failed to send a request.\n", stderr);
		exit(1);
	}
}

static void post(Arena *arena, int client_socket, char *buf_recv) {
	char *message = get_message(buf_recv);
	char *filename = get_filename(arena, buf_recv);
	assert(message != NULL);
	assert(filename != NULL);

	FILE *f = fopen(filename, "w");
	if (f == NULL) {
		fputs("Error: Failed to open a file.\n", stderr);
		exit(1);
	}

	fwrite(message, 1, strlen(message), f);
	if (ferror(f)) {
		fprintf(stderr, "Error: Failed to write data into the %s file.\n", filename);
		exit(1);
	}
	fclose(f);

	if (send(client_socket, HTTP_HEADER_POST, sizeof(HTTP_HEADER_POST) - 1, 0) == -1) {
		fputs("Error: Failed to send a request.\n", stderr);
		exit(1);
	}
}

static void error(int client_socket) {
	if (send(client_socket, HTTP_HEADER_ERROR, sizeof(HTTP_HEADER_ERROR) - 1, 0) == -1) {
		fputs("Error: Failed to send a request.\n", stderr);
		exit(1);
	}
}

static void *worker(void *arg) {
	Queue *q = arg;
	Arena arena;
	arena_init(&arena);

	for (;;) {
		Task t = dequeue(q);

		char buf_recv[PAGE_SIZE];
		ssize_t recv_func = recv(t.client_socket, buf_recv, PAGE_SIZE - 1, 0);
		if (recv_func == -1) {
			fputs("Error: Failed to receive a message from the socket.\n", stderr);
			continue;
		}
		buf_recv[recv_func] = '\0';

		if ((strncmp(buf_recv, "GET", 3) == 0) && (strncmp(buf_recv + 4, "/files/", 7) == 0)) {
			get(&arena, t.client_socket, buf_recv);
		} else if ((strncmp(buf_recv, "POST", 4) == 0) && (strncmp(buf_recv + 5, "/files/", 7) == 0)) {
			post(&arena, t.client_socket, buf_recv);
		} else {
			error(t.client_socket);
		}

		close(t.client_socket);
		arena_free(&arena);
	}
	return NULL;
}

int main(void) {
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		fputs("Error: Failed to create a socket.\n", stderr);
		return 1;
	}

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int reuse = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1) {
		fputs("Error: Failed to reuse the port.\n", stderr);
		return 1;
	}

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		fputs("Error: Failed to bind the socket.\n", stderr);
		return 1;
	}

	if (listen(server_socket, 10) < 0) {
		fputs("Error: Failed to listen to the socket.\n", stderr);
		return 1;
	}

	Queue queue = {0};

	pthread_mutex_init(&queue.mutex, NULL);
	pthread_cond_init(&queue.cond, NULL);

	for (unsigned i = 0; i < THREADS; ++i) {
		pthread_create(&queue.thread[i], NULL, worker, &queue);
	}

	for (;;) {
		int client_socket = accept(server_socket, NULL, NULL);
		if (client_socket == -1) {
			fputs("Error: Failed to accept a request.\n", stderr);
			continue;
		}

		Task task;
		task.client_socket = client_socket;
		enqueue(&queue, task);
	}

	for (unsigned i = 0; i < THREADS; ++i) {
		pthread_join(queue.thread[i], NULL);
	}

	close(server_socket);
	return 0;
}
