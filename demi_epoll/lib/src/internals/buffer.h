#pragma once

#include <stddef.h>
#include <assert.h>
#include <stdlib.h>


#define BUFFER_DEF(name, type, free_var_name, buf_name)	\
	typedef struct name {		\
		type *items;	\
		size_t size;	\
		int next_free;	\
	} name ## _t;			\
	static struct name buf_name = { .next_free = -1 }; \
	static int name ## _next(void) {	\
		if (buf_name.next_free > -1) {	\
			int fd = buf_name.next_free;\
			buf_name.next_free = buf_name.items[fd].free_var_name;\
			return fd;		\
		}				\
		const size_t size = sizeof(type) * (++buf_name.size);	\
		buf_name.items = realloc(buf_name.items, size);		\
		assert(buf_name.items);		\
		return buf_name.size - 1;		\
	}					\
	static void name ## _free(int fd) {	\
		assert(fd < buf_name.size);		\
		type *t = buf_name.items + fd;	\
		if (buf_name.next_free) {		\
			buf_name.next_free = fd;	\
			t->free_var_name = -1;	\
		} else {			\
			t->free_var_name = buf_name.next_free;\
			buf_name.next_free = fd;	\
		}				\
	}					\
	static inline type * name ## _get(int fd) {	\
		assert(fd < buf_name.size);	\
		return buf_name.items + fd;	\
	}
