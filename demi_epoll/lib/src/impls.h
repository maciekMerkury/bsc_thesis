#pragma once

#include <assert.h>
#include <sys/socket.h>
#include "epoll_wrapper.h"
#include <stdbool.h>
#include <sys/epoll.h>

#define DPOLL_EPOLL_OFFSET ((int)1 << 16)
#define DPOLL_SOCKET_OFFSET (DPOLL_EPOLL_OFFSET + 1024)

static inline bool qd_is_dpoll(int qd)
{
	return qd >= DPOLL_EPOLL_OFFSET;
}

static inline bool qd_is_epoll(int qd)
{
	return qd >= DPOLL_EPOLL_OFFSET && qd < DPOLL_SOCKET_OFFSET;
}

static inline int get_epoll_fd(int qd)
{
	assert(qd >= DPOLL_EPOLL_OFFSET);
	return qd - DPOLL_EPOLL_OFFSET;
}

static inline int get_socket_fd(int qd)
{
	assert(qd >= DPOLL_SOCKET_OFFSET);
	return qd - DPOLL_SOCKET_OFFSET;
}

int dpoll_socket_impl(void);

int dpoll_bind_impl(int qd, const struct sockaddr *addr, socklen_t addrlen);

int dpoll_connect_impl(int qd, const struct sockaddr *addr, socklen_t size);

int dpoll_accept_impl(int qd, struct sockaddr *addr, socklen_t *addrlen);

int dpoll_listen_impl(int qd, int backlog);

/// the next 2 functions are not really that supported
int dpoll_getsockname_impl(int qd, struct sockaddr *addr, socklen_t *addrlen);

int dpoll_setsockopt_impl(int qd, int level, int optname, const void *optval,
                          socklen_t optlen);

ssize_t dpoll_sendmsg_impl(int qd, const struct msghdr *msg, int flags);

ssize_t dpoll_recvmsg_impl(int qd, struct msghdr *msg, int flags);

int dpoll_close_impl(int qd);

int dpoll_create_impl(int flags);

int dpoll_ctl_impl(int dpollfd, int op, int fd, struct epoll_event *event);

int dpoll_pwait_impl(int dpollfd, struct epoll_event *events, int maxevents,
                     int timeout, const sigset_t *sigmask);

ssize_t dpoll_write_impl(int qd, const void *buf, size_t count);
ssize_t dpoll_read_impl(int qd, void *buf, size_t count);

ssize_t dpoll_readv_impl(int qd, const struct iovec *iov, int iovcnt);
ssize_t dpoll_writev_impl(int qd, const struct iovec *iov, int iovcnt);

uint32_t available_events(const epoll_item_t *it);
