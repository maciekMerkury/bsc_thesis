#pragma once

#include <errno.h>
#include <stdlib.h>
#include "log.h"
#include <demi/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>

#define DEMI_ERR(_ret, msg, ...) do { if (_ret != 0) { errno = _ret; demi_log(msg, ##__VA_ARGS__); return -1; } } while(0)

#define GIVE_UP(msg, ...) do { demi_log(msg, ##__VA_ARGS__); abort(); } while(0)

#define UNIMPLEMENTED() do { demi_log("%s is not implemented\n", __func__); abort(); } while(0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define check_event(_subev, _epoll, _func) ((((_subev) & (_epoll)) && (_func)) ? (_epoll) : 0)

static inline void verify_events(uint32_t events)
{
	if (events & ~(EPOLLIN | EPOLLOUT)) {
		GIVE_UP("not supported events requested: 0b%b\n", events);
	}
}

/// atm it will panic if the sga cannot fit the entire buf
size_t copy_buf_into_sga(const void *buf, size_t len, demi_sgarray_t *sga);

bool copy_sga_into_buf(void *buf, size_t buf_len, const demi_sgarray_t *sga,
                       ssize_t *offset);

struct timespec ms_timeout_to_timespec(int ms_timeout);
