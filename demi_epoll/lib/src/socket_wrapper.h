#pragma once

#include <stddef.h>
#include "internals/maybe.h"
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/types.h>
#include "demi_socket.h"

#include <demi/sga.h>

#define MAX_OPS 4

MAYBE_DEF(demi_sgarray_t, sga);

MAYBE_DEF(demi_accept_result_t, accept);

typedef struct socket {
	demi_socket_t qd;
	struct sockaddr_in addr;

	struct sga send;
	// -1 if accepting
	ssize_t recv_off;

	union {
		struct sga recv;
		struct accept accept;
	};
} socket_t;

int socket_init(socket_t *soc);
void socket_destroy(socket_t *soc);
ssize_t maybe_write(socket_t *soc, const void *buf, size_t len);
ssize_t maybe_read(socket_t *soc, void *buf, size_t len);
ssize_t maybe_writev(socket_t *soc, const struct iovec *iov, int iov_cnt);
ssize_t maybe_readv(socket_t *soc, struct iovec *iov, int iov_cnt);
/// returns -1 on error, and qd on success
demi_result_t maybe_accept(socket_t *soc, struct sockaddr_in *addr);

bool socket_can_write(const socket_t *soc);
bool socket_can_read(const socket_t *soc);
bool socket_can_accept(const socket_t *soc);

/// adds the result to the socket
void socket_handle_event(socket_t *soc, const demi_qresult_t *res);

static inline bool socket_is_accepting(const socket_t *soc)
{
	return soc->recv_off == -1;
}
