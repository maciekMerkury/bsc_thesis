#pragma once
#include <bits/socket.h>

/*
 * all functions here behave like posix socket calls (i.e. return qd or -1 and set errno on failure)
 */

/*
 * function calls act as if the created socket was created with O_NONBLOCK
 */
int dsoc_socket(int domain, int type, int protocol);

int dsoc_bind(int qd, const struct sockaddr *addr, socklen_t addrlen);

int dsoc_connect(int qd, const struct sockaddr *addr, socklen_t size);

int dsoc_accept(int qd, struct sockaddr *addr, socklen_t *addrlen);

int dsoc_listen(int qd, int backlog);

/// the next 2 functions are not really that supported
int dsoc_getsockname(int qd, struct sockaddr *addr, socklen_t *addrlen);

int dsoc_setsockopt(int qd, int level, int optname, const void *optval,
                    socklen_t optlen);

ssize_t dsoc_send(int qd, const void *buf, size_t len);
ssize_t dsoc_sendmsg(int qd, const struct msghdr *msg, int flags);

ssize_t dsoc_recv(int qd, void *buf, size_t len);
ssize_t dsoc_recvmsg(int qd, struct msghdr *msg, int flags);

int dsoc_close(int qd);
