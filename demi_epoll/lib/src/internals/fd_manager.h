#pragma once

#include <stddef.h>

typedef struct socket {
	int qd;

	// TODO: actually fill this with stuff please im going to die
} socket_t;

typedef struct {
	socket_t *sockets;
	size_t sockets_size;
	int next_free;
} sockets_t;

int next_socket(void);
int close_socket(int sock_fd);
