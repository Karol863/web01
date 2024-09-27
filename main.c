#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "threads.h"
#include "memory.h"

#define PORT 6969
#define HTTP_HEADER_OK "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HTTP_HEADER_POST "HTTP/1.1 201 Created\r\nContent-Type: application/octet-stream\r\nConnection: close\r\n\r\n"
#define HTTP_HEADER_ERROR "HTTP/1.1 404 Not Found\r\n\r\n"

a buf;

static void *process_request(void *arg) {
	int *client_socket = (int *)arg;
	printf("Processing: %d\n", *client_socket);
	return NULL;
}

static void *received_buf(void *arg) {
	int *buf_recv = (int *)arg;
	printf("Processing: %d\n", *buf_recv);
	return NULL;
}

static void *sent_request(void *arg) {
	int *sent_bytes = (int *)arg;
	printf("Processing: %d\n", *sent_bytes);
	return NULL;
}

int main(void) {
	threads *t = malloc(sizeof(threads));

	t = init_queue(t, 6);
	init_array(&buf);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		fputs("Failed to create a socket!\n", stderr);
		free_array(&buf);
		destroy(t);
		return -1;
	}

	struct sockaddr_in server_addres;
	server_addres.sin_family = AF_INET;
	server_addres.sin_port = htons(PORT);
	server_addres.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_socket, (const struct sockaddr *)&server_addres, sizeof(server_addres)) == -1) {
		fputs("Failed to bind the socket!\n", stderr);
		free_array(&buf);
		destroy(t);
		return -1;
	}

	if (listen(server_socket, 5) < 0) {
		fputs("Failed to listen to the socket!\n", stderr);
		free_array(&buf);
		destroy(t);
		return -1;
	}

	int *client_socket = malloc(sizeof(int));
	int *buf_recv = malloc(sizeof(int));
	int *sent_bytes = malloc(sizeof(int));

	while (1) {
		*client_socket = accept(server_socket, NULL, NULL);
		if (*client_socket == -1) {
			fputs("Failed to accept the request. Trying again...\n", stderr);
		}

		task task_first = {process_request, client_socket};
		enqueue(t, task_first);

		if (buf.capacity == buf.size) {
			resize_array(&buf);
		}

		*buf_recv = recv(*client_socket, buf.data, buf.capacity, 0);
		if (*buf_recv == -1) {
			fputs("Failed to receive a buffer. Trying again...\n", stderr);
		}

		task task_second = {received_buf, buf_recv};
		enqueue(t, task_second);

		char *method = strtok(buf.data, " ");
		char *path = strtok(NULL, " ");

		if ((strcmp(method, "GET") == 0) && (strncmp(path, "/files/", 7)) == 0) {
			char *filename = strtok(path, "/");
			filename = strtok(NULL, " ");

			FILE *fp = fopen(filename, "r");
			if (fp == NULL) {
				fputs("Failed to open a file!\n", stderr);
			}

			fseek(fp, 0, SEEK_END);
			size_t file_len = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			size_t message_len = (sizeof(HTTP_HEADER_OK) - 1) + file_len;
			char *message = calloc(message_len, sizeof(char));
			strcpy(message, HTTP_HEADER_OK);

			size_t read_size = fread(message + sizeof(HTTP_HEADER_OK), 1, message_len, fp);
			if (read_size != file_len) {
				fputs("Failed to read data from a file!\n", stderr);
			}
			fclose(fp);

			*sent_bytes = send(*client_socket, message, message_len, 0);
			if (*sent_bytes == -1) {
				fputs("Failed to send a request. Trying again...\n", stderr);
			}
			task task_third = {sent_request, sent_bytes};
			enqueue(t, task_third);

			free(message);
		} else if ((strcmp(method, "POST") == 0) && (strncmp(path, "/files/", 7)) == 0) {
			method = strtok(NULL, "\r\n");
			method = strtok(NULL, "\r\n");
			method = strtok(NULL, "\r\n");
			method = strtok(NULL, "\r\n");
			method = strtok(NULL, "\r\n");
			method = strtok(NULL, "\r\n");
			method = strtok(NULL, "\r\n");

			char *filename = strtok(path, "/");
			filename = strtok(NULL, " ");

			FILE *fp = fopen(filename, "w");
			if (fp == NULL) {
				fputs("Failed to open a file!\n", stderr);
			}

			size_t write_size = fwrite(method, 1, strlen(method), fp);
			if (write_size != strlen(method)) {
				fputs("Failed to write data to a file!\n", stderr);
			}
			fclose(fp);

			*sent_bytes = send(*client_socket, HTTP_HEADER_POST, (strlen(HTTP_HEADER_POST)), 0);
			if (*sent_bytes == -1) {
				fputs("Failed to send a request. Trying again...\n", stderr);
			}

			task task_fourth = {sent_request, sent_bytes};
			enqueue(t, task_fourth);
		} else {
			*sent_bytes = send(*client_socket, HTTP_HEADER_ERROR, (strlen(HTTP_HEADER_ERROR)), 0);
			if (*sent_bytes == -1) {
				fputs("Failed to send a request. Trying again...\n", stderr);
			}

			task task_fiveth = {sent_request, sent_bytes};
			enqueue(t, task_fiveth);
		}
		close(*client_socket);
	}
	close(server_socket);
	free_array(&buf);
	destroy(t);
	free(client_socket);
	free(buf_recv);
	free(sent_bytes);
	return 0;
}
