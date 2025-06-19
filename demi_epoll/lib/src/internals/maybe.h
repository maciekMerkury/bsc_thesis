#pragma once

#include <demi/types.h>

struct maybe_prefix {
	demi_qtoken_t tok;
	_Bool pending;
	int ret;
};

#define MAYBE_DEF(type, name) struct name { struct maybe_prefix base; type elem; };
