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

ssize_t dsoc_send(int qd, const void *buf, size_t len);

ssize_t dsoc_recv(int qd, void *buf, size_t len);

int dsoc_close(int qd);
