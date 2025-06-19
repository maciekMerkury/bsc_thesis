#include "impls.h"
#include "impls.h"
#include "internals/buffer.h"
#include "log.h"
#include "socket_wrapper.h"
#include "utils.h"
#include <demi/libos.h>
#include <demi/wait.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "epoll_wrapper.h"

BUFFER_DEF(soc_buf, socket_t, qd, soc_buf)
BUFFER_DEF(epoll_buf, epoll_t, epollfd, epoll_buf)

uint32_t available_events(const epoll_item_t *it)
{
	const socket_t *soc = soc_buf_get(it->soc_idx);
	return check_event(it->subevs, EPOLLIN,
	                   socket_is_accepting(soc) ? socket_can_accept(soc) :
	                   socket_can_read(soc)) ||
	       check_event(it->subevs, EPOLLOUT, socket_can_write(soc));
}

/// a small helpful macro for making sure that a call fail due to EWOULDBLOCK
#define schedule(_func) do {	\
	int _ret = _func;	\
	assert(_ret < 0 && errno == EWOULDBLOCK); \
	} while (0)

/// iterates over all items in `ep->items` and adds them to the readylist if at
/// least one event is set, and schedules all other uncompleted events
static size_t check_and_schedule_evs(epoll_t *ep, demi_qtoken_t **toks_dest)
{
	epoll_item_t *it;
	size_t tok_count = 0;
	demi_qtoken_t *toks = NULL;
	RB_FOREACH(it, epoll_head, &ep->items) {
		const uint32_t avs = available_events(it);
		if (avs != 0) {
			list_add_to_head(&ep->ready_list,
			                 &it->ready_list_entry);
		}
		const uint32_t rem = avs ^ it->subevs;
		if (rem == 0)
			// no more events to process
			continue;

		const int schedule_count = __builtin_popcount(rem);
		toks = realloc(
			toks, (tok_count + schedule_count) * sizeof(toks[0]));
		assert(toks);

		socket_t *soc = soc_buf_get(it->soc_idx);
		verify_events(rem);
		if (rem & EPOLLIN) {
			if (!soc->recv.base.pending) {
				if (socket_is_accepting(soc)) {
					schedule(maybe_accept(soc, NULL));
				} else {
					schedule(maybe_read(soc, NULL,
						DPOLL_DEFAULT_READ_SIZE));
				}
			}
			assert(soc->recv.base.pending);
			toks[tok_count++] = soc->recv.base.tok;
		}
		if (rem & EPOLLOUT) {
			assert(soc->send.base.pending);
			toks[tok_count++] = soc->send.base.tok;
		}
	}

	*toks_dest = toks;
	return tok_count;
}

void dpoll_init(void)
{
	struct demi_args args = {
		.argc = 0,
		.argv = NULL,
	};

	assert(demi_init(&args) == 0);
	demi_log_init();
}

int dpoll_socket_impl(void)
{
	int fd = soc_buf_next();
	int ret = socket_init(soc_buf.items + fd);
	if (ret < 0)
		goto err_close_soc;
	return fd;
err_close_soc:
	soc_buf_free(fd);
	return -1;
}

int dpoll_bind_impl(int qd, const struct sockaddr *addr, socklen_t addrlen)
{
	socket_t *soc = soc_buf_get(qd);
	struct sockaddr_in *a = (void *)addr;
	if (a->sin_addr.s_addr == 0) {
		demi_log(
			"addr cannot be 0.0.0.0, for some reason demikernel does not support this\n");
	}
	int ret = demi_bind(soc->qd, addr, addrlen);
	assert(addrlen == sizeof(soc->addr));
	memcpy(&soc->addr, addr, addrlen);
	DEMI_ERR(ret, "binding\n");
	return 0;
}

int dpoll_connect_impl(int qd, const struct sockaddr *addr, socklen_t size)
{
	UNIMPLEMENTED();
}

int dpoll_listen_impl(int qd, int backlog)
{
	socket_t *soc = soc_buf_get(qd);
	int ret = demi_listen(soc->qd, backlog);
	DEMI_ERR(ret, "listen\n");
	soc->recv_off = -1;
	return 0;
}

int dpoll_accept_impl(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	socket_t *soc = soc_buf_get(qd);
	assert(soc->recv_off == -1);
	struct sockaddr_in ad;
	int ret = maybe_accept(soc, &ad);
	if (ret < 0)
		return -1;
	int fd = dpoll_socket_impl();
	socket_t *new_soc = soc_buf_get(fd);
	*new_soc = (socket_t){
		.qd = ret,
		.addr = ad,
	};
	if (addr) {
		const size_t size = sizeof(ad);
		assert(*addrlen >= size);
		memcpy(addr, &ad, size);
		*addrlen = size;
	}

	return fd;
}

