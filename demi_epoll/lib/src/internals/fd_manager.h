#pragma once

#include <stddef.h>

typedef struct socket {
	int qd;
} socket_t;

typedef struct {
	socket_t *sockets;
	size_t sockets_size;
} sockets_t;
