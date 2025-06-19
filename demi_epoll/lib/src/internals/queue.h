#pragma once

#include <stddef.h>
#include <assert.h>

#define increment_index(curr, max_size) do { (curr) = ((curr) + 1) % (max_size) } while (0)

// poor man's template
#define CQ_DEF(name, type, max_size)	\
	struct name {	\
		type items[(max_size)];	\
		size_t head;		\
		size_t next;		\
	};				\
	inline _Bool name ## _is_full(const name *cq) { return cq->next >= (max_size); } \
	inline _Bool name ## _is_empty(const name *cq) { return cq->next == cq->head; }	\
	/* TODO: fix this, the increment should wrap around the max_size */		\
	inline void name ## _push(name *cq, type item) { assert(!(name ## _is_full(cq))); cb->items[cq->next] = item; increment_index(cq->next, max_size); }	\
	inline type name ## _pop(name *cq) { assert(!name ## _is_empty(cq)); type tmp = cq->items[cq->head]; increment_index(cq->head, max_size); return tmp; }