int dpoll_getsockname_impl(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	socket_t *soc = soc_buf_get(qd);
	if (soc->addr.sin_family != AF_INET) {
		demi_log("getsockname failed with family: %d\n",
		         soc->addr.sin_family);
		errno = ENOTSOCK;
		return -1;
	}
	return 0;
}

int dpoll_setsockopt_impl(int qd, int level, int optname, const void *optval,
                          socklen_t optlen)
{
	demi_log("qd: %d, level: %d, optname: %d\n", qd, level, optname);
	return 0;
}

ssize_t dpoll_sendmsg_impl(int qd, const struct msghdr *msg, int flags)
{
	UNIMPLEMENTED();
}

ssize_t dpoll_recvmsg_impl(int qd, struct msghdr *msg, int flags)
{
	UNIMPLEMENTED();
}

int dpoll_close_impl(int qd)
{
	if (qd_is_epoll(qd)) {
		qd = get_epoll_fd(qd);
		epoll_t *ep = epoll_buf_get(qd);
		ep_destroy(ep);
		epoll_buf_free(qd);
		return 0;
	}

	qd = get_socket_fd(qd);
	socket_destroy(soc_buf_get(qd));
	soc_buf_free(qd);
	return 0;
}

int dpoll_create_impl(int flags)
{
	int fd = epoll_buf_next();
	int ret = ep_init(epoll_buf.items + fd, flags);
	if (ret < 0)
		goto err;
	return fd;
err:
	epoll_buf_free(fd);
	return -1;
}

int dpoll_ctl_impl(int dpollfd, int op, int fd, struct epoll_event *event)
{
	epoll_t *ep = epoll_buf_get(dpollfd);
	if (!qd_is_dpoll(fd)) {
		// fd must be processed by linux' epoll
		return epoll_ctl(ep->epollfd, op, fd, event);
	}

	const int socfd = get_socket_fd(fd);
	const socket_t *soc = soc_buf_get(socfd);
	return ep_ctl(ep, op, socfd, soc, event);
}

int dpoll_pwait_impl(int dpollfd, struct epoll_event *events, int maxevents,
                     int timeout, const sigset_t *sigmask)
{
	epoll_t *ep = epoll_buf_get(dpollfd);
	int epoll_timeout = 0;
	demi_log("%s: sigmask is not used atm\n", __func__);
	// TODO: keep track of the maximum amount of qtokens required, and store the qtoken buffer to limit the allocations
	demi_qtoken_t *tokens = NULL;
	const size_t tokens_len = check_and_schedule_evs(ep, &tokens);
	demi_log("waiting on %lu tokens\n", tokens_len);
	if (tokens_len == 0) {
		epoll_timeout = timeout;
		goto add_epoll_events;
	}
	if (ep->ready_list)
		timeout = 0; // we already have some events ready, just poll

	const struct timespec ts = ms_timeout_to_timespec(timeout);
	demi_qresult_t res;
	int offset;
	int ret = demi_wait_any(&res, &offset, tokens, tokens_len,
	                        (timeout >= 0) ? &ts : NULL);
	if (ret == ETIMEDOUT) {
		goto add_epoll_events;
	}
	assert(ret == 0);
	demi_log("looking for %d\n", res.qr_qd);
	epoll_item_t *it = ep_find_item(ep, res.qr_qd);
	assert(it);
	socket_handle_event(soc_buf_get(it->soc_idx), &res);
	if (!list_contains_elem(ep->ready_list, &it->ready_list_entry))
		list_add_to_head(&ep->ready_list, &it->ready_list_entry);

add_epoll_events:
	int events_added = ep_drain_ready_list(ep, events, maxevents);
	assert(events_added <= maxevents);

	if (maxevents - events_added > 0) {
		ret = epoll_pwait(ep->epollfd, events + events_added,
		                  maxevents - events_added, epoll_timeout,
		                  sigmask);
		if (ret < 0) {
			perror("epoll_wait");
			assert(errno == ETIMEDOUT);
			goto cleanup;
		}
		events_added += ret;
	}
	ret = events_added;

cleanup:
	free(tokens);
	return ret;
}

void debug_print(void)
{
}

ssize_t dpoll_write_impl(int qd, const void *buf, size_t count)
{
	socket_t *soc = soc_buf_get(qd);
	return maybe_write(soc, buf, count);
}

ssize_t dpoll_read_impl(int qd, void *buf, size_t len)
{
	socket_t *soc = soc_buf_get(qd);
	assert(soc->recv_off > -1);
	return maybe_read(soc, buf, len);
}

ssize_t dpoll_readv_impl(int qd, const struct iovec *iov, int iovcnt)
{
	UNIMPLEMENTED();
}

ssize_t dpoll_writev_impl(int qd, const struct iovec *iov, int iovcnt)
{
	UNIMPLEMENTED();
}
