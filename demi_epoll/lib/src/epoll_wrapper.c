#include "epoll_wrapper.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <demi/wait.h>

#include "impls.h"
#include "log.h"
#include "utils.h"

RB_GENERATE(epoll_head, epoll_item, tree, compare_items);

int ep_init(epoll_t *ep, int flags)
{
	int epollfd = epoll_create1(flags);
	if (epollfd < 0)
		return -1;

	memset(ep, 0, sizeof(*ep));
	*ep = (epoll_t){
		.epollfd = epollfd,
		.items = RB_INITIALIZER(),
		.qtokens_len = DPOLL_DEFAULT_QTOKEN_LEN,
		.ready_list = NULL,
		.qtokens = calloc(
			DPOLL_DEFAULT_QTOKEN_LEN, sizeof(ep->qtokens[0])),
	};
	assert(ep->qtokens);

	return 0;
}

void ep_destroy(epoll_t *ep)
{
	close(ep->epollfd);
	free(ep->qtokens);
	epoll_item_t *it;
	RB_FOREACH(it, epoll_head, &ep->items) {
		free(it->tree.rbe_parent);
	}
}

static int epoll_add(epoll_t *ep, const socket_t *soc,
                     int fd, struct epoll_event *ev)
{
	// TODO: think if we should actually check if the entry is already present
	verify_events(ev->events);

	epoll_item_t *it = calloc(1, sizeof(*it));
	assert(it);
	*it = (epoll_item_t){
		.soc_idx = fd,
		.demi_qd = soc->qd,
		.subevs = ev->events,
		.data = ev->data,
	};
	LIST_HEAD_INIT(&it->ready_list_entry);

	RB_INSERT(epoll_head, &ep->items, it);
	return 0;
}

static int epoll_del(epoll_t *ep, int qd)
{
	epoll_item_t *it = ep_find_item(ep, qd);
	if (!it)
		return -1;
	RB_REMOVE(epoll_head, &ep->items, it);
	if (!list_is_empty(&it->ready_list_entry))
		list_remove(&it->ready_list_entry);
	free(it);
	return 0;
}

static int epoll_mod(epoll_t *ep, int qd, const struct epoll_event *ev)
{
	epoll_item_t *it = ep_find_item(ep, qd);
	if (!it)
		return -1;
	verify_events(ev->events);
	it->subevs = ev->events;
	return 0;
}


int ep_ctl(epoll_t *ep, int op, int fd, const socket_t *soc,
           struct epoll_event *ev)
{
	switch (op) {
	case EPOLL_CTL_ADD:
		return epoll_add(ep, soc, fd, ev);
	case EPOLL_CTL_DEL:
		return epoll_del(ep, soc->qd);
	case EPOLL_CTL_MOD:
		return epoll_mod(ep, soc->qd, ev);

	default:
		abort();
	}
}

#define next(_item_ptr) container_of((_item_ptr)->ready_list_entry.next, epoll_item_t, ready_list_entry)

/// brief adds as many events from `ep->ready_list_head` into the events as possible
///
/// returns number of events added
int ep_drain_ready_list(epoll_t *ep, struct epoll_event *evs, int evs_size)
{
	if (evs_size == 0 || ep->ready_list == NULL)
		return 0;

	int events_idx = 0;
	epoll_item_t *it = container_of(ep->ready_list, epoll_item_t,
	                                ready_list_entry);

	// make the list non-cyclic
	// ep->ready_list->prev->next = NULL;
	// ep->ready_list->prev = NULL;
	// ep->ready_list = NULL;

	// TODO: finish this code
	while (events_idx < evs_size) {
		evs[events_idx++] = (struct epoll_event){
			.events = available_events(it),
			.data = it->data,
		};
		if (list_is_empty(&it->ready_list_entry)) {
			ep->ready_list = NULL;
			break;
		}
		list_remove(&it->ready_list_entry);
		it = next(it);
	}
	// overwrite the readylist if it's not empty
	if (ep->ready_list)
		ep->ready_list = &it->ready_list_entry;

	return events_idx;
}

demi_qresult_t ep_wait(const epoll_t *ep, const struct timespec *timeout,
                       demi_qtoken_t *toks, size_t toks_size)
{
	int _;
	demi_qresult_t res;
	int ret = demi_wait_any(&res, &_, toks, toks_size, timeout);
	assert(ret == 0 || ret == ETIMEDOUT);
	if (ret == ETIMEDOUT) {
		errno = ETIMEDOUT;
		res.qr_qd = -1;
		return res;
	}
	return res;
}

epoll_item_t *ep_find_item(epoll_t *ep, int qd)
{
	epoll_item_t search = { .demi_qd = qd };
	epoll_item_t *it = RB_FIND(epoll_head, &ep->items, &search);
	if (!it) {
		errno = ENOENT;
		return NULL;
	}
	return it;
}

