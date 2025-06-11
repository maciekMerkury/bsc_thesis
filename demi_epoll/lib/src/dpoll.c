#include "impls.h"
#include "dpoll.h"
#include <stdlib.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#include <sys/socket.h>
#include <execinfo.h>
#include <sys/uio.h>

#define BRUH(call) do { demi_log("calling %s\n", #call); int _ret = call; if (_ret < 0) { demi_log("call failed: %s\n", strerror(errno)); }; return _ret; } while (0)

void debug_print(void)
{
}

int dpoll_socket_impl(int domain, int type, int protocol)
{
	demi_log("PLEASE FUCKING PRINT\n");
	BRUH(socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol));
}

int dpoll_bind_impl(int qd, const struct sockaddr *addr, socklen_t addrlen)
{
	BRUH(bind(qd, addr, addrlen));
}

int dpoll_connect_impl(int qd, const struct sockaddr *addr, socklen_t size)
{
	BRUH(connect(qd, addr, size));
}

int dpoll_accept_impl(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	BRUH(accept(qd, addr, addrlen));
}

int dpoll_listen_impl(int qd, int backlog)
{
	BRUH(listen(qd, backlog));
}

/// the next 2 functions are not really that supported
int dpoll_getsockname_impl(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	BRUH(getsockname(qd, addr, addrlen));
}

int dpoll_setsockopt_impl(int qd, int level, int optname, const void *optval,
                          socklen_t optlen)
{
	BRUH(setsockopt(qd, level, optname, optval, optlen));
}

ssize_t dpoll_sendmsg_impl(int qd, const struct msghdr *msg, int flags)
{
	BRUH(sendmsg(qd, msg, flags));
}

ssize_t dpoll_recvmsg_impl(int qd, struct msghdr *msg, int flags)
{
	BRUH(recvmsg(qd, msg, flags));
}

int dpoll_close_impl(int qd)
{
	if (qd_is_epoll(qd))
		qd = get_epoll_fd(qd);
	else
		qd = get_socket_fd(qd);
	BRUH(close(qd));
}

int dpoll_create_impl(int flags)
{
	int ret = epoll_create(flags);
	demi_log("created %d\n", ret);
	return ret;
}

static int counter = 0;

static const char *get_op(int op)
{
	switch (op) {
	case EPOLL_CTL_ADD:
		return "ADD";
	case EPOLL_CTL_DEL:
		return "DEL";
	case EPOLL_CTL_MOD:
		return "MOD";
	default:
		return NULL;
	}
}

int dpoll_ctl_impl(int dpollfd, int op, int fd, struct epoll_event *event)
{
	// *multiplexing*
	if (qd_is_dpoll(fd))
		fd = get_socket_fd(fd);
	demi_log("[%d]: %s %d\n", dpollfd, get_op(op), fd);
	if (dpollfd == 13 && op == EPOLL_CTL_ADD)
		++counter;
	BRUH(epoll_ctl(dpollfd, op, fd, event));
}

static int debug_num = 0;

int dpoll_pwait_impl(int dpollfd, struct epoll_event *events, int maxevents,
                     int timeout, const sigset_t *sigmask)
{
	demi_log("%d is waiting for %d\n", dpollfd, timeout);
	int ret = epoll_pwait(dpollfd, events, maxevents, timeout, sigmask);
	demi_log("epoll_pwait returned %d\n", ret);
	if (dpollfd == 13 && ret == 0) {
		++debug_num;
		if (debug_num > 100) {
			demi_log("bruh sus amogus %d\n", debug_num);
			void *buffer[100];
			int ntraces = backtrace(buffer, 100);
			demi_log("backtrace returned %d\n", ntraces);
			char **bruh = backtrace_symbols(buffer, ntraces);
			for (int i = 0; i < ntraces; ++i) {
				demi_log("%s\n", bruh[i]);
			}
			free(bruh);
			demi_log("please brother work\n");
			return -1;
		}
	} else {
		debug_num = 0;
	}
	return ret;
}

ssize_t dpoll_write_impl(int qd, const void *buf, size_t count)
{
	BRUH(write(qd, buf, count));
}

ssize_t dpoll_read_impl(int qd, void *buf, size_t count)
{
	BRUH(read(qd, buf, count));
}

ssize_t dpoll_readv_impl(int qd, const struct iovec *iov, int iovcnt)
{
	BRUH(readv(qd, iov, iovcnt));
}

ssize_t dpoll_writev_impl(int qd, const struct iovec *iov, int iovcnt)
{
	BRUH(writev(qd, iov, iovcnt));
}
