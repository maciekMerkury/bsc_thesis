#pragma once
#include <bits/socket.h>

/*
 * all functions here behave like posix socket calls (i.e. return qd or -1 and set errno on failure)
 */

/*
 * function calls act as if the created socket was created with O_NONBLOCK
 */
int dpoll_socket(int domain, int type, int protocol);

int dpoll_bind(int qd, const struct sockaddr *addr, socklen_t addrlen);

int dpoll_connect(int qd, const struct sockaddr *addr, socklen_t size);

int dpoll_accept(int qd, struct sockaddr *addr, socklen_t *addrlen);

int dpoll_listen(int qd, int backlog);

/// the next 2 functions are not really that supported
int dpoll_getsockname(int qd, struct sockaddr *addr, socklen_t *addrlen);

int dpoll_setsockopt(int qd, int level, int optname, const void *optval,
                     socklen_t optlen);

ssize_t dpoll_sendmsg(int qd, const struct msghdr *msg, int flags);

ssize_t dpoll_recvmsg(int qd, struct msghdr *msg, int flags);

int dpoll_close(int qd);

ssize_t dpoll_write(int qd, const void *buf, size_t count);
ssize_t dpoll_read(int qd, void *buf, size_t count);

ssize_t dpoll_readv(int qd, struct iovec* iov, int iovcnt);
ssize_t dpoll_writev(int qd, const struct iovec *iov, int iovcnt);
