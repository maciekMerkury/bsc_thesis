#pragma once

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>

#define CB_DEF(name, type, max_size)	\
	struct name {	\
		type items[(max_size)];	\
		size_t head;		\
		size_t next;		\
	}				\
	inline _Bool name ## _is_full(const name *cb) { return cb->next >= (max_size); } \
	inline _Bool name ## _is_empty(const name *cb) { return cb->next == cb->head; }	\
	/* TODO: fix this, the increment should wrap around the max_size */		\
	inline void name ## _push(name *cb, type item) { assert(!(name ## _is_full(cb))); cb->items[cb->next++] = item; }


#define BUF_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define CB_IS_FULL(cb_ptr) ((cb_ptr)->next < BUF_SIZE((cb_ptr)->items))

#define CB_PUSH(cb_ptr, item) do {	\
	assert(!CB_IS_FULL((cb_ptr))); \
	(cb_ptr)->items[(cb_ptr)->next++] = item; \
	while (0)



