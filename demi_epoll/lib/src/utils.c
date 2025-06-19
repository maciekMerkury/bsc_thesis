#include "utils.h"

#include <assert.h>
#include <stddef.h>
#include <sys/param.h>
#include <string.h>

size_t copy_buf_into_sga(const void *buf, size_t len, demi_sgarray_t *sga)
{
	size_t copied = 0;
	size_t seg = 0;
	while (copied < len) {
		const demi_sgaseg_t s = sga->sga_segs[seg++];
		const size_t diff = MIN(len - copied, s.sgaseg_len);
		memcpy(s.sgaseg_buf, buf + copied, diff);
		copied += diff;
		assert(seg <= sga->sga_numsegs);
	}
	return copied;
}

bool copy_sga_into_buf(void *buf, size_t buf_len, const demi_sgarray_t *sga,
                       ssize_t *offset)
{
	if (!buf || !sga || !offset) {
		return false;
	}

	size_t copied = 0;
	size_t remaining = buf_len;
	size_t current_offset = *offset;

	for (unsigned i = 0; i < sga->sga_numsegs; i++) {
		void *seg_buf = sga->sga_segs[i].sgaseg_buf;
		const size_t seg_len = sga->sga_segs[i].sgaseg_len;

		if (current_offset >= seg_len) {
			current_offset -= seg_len;
			continue;
		}

		const size_t seg_remaining = seg_len - current_offset;
		const size_t to_copy = (remaining < seg_remaining) ?
			                       remaining :
			                       seg_remaining;

		memcpy(buf + copied,
		       seg_buf + current_offset, to_copy);

		copied += to_copy;
		remaining -= to_copy;
		current_offset = 0;

		if (remaining == 0) {
			*offset += copied;
			return false;
		}
	}

	*offset += copied;
	return true;
}

struct timespec ms_timeout_to_timespec(int ms_timeout)
{
	struct timespec ts = { 0 };
	if (ms_timeout < 0)
		return ts;
	ts = (struct timespec){
		.tv_sec = ms_timeout / 1000,
		.tv_nsec = (ms_timeout % 1000) * 1000000L,
	};
	return ts;
}
