#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "pool.h"

#define PORT 6969
#define HTTP_HEADER_OK "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HTTP_HEADER_POST "HTTP/1.1 201 Created\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n"
#define HTTP_HEADER_ERROR "HTTP/1.1 404 Not Found\r\n\r\n"

#define CAPACITY 268435456

static char *buf_recv;
static char *buf_get;

static char *path;
static char *method;
static char *saveptr;

void get(int client_socket) {
	char *filename = strtok_r(path, "/", &saveptr);
	filename = strtok_r(NULL, " ", &saveptr);

	FILE *f = fopen(filename, "r");
	if (f == NULL) {
		fputs("File not found!\n", stderr);
		return;
	}
	fseek(f, 0, SEEK_END);
	usize file_len = ftell(f);
	fseek(f, 0, SEEK_SET);

	usize buf_get_len = (sizeof(HTTP_HEADER_OK) - 1) + file_len;
	strcpy(buf_get, HTTP_HEADER_OK);

	usize read_size = fread(buf_get + sizeof(HTTP_HEADER_OK), 1, buf_get_len, f);
	if (read_size > CAPACITY - 1) {
		fputs("The file is too big!\n", stderr);
	}
	fclose(f);

	if (send(client_socket, buf_get, buf_get_len, 0) == -1) {
		fputs("Failed to send a request!\n", stderr);
	}
}

void post(int client_socket) {
	method = strtok_r(NULL, "\r\n", &saveptr);
	method = strtok_r(NULL, "\r\n", &saveptr);
	method = strtok_r(NULL, "\r\n", &saveptr);
	method = strtok_r(NULL, "\r\n", &saveptr);
	method = strtok_r(NULL, "\r\n", &saveptr);
	method = strtok_r(NULL, "\r\n", &saveptr);
	method = strtok_r(NULL, "\r\n", &saveptr);

	char *filename = strtok_r(path, "/", &saveptr);
	filename = strtok_r(NULL, " ", &saveptr);

	FILE *f = fopen(filename, "w");
	if (f == NULL) {
		fputs("File not found!\n", stderr);
		return;
	}
	usize write_size = fwrite(method, 1, strlen(method), f);
	if (write_size > CAPACITY - 1) {
		fputs("The file is too big!\n", stderr);
	}
	fclose(f);

	if (send(client_socket, HTTP_HEADER_POST, strlen(HTTP_HEADER_POST), 0) == -1) {
		fputs("Failed to send a request!\n", stderr);
	}
}

void error(int client_socket) {
	if (send(client_socket, HTTP_HEADER_ERROR, strlen(HTTP_HEADER_ERROR), 0) == -1) {
		fputs("Failed to send a request!\n", stderr);
	}
}

int main(void) {
	Queue q = {0};
	Arena a_get = {0};
	Arena a_recv = {0};

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (unlikely(server_socket == -1)) {
		fputs("Failed to create a socket!\n", stderr);
		return -1;
	}

	struct sockaddr_in server_adress;
	server_adress.sin_family = AF_INET;
	server_adress.sin_port = htons(PORT);
	server_adress.sin_addr.s_addr = INADDR_ANY;

	if (unlikely(bind(server_socket, (struct sockaddr *)&server_adress, sizeof(server_adress)) == -1)) {
		fputs("Failed to bind the socket!\n", stderr);
		close(server_socket);
		return -1;
	}

	if (unlikely(listen(server_socket, 10) < 0)) {
		fputs("Failed to listen to socket!\n", stderr);
		close(server_socket);
		return -1;
	}

	arena_init(&a_get, &q);
	arena_init(&a_recv, &q);

	pthread_mutex_init(&q.mutex, NULL);
	pthread_cond_init(&q.cond, NULL);

	buf_get = arena_alloc(&a_get, &q, CAPACITY);
	buf_recv = arena_alloc(&a_recv, &q, CAPACITY);

	if (a_get.offset > CAPACITY - 1) {
		arena_alloc(&a_get, &q, CAPACITY * 2);
	}

	if (a_recv.offset > CAPACITY - 1) {
		arena_alloc(&a_recv, &q, CAPACITY * 2);
	}

	for (u8 i = 0; i < THREADS; ++i) {
		pthread_create(&q.thread[i], NULL, worker, &q);
	}

	for (;;) {
		int client_socket = accept(server_socket, NULL, NULL);
		if (client_socket == -1) {
			fputs("Failed to accept the request!\n", stderr);
		}

		int recv_func = recv(client_socket, buf_recv, CAPACITY - 1, 0);
		if (recv_func == -1) {
			fputs("Failed to receive a message from the socket!\n", stderr);
		}

		method = strtok_r(buf_recv, " ", &saveptr);
		path = strtok_r(NULL, " ", &saveptr);

		Task task;
		task.client_socket = client_socket;

		if ((strcmp(method, "GET") == 0) && (strncmp(path, "/files/", 7) == 0)) {
			task.func = get;
			enqueue(&q, task);
		} else if ((strcmp(method, "POST") == 0) && (strncmp(path, "/files/", 7) == 0)) {
			task.func = post;
			enqueue(&q, task);
		} else {
			task.func = error;
			task.client_socket = client_socket;
			enqueue(&q, task);
		}
	}

	for (u8 i = 0; i < THREADS; ++i) {
		pthread_join(q.thread[i], NULL);
	}

	arena_free(&a_get, &q);
	arena_free(&a_recv, &q);

	pthread_mutex_destroy(&q.mutex);
	pthread_cond_destroy(&q.cond);

	close(server_socket);
	return 0;
}
