#pragma once

#include "internals/tree.h"
#include <sys/epoll.h>
#include "socket_wrapper.h"
#include "internals/list.h"

#define DPOLL_DEFAULT_QTOKEN_LEN 32

/// this is quite bad, but i cant think of a different way of doing this
#define DPOLL_DEFAULT_READ_SIZE 1024

typedef struct epoll_item {
	RB_ENTRY(epoll_item) tree;

	list_elem_t ready_list_entry;
	uint32_t subevs;
	int soc_idx;

	// TODO: figure out a better way of doing this
	int demi_qd;
	epoll_data_t data;
} epoll_item_t;

typedef struct epoll {
	RB_HEAD(epoll_head, epoll_item) items;

	demi_qtoken_t *qtokens;
	size_t qtokens_len;
	list_elem_t *ready_list;
	int epollfd;
} epoll_t;

int ep_init(epoll_t *ep, int flags);
void ep_destroy(epoll_t *ep);
// this is not really ergonomic
int ep_ctl(epoll_t *ep, int op, int fd, const socket_t *soc,
           struct epoll_event *ev);
/// `res.qr_qd` is equal to -1 on timeout, and errno is set
demi_qresult_t ep_wait(const epoll_t *ep, const struct timespec *timeout,
                       demi_qtoken_t *toks, size_t tok_size);

epoll_item_t *ep_find_item(epoll_t *ep, int qd);

/// tries to add all events from `ev->read_list_head` into the array
///
/// returns the number of events added
int ep_drain_ready_list(epoll_t *ep, struct epoll_event *evs, int evs_size);

static inline int compare_items(const epoll_item_t *left,
                                const epoll_item_t *right)
{
	return left->demi_qd - right->demi_qd;
}

RB_PROTOTYPE(epoll_head, epoll_item, tree, compare_items);

