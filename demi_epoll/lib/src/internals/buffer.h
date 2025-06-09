#pragma once

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>

#define increment_index(curr, max_size) do { (curr) = ((curr) + 1) % (max_size) } while (0)

// poor man's template
#define CB_DEF(name, type, max_size)	\
	struct name {	\
		type items[(max_size)];	\
		size_t head;		\
		size_t next;		\
	}				\
	inline _Bool name ## _is_full(const name *cb) { return cb->next >= (max_size); } \
	inline _Bool name ## _is_empty(const name *cb) { return cb->next == cb->head; }	\
	/* TODO: fix this, the increment should wrap around the max_size */		\
	inline void name ## _push(name *cb, type item) { assert(!(name ## _is_full(cb))); cb->items[cb->next] = item; increment_index(cb->next, max_size); }	\
	inline type name ## _pop(name *cb) { assert(!name ## _is_empty(cb)); type tmp = cb->items[cb->head]; increment_index(cb->head, max_size); return tmp; }
