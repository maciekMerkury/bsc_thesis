#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * all of this is needed because despite what the type `int` might suggest,
 * a demi socket qd is actually an u32, so negative values are considered correct (thanks microsoft)
 */

typedef uint32_t demi_socket_t;
typedef int64_t demi_result_t;

static inline bool result_is_ok(demi_result_t res)
{
	return res >= 0;
}

static inline demi_result_t result_from_soc(demi_socket_t soc)
{
	return (demi_result_t)soc;
}

static inline demi_socket_t soc_from_result(demi_result_t res)
{
	assert(result_is_ok(res));
	return (demi_socket_t)res;
}
