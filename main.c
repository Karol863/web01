#include <string.h>

#include "arena.c"
#include "pool.c"

#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 6969
#define HTTP_HEADER_OK "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HTTP_HEADER_POST "HTTP/1.1 201 Created\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n"
#define HTTP_HEADER_ERROR "HTTP/1.1 404 Not Found\r\n\r\n"

static Queue queue;
static Arena arena;

static char *buf_filename;
static char *buf_recv;
static char *buf_get;

char *get_filename(void) {
	char *start = strchr(buf_recv, '/');
	start = strchr(start + 1, '/');
	char *end = strchr(start, ' ');
	assert(start != NULL && end != NULL);

	u16 length = end - start;
	memcpy(buf_filename, start + 1, length - 1);
	buf_filename[length - 1] = '\0';

	assert(buf_filename != NULL);
	return buf_filename;
}

char *get_message(void) {
	const char *str = "Content-Type: application/x-www-form-urlencoded";

	char *message = strstr(buf_recv, str);
	message += strlen(str) + 2;

	assert(message != NULL);
	return message;
} 

void get(int client_socket) {
	char *filename = get_filename();

	FILE *f = fopen(filename, "r");
	if (f == NULL) {
		fputs("Error: failed to open a file.\n", stderr);
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	u64 file_len = ftell(f);
	fseek(f, 0, SEEK_SET);

	u64 buf_get_len = (sizeof(HTTP_HEADER_OK) - 1) + file_len;
	assert(buf_get_len > 0);
	buf_get = arena_alloc(&arena, &queue, buf_get_len);

	memcpy(buf_get, HTTP_HEADER_OK, sizeof(HTTP_HEADER_OK));

	u64 read_size = fread(buf_get + sizeof(HTTP_HEADER_OK), 1, buf_get_len, f);
	if (ferror(f) || read_size >= buf_get_len) {
		fprintf(stderr, "Error: failed to read from the %s file.\n", buf_filename);
		exit(1);
	}
	fclose(f);

	if (send(client_socket, buf_get, buf_get_len, 0) == -1) {
		fputs("Error: failed to send a request.\n", stderr);
		exit(1);
	}
}

void post(int client_socket) {
	char *message = get_message();
	char *filename = get_filename();

	FILE *f = fopen(filename, "w");
	if (f == NULL) {
		fputs("Error: failed to open a file.\n", stderr);
		exit(1);
	}

	fwrite(message, 1, strlen(message), f);
	if (ferror(f)) {
		fprintf(stderr, "Error: failed to write data into the %s file.\n", filename);
		exit(1);
	}
	fclose(f);

	if (send(client_socket, HTTP_HEADER_POST, (sizeof(HTTP_HEADER_POST) - 1), 0) == -1) {
		fputs("Error: failed to send a request.\n", stderr);
		exit(1);
	}
}

void error(int client_socket) {
	if (send(client_socket, HTTP_HEADER_ERROR, (sizeof(HTTP_HEADER_ERROR) - 1), 0) == -1) {
		fputs("Error: failed to send a request.\n", stderr);
		exit(1);
	}
}

void *worker(void *arg) {
	Queue *q = arg;
	Task t;

	for (;;) {
		t = dequeue(q);
		t.func(t.client_socket);
		close(t.client_socket);
	}
	arena_free(&arena, &queue);
}

int main(void) {
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		fputs("Error: failed to create a socket.\n", stderr);
		return 1;
	}

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		fputs("Error: failed to bind the socket.\n", stderr);
		return 1;
	}

	if (listen(server_socket, 10) < 0) {
		fputs("Error: failed to listen to socket.\n", stderr);
		return 1;
	}

	arena_init(&arena, &queue);
	buf_filename = arena_alloc(&arena, &queue, 2 * PAGE_SIZE);
	buf_recv = arena_alloc(&arena, &queue, PAGE_SIZE);

	pthread_mutex_init(&queue.mutex, NULL);
	pthread_cond_init(&queue.cond, NULL);

	for (u16 i = 0; i < THREADS; ++i) {
		pthread_create(&queue.thread[i], NULL, worker, &queue);
	}

	for (;;) {
		int client_socket = accept(server_socket, NULL, NULL);
		if (client_socket == -1) {
			fputs("Error: failed to accept a request.\n", stderr);
			return 1;
		}

		ssize_t recv_func = recv(client_socket, buf_recv, PAGE_SIZE, 0);
		if (recv_func == -1) {
			fputs("Error: failed to receive a message from the socket.\n", stderr);
			return 1;
		} else if (recv_func < PAGE_SIZE) {
			buf_recv[recv_func] = '\0';
		} else if (recv_func > PAGE_SIZE) {
			buf_recv = arena_alloc(&arena, &queue, recv_func);
			buf_recv[recv_func] = '\0';
		}

		Task task;
		task.client_socket = client_socket;

		if ((strncmp(buf_recv, "GET", 3) == 0) && (strncmp(buf_recv + 4, "/files/", 7) == 0)) {
			task.func = get;
			enqueue(&queue, task);
		} else if ((strncmp(buf_recv, "POST", 4) == 0) && (strncmp(buf_recv + 5, "/files/", 7) == 0)) {
			task.func = post;
			enqueue(&queue, task);
		} else {
			task.func = error;
			enqueue(&queue, task);
		}
	}

	for (u16 i = 0; i < THREADS; ++i) {
		pthread_join(queue.thread[i], NULL);
	}

	return 0;
}
