#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static int log_enabled = 0;

static bool env_is_trace(const char *const env_name)
{
	const char *env = getenv(env_name);
	return (env && strcmp(env, "trace") == 0);
}

void demi_log_init(void)
{
	log_enabled = env_is_trace("RUST_LOG") ||
	              env_is_trace("DEMI_EPOLL_LOG");
}

void demi_log(const char *const format, ...)
{
	if (!log_enabled)
		return;

	va_list args = { 0 };
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
