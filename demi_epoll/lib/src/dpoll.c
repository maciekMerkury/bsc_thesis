#include "dpoll.h"
#include <assert.h>
#include <stdio.h>
#include <demi/libos.h>
#include <unistd.h>
#include "log.h"

#define DPOLL_EPOLL_OFFSET 0

#define DPOLL_SOCKET_OFFSET 0

//#define DPOLL_EPOLL_OFFSET 65536
//
//#define DPOLL_SOCKET_OFFSET (DPOLL_EPOLL_OFFSET + 8)

static int get_epoll_fd(int fd)
{
	assert(fd >= DPOLL_EPOLL_OFFSET);
	return fd - DPOLL_EPOLL_OFFSET;
}

int dpoll_create(int flags)
{
	int fd = epoll_create1(flags);
	if (fd < 0)
		return -1;
	return fd + DPOLL_EPOLL_OFFSET;
}

int dpoll_ctl(int dpollfd, int op, int fd, struct epoll_event *event)
{
	const int epoll = get_epoll_fd(dpollfd);
	if (fd >= DPOLL_SOCKET_OFFSET)
		fd -= DPOLL_SOCKET_OFFSET;
	return epoll_ctl(epoll, op, fd, event);
}

int dpoll_pwait(int dpollfd, struct epoll_event *events, int maxevents,
                int timeout, const sigset_t *sigmask)
{
	const int epoll = get_epoll_fd(dpollfd);
	int ret = epoll_pwait(epoll, events, maxevents, timeout, sigmask);
	if (ret < 0)
		return -1;

	return ret;
}

static int get_socket_fd(int fd)
{
	assert(fd >= DPOLL_SOCKET_OFFSET);
	return fd - DPOLL_SOCKET_OFFSET;
}

int dpoll_socket(int domain, int type, int protocol)
{
	//	assert(domain == AF_INET);
	//	assert(type == SOCK_STREAM || type == SOCK_DGRAM);

	int fd = socket(domain, type, protocol);
	if (fd == -1)
		return -1;
	return fd + DPOLL_SOCKET_OFFSET;
}

int dpoll_bind(int qd, const struct sockaddr *addr, socklen_t addrlen)
{
	const int fd = get_socket_fd(qd);
	return bind(fd, addr, addrlen);
}

int dpoll_connect(int qd, const struct sockaddr *addr, socklen_t size)
{
	const int fd = get_socket_fd(qd);
	return connect(fd, addr, size);
}

int dpoll_accept(int qd, struct sockaddr *restrict addr,
                 socklen_t *restrict addrlen)
{
	const int fd = get_socket_fd(qd);
	int other = accept(fd, addr, addrlen);
	if (other == -1)
		return -1;
	return other + DPOLL_SOCKET_OFFSET;
}

int dpoll_listen(int qd, int backlog)
{
	const int fd = get_socket_fd(qd);
	return listen(fd, backlog);
}

int dpoll_getsockname(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	const int fd = get_socket_fd(qd);
	return getsockname(fd, addr, addrlen);
}

int dpoll_setsockopt(int qd, int level, int optname, const void *optval,
                     socklen_t optlen)
{
	const int fd = get_socket_fd(qd);
	return setsockopt(fd, level, optname, optval, optlen);
}

ssize_t dpoll_send(int qd, const void *buf, size_t len)
{
	const int fd = get_socket_fd(qd);
	return send(fd, buf, len, 0);
}

ssize_t dpoll_sendmsg(int qd, const struct msghdr *msg, int flags)
{
	const int fd = get_socket_fd(qd);
	return sendmsg(fd, msg, flags);
}

ssize_t dpoll_recv(int qd, void *buf, size_t len)
{
	const int fd = get_socket_fd(qd);
	return recv(fd, buf, len, 0);
}

ssize_t dpoll_recvmsg(int qd, struct msghdr *msg, int flags)
{
	const int fd = get_socket_fd(qd);
	return recvmsg(fd, msg, flags);
}

int dpoll_close(int qd)
{
	const int fd = get_socket_fd(qd);
	return close(fd);
}

void debug_print(void)
{
}
