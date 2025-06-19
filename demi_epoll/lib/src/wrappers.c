#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#include "impls.h"
#include "log.h"
#include "sockets.h"

static inline int maybe_add(int ret, int off)
{
	return ret > -1 ? ret + off : -1;
}

int dpoll_epoll_create(int flags)
{
	return maybe_add(dpoll_create_impl(flags), DPOLL_EPOLL_OFFSET);
}

int dpoll_epoll_ctl(int dpollfd, int op, int fd, struct epoll_event *event)
{
	assert(qd_is_dpoll(dpollfd));
	return dpoll_ctl_impl(get_epoll_fd(dpollfd), op, fd,
	                      event);
}

int dpoll_epoll_pwait(int dpollfd, struct epoll_event *events, int maxevents,
                      int timeout, const sigset_t *sigmask)
{
	assert(qd_is_dpoll(dpollfd));
	return dpoll_pwait_impl(get_epoll_fd(dpollfd), events, maxevents,
	                        timeout, sigmask);
}

int dpoll_socket(int domain, int type, int protocol)
{
	demi_log("domain: %d, type: %d\n", domain, type);
	if (domain == AF_INET6) {
		demi_log("domain requested is IPV4, we do not support this\n");
		abort();
	}
	int fd;
	if (domain == AF_INET && type == SOCK_STREAM)
		fd = maybe_add(dpoll_socket_impl(), DPOLL_SOCKET_OFFSET);
	else
		fd = socket(domain, type, protocol);
	demi_log("socket: %d\n", fd);
	return fd;
}

int dpoll_bind(int qd, const struct sockaddr *addr, socklen_t addrlen)
{
	if (qd_is_dpoll(qd))
		return dpoll_bind_impl(get_socket_fd(qd), addr, addrlen);
	return bind(qd, addr, addrlen);
}

int dpoll_connect(int qd, const struct sockaddr *addr, socklen_t size)
{
	if (qd_is_dpoll(qd))
		return dpoll_connect_impl(get_socket_fd(qd), addr, size);
	return connect(qd, addr, size);
}

int dpoll_accept(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	if (qd_is_dpoll(qd))
		return maybe_add(
			dpoll_accept_impl(get_socket_fd(qd), addr, addrlen),
			DPOLL_SOCKET_OFFSET);
	return accept(qd, addr, addrlen);
}

int dpoll_listen(int qd, int backlog)
{
	if (qd_is_dpoll(qd))
		return dpoll_listen_impl(get_socket_fd(qd), backlog);
	return listen(qd, backlog);
}

int dpoll_getsockname(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	if (qd_is_dpoll(qd))
		return dpoll_getsockname_impl(get_socket_fd(qd), addr, addrlen);
	return getsockname(qd, addr, addrlen);
}

int dpoll_setsockopt(int qd, int level, int optname, const void *optval,
                     socklen_t optlen)
{
	if (qd_is_dpoll(qd))
		return dpoll_setsockopt_impl(get_socket_fd(qd), level, optname,
		                             optval,
		                             optlen);
	return setsockopt(qd, level, optname, optval, optlen);
}

ssize_t dpoll_sendmsg(int qd, const struct msghdr *msg, int flags)
{
	if (qd_is_dpoll(qd))
		return dpoll_sendmsg_impl(get_socket_fd(qd), msg, flags);
	return sendmsg(qd, msg, flags);
}

ssize_t dpoll_recvmsg(int qd, struct msghdr *msg, int flags)
{
	if (qd_is_dpoll(qd))
		return dpoll_recvmsg_impl(get_socket_fd(qd), msg, flags);
	return recvmsg(qd, msg, flags);
}

int dpoll_close(int qd)
{
	if (qd_is_dpoll(qd))
		return dpoll_close_impl(qd);
	return close(qd);
}

ssize_t dpoll_write(int qd, const void *buf, size_t count)
{
	if (qd_is_dpoll(qd))
		return dpoll_write_impl(get_socket_fd(qd), buf, count);
	return write(qd, buf, count);
}

ssize_t dpoll_read(int qd, void *buf, size_t count)
{
	if (qd_is_dpoll(qd))
		return dpoll_read_impl(get_socket_fd(qd), buf, count);
	return read(qd, buf, count);
}

ssize_t dpoll_readv(int qd, const struct iovec *iov, int iovcnt)
{
	if (qd_is_dpoll(qd))
		return dpoll_readv_impl(get_socket_fd(qd), iov, iovcnt);
	return readv(qd, iov, iovcnt);
}

ssize_t dpoll_writev(int qd, const struct iovec *iov, int iovcnt)
{
	if (qd_is_dpoll(qd))
		return dpoll_writev_impl(get_socket_fd(qd), iov, iovcnt);
	return writev(qd, iov, iovcnt);
}
