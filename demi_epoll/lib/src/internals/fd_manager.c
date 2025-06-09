#include "fd_manager.h"

#include <demi/libos.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

extern sockets_t sockets;

static int qd_close(int qd)
{
	const int ret = demi_close(qd);
	if (ret == 0)
		return 0;
	errno = ret;
	return -1;
}

static int destroy_socket(socket_t *soc)
{
	int ret = qd_close(soc->qd);
	if (ret < 0)
		return -1;
	return 0;
}

int next_socket(void)
{
	if (sockets.next_free > -1) {
		int soc = sockets.next_free;
		sockets.next_free = sockets.sockets[soc].qd;
		return soc;
	}

	const size_t size = sizeof(socket_t) * (++sockets.sockets_size);
	sockets.sockets = realloc(sockets.sockets, size);
	assert(sockets.sockets);
	return sockets.sockets_size - 1;
}

int close_socket(int sock_fd)
{
	if (sock_fd >= sockets.sockets_size) {
		errno = EBADF;
		return -1;
	}
	socket_t *soc = sockets.sockets + sock_fd;
	destroy_socket(soc);
	if (sockets.next_free == -1) {
		sockets.next_free = sock_fd;
		soc->qd = -1;
	} else {
		soc->qd = sockets.next_free;
		sockets.next_free = sock_fd;
	}
	return 0;
}

