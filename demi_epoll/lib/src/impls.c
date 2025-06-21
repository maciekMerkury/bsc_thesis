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

typedef socket_t *socket_ptr;

BUFFER_DEF(soc_buf, socket_ptr, soc_buf)
BUFFER_DEF(epoll_buf, epoll_t, epoll_buf)

uint32_t available_events(const epoll_item_t *it)
{
	const socket_t *soc = it->soc;
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
	list_elem_t *delete_list = NULL;
	RB_FOREACH(it, epoll_head, &ep->items) {
		demi_log("looking at %u\n", it->soc->qd);
		if (!it->soc->open) {
			demi_log("it's not open\n");
			assert(list_is_empty(&it->ready_list_entry));
			list_add_to_head(&delete_list, &it->ready_list_entry);
			// never schedule a closed socket
			continue;
		}
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

		socket_t *soc = it->soc;
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
			demi_log("waiting for EPOLLIN on %u with tok: %lu\n",
			         soc->qd, soc->recv.base.tok);
		}
		if (rem & EPOLLOUT) {
			assert(soc->send.base.pending);
			toks[tok_count++] = soc->send.base.tok;
			demi_log("waiting for EPOLLOUT on %u with tok: %lu\n",
			         soc->qd, soc->send.base.tok);
		}
	}

	if (delete_list) {
		it = container_of(delete_list, epoll_item_t, ready_list_entry);

		do {
			epoll_item_t *tmp = it;
			demi_log("removing %u from epoll tree\n", it->soc->qd);
			it = container_of(delete_list, epoll_item_t,
			                  ready_list_entry);
			RB_REMOVE(epoll_head, &ep->items, tmp);
			socket_close(tmp->soc);
			free(tmp);
		} while (it != container_of(delete_list, epoll_item_t,
		                            ready_list_entry));
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
	socket_t *soc = socket_init();
	if (!soc)
		goto err_close_soc;

	*soc_buf_get(fd) = soc;
	return fd;
err_close_soc:
	soc_buf_free(fd);
	return -1;
}

int dpoll_bind_impl(int qd, const struct sockaddr *addr, socklen_t addrlen)
{
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);
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
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);
	int ret = demi_listen(soc->qd, backlog);
	DEMI_ERR(ret, "listen\n");
	soc->recv_off = -1;
	return 0;
}

int dpoll_accept_impl(int qd, struct sockaddr *addr, socklen_t *addrlen)
{
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);

	assert(soc->recv_off == -1);
	struct sockaddr_in ad;
	demi_result_t ret = maybe_accept(soc, &ad);
	if (!result_is_ok(ret))
		return -1;
	int fd = dpoll_socket_impl();
	assert(fd >= 0);

	socket_t *new_soc = *soc_buf_get(fd);
	new_soc->qd = soc_from_result(ret);
	new_soc->addr = ad;

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
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);
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
		ep_close(ep);
		epoll_buf_free(qd);
		return 0;
	}

	qd = get_socket_fd(qd);
	socket_t *soc = *soc_buf_get(qd);
	demi_log("closing %u\n", soc->qd);
	soc->open = false;
	socket_close(soc);
	// soc_buf_free(qd);
	return 0;
}

int dpoll_create_impl(int flags)
{
	int fd = epoll_buf_next();
	int ret = ep_init(&epoll_buf.items[fd].it, flags);
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
	int ret;
	if (!qd_is_dpoll(fd)) {
		// fd must be processed by linux' epoll
		ret = epoll_ctl(ep->epollfd, op, fd, event);
		goto defer;
	}

	const int socfd = get_socket_fd(fd);
	socket_t *soc = *soc_buf_get(socfd);
	ret = ep_ctl(ep, op, socfd, soc, event);

defer:
	return ret;
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
	if (tokens_len == 1)
		demi_log("waiting on token %lu\n", tokens[0]);
	if (tokens_len == 0) {
		epoll_timeout = timeout;
		goto add_epoll_events;
	}
	if (ep->ready_list) {
		demi_log("ready list is not empty, so not going to wait\n");
		timeout = 0; // we already have some events ready, just poll
	}

	const struct timespec ts = ms_timeout_to_timespec(timeout);
	demi_qresult_t res;
	int offset;
	int ret = demi_wait_any(&res, &offset, tokens, tokens_len,
	                        (timeout >= 0) ? &ts : NULL);
	if (ret == ETIMEDOUT)
		goto add_epoll_events;

	if (ret != 0) {
		demi_log("%s: %s\nsearched for: \n", __func__, strerror(ret));
		for (size_t i = 0; i < tokens_len; ++i) {
			demi_log("%lu\n", tokens[i]);
		}
	}
	assert(ret == 0);
	demi_log("looking for %u because %lu\n", res.qr_qd, res.qr_qt);
	epoll_item_t *it = ep_find_item(ep, res.qr_qd);
	if (!it) {
		demi_log("did not find it, here's the tree in some order\n");
		RB_FOREACH(it, epoll_head, &ep->items) {
			demi_log("in the tree: %u\n", it->soc->qd);
		}
		goto add_epoll_events;
	}

	demi_log("found %u\n", it->soc->qd);
	assert(res.qr_qd == it->soc->qd);
	socket_handle_event(it->soc, &res);
	if (!list_contains_elem(ep->ready_list, &it->ready_list_entry))
		list_add_to_head(&ep->ready_list,
		                 &it->ready_list_entry);

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
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);
	return maybe_write(soc, buf, count);
}

ssize_t dpoll_read_impl(int qd, void *buf, size_t len)
{
	socket_t *soc = *soc_buf_get(qd);
	demi_log("%p\n", soc);
	assert(soc->open);
	assert(!socket_is_accepting(soc));
	return maybe_read(soc, buf, len);
}

ssize_t dpoll_readv_impl(int qd, struct iovec *iov, int iovcnt)
{
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);
	assert(!socket_is_accepting(soc));
	return maybe_readv(soc, iov, iovcnt);
}

ssize_t dpoll_writev_impl(int qd, const struct iovec *iov, int iovcnt)
{
	socket_t *soc = *soc_buf_get(qd);
	assert(soc->open);
	return maybe_writev(soc, iov, iovcnt);
}
