#include "socket_wrapper.h"
#include "log.h"
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <demi/libos.h>
#include <demi/wait.h>
#include <stdio.h>
#include <sys/param.h>

#include "utils.h"

const struct timespec ZERO = { 0 };

static inline bool sga_is_empty(const struct sga *sga)
{
	return sga->elem.sga_numsegs == 0;
}

static inline bool accept_is_empty(const struct accept *acc)
{
	return acc->elem.qd == -1;
}

static inline void accept_free(struct accept *acc)
{
	acc->elem.qd = -1;
	acc->base.pending = false;
}

static void sga_free(struct sga *sga)
{
	assert(!sga_is_empty(sga));
	int ret = demi_sgafree(&sga->elem);
	if (ret) {
		demi_log("seg count: %d\n", sga->elem.sga_numsegs);
		demi_log("sga_free: %s\n", strerror(ret));
	}
	sga->elem.sga_numsegs = 0;
	sga->base.pending = false;
}

static void sga_new(struct sga *sga, size_t size)
{
	sga->elem = demi_sgaalloc(size);
	assert(!sga_is_empty(sga));
}

demi_result_t maybe_accept(socket_t *soc, struct sockaddr_in *addr)
{
	if (accept_is_empty(&soc->accept)) {
		assert(demi_accept(&soc->accept.base.tok, soc->qd) == 0);
		soc->accept.base.pending = true;
		errno = EWOULDBLOCK;
		return -1;
	}
	if (soc->accept.base.pending) {
		demi_qresult_t res;
		const int ret = demi_wait(&res, soc->accept.base.tok, &ZERO);
		if (ret == ETIMEDOUT) {
			errno = EWOULDBLOCK;
			return -1;
		}
		assert(ret == 0);
		assert(res.qr_opcode == DEMI_OPC_ACCEPT ||
			res.qr_opcode == DEMI_OPC_FAILED);
		if (res.qr_opcode == DEMI_OPC_ACCEPT) {
			soc->accept.elem = res.qr_value.ares;
		} else {
			demi_log("accept failed with reason: %s\n",
			         strerror(res.qr_ret));
			errno = res.qr_ret;
			return -1;
		}
	}

	*addr = soc->accept.elem.addr;
	const demi_socket_t qd = soc->accept.elem.qd;
	accept_free(&soc->accept);
	demi_log("soc %d accepted a new connection with qd %i\n", soc->qd, qd);
	return result_from_soc(qd);
}

ssize_t maybe_write(socket_t *soc, const void *buf, size_t len)
{
	demi_qresult_t res;
	if (soc->send.base.pending) {
		const int ret = demi_wait(&res, soc->send.base.tok, &ZERO);
		if (ret == ETIMEDOUT)
			goto would_block;

		assert(ret == 0);
		sga_free(&soc->send);
	}
	if (sga_is_empty(&soc->send)) {
		sga_new(&soc->send, len);
		size_t ret = copy_buf_into_sga(buf, len, &soc->send.elem);
		assert(
			demi_push(&soc->send.base.tok, soc->qd, &soc->send.elem)
			==
			0);
		soc->send.base.pending = true;
		return ret;
	}

	demi_log("unreachable state in %s\n", __func__);
	abort();

would_block:
	errno = EWOULDBLOCK;
	return -1;
}

ssize_t maybe_read(socket_t *soc, void *buf, size_t len)
{
	if (sga_is_empty(&soc->recv) && !soc->recv.base.pending) {
		soc->recv.base.pending = true;
		assert(demi_pop(&soc->recv.base.tok, soc->qd) == 0);
		goto would_block;
	}

	if (soc->recv.base.pending) {
		demi_qresult_t res;
		const int ret = demi_wait(&res, soc->recv.base.tok, &ZERO);
		if (ret == ETIMEDOUT)
			goto would_block;
		assert(ret == 0);
		soc->recv.base.pending = false;
		soc->recv_off = 0;
		// TODO: error handling
		soc->recv.elem = res.qr_value.sga;
	}
	assert(!sga_is_empty(&soc->recv));
	const size_t off = soc->recv_off;
	bool emptied = copy_sga_into_buf(buf, len, &soc->recv.elem,
	                                 &soc->recv_off);
	if (emptied) {
		sga_free(&soc->recv);
	}
	return (ssize_t)(soc->recv_off - off);

would_block:
	errno = EWOULDBLOCK;
	return -1;
}

int socket_init(socket_t *soc)
{
	memset(soc, 0, sizeof(*soc));
	accept_free(&soc->accept);
	const int ret = demi_socket((int *)&soc->qd, AF_INET, SOCK_STREAM, 0);
	DEMI_ERR(ret, "socket init\n");
	return 0;
}

void socket_destroy(socket_t *soc)
{
	struct sga *sgas[2] = { &soc->send, &soc->recv };
	const int sgas_count = 2 - socket_is_accepting(soc);
	demi_qresult_t res;
	for (int i = 0; i < sgas_count; ++i) {
		if (!sga_is_empty(sgas[i])) {
			// TODO: do this better
			if (sgas[i]->base.pending) {
				assert(
					demi_wait(&res, sgas[i]->base.tok, NULL)
					==
					0);
			}
			//assert(res.qr_opcode == DEMI_OPC_PUSH || res.qr_opcode == DEMI_OPC_POP || );
			assert(
				res.qr_opcode != DEMI_OPC_FAILED || res.
				qr_opcode != DEMI_OPC_INVALID);
			if (i == 0) {
				demi_log("just finished writing\n");
			}
			sga_free(sgas[i]);
		}
	}
	assert(demi_close(soc->qd) == 0);
}

bool socket_can_write(const socket_t *soc)
{
	return sga_is_empty(&soc->send) && !soc->send.base.pending;
}

bool socket_can_read(const socket_t *soc)
{
	return !soc->recv.base.pending && !sga_is_empty(&soc->recv);
}

bool socket_can_accept(const socket_t *soc)
{
	return !soc->accept.base.pending && !accept_is_empty(&soc->accept);
}

void socket_handle_event(socket_t *soc, const demi_qresult_t *res)
{
	const demi_opcode_t opcode = res->qr_opcode;
	assert(
		opcode == DEMI_OPC_ACCEPT || opcode == DEMI_OPC_PUSH || opcode
		== DEMI_OPC_POP);

	switch (opcode) {
	case DEMI_OPC_ACCEPT:
		assert(socket_is_accepting(soc));
		soc->accept.base.pending = false;
		soc->accept.elem = res->qr_value.ares;
		demi_log("socket %d can accept a new con\n", soc->qd);
		break;
	case DEMI_OPC_POP:
		assert(!socket_is_accepting(soc));
		soc->recv.base.pending = false;
		soc->recv_off = 0;
		soc->recv.elem = res->qr_value.sga;
		break;
	case DEMI_OPC_PUSH:
		soc->send.base.pending = false;
		soc->send.elem = res->qr_value.sga;
		break;
	default:
		GIVE_UP("invalid demi opcode: %d\n", opcode);
	}
}

static void copy_iovs_into_sga(const struct iovec *iov, int iov_cnt,
                               struct sga *sga)
{
	const demi_sgarray_t *s = &sga->elem;
	size_t buf_off = 0;
	size_t seg_off = 0;
	for (int i = 0; i < iov_cnt; ++i) {
		struct iovec v = iov[i];
		size_t copied = 0;
		while (copied < v.iov_len) {
			demi_sgaseg_t seg = s->sga_segs[seg_off];
			size_t to_copy = MIN(v.iov_len,
			                     seg.sgaseg_len - buf_off);
			memcpy(seg.sgaseg_buf + buf_off, v.iov_base + copied,
			       to_copy);
			copied += to_copy;
			buf_off += to_copy;
			if (buf_off >= seg.sgaseg_len) {
				++seg_off;
				buf_off = 0;
			}
		}
	}
}

ssize_t maybe_writev(socket_t *soc, const struct iovec *iov, int iov_cnt)
{
	if (soc->send.base.pending) {
		demi_qresult_t res;
		const int ret = demi_wait(&res, soc->send.base.tok, &ZERO);
		if (ret == ETIMEDOUT) {
			errno = EWOULDBLOCK;
			return -1;
		}

		assert(ret == 0);
		assert(res.qr_opcode == DEMI_OPC_PUSH);
		sga_free(&soc->send);
	}
	assert(sga_is_empty(&soc->send));
	size_t total_size = 0;
	for (int i = 0; i < iov_cnt; ++i) {
		total_size += iov[i].iov_len;
	}
	if (total_size == 0)
		return 0;
	sga_new(&soc->send, total_size);
	copy_iovs_into_sga(iov, iov_cnt, &soc->send);
	// TODO: actually push data

	assert(demi_push(&soc->send.base.tok, soc->qd, &soc->send.elem) == 0);
	soc->send.base.pending = true;
	return total_size;
}

ssize_t maybe_readv(socket_t *soc, struct iovec *iovs, int iovs_cnt)
{
	ssize_t read = 0;
	for (int i = 0; i < iovs_cnt; ++i) {
		struct iovec iov = iovs[i];
		ssize_t r = maybe_read(soc, iov.iov_base, iov.iov_len);
		if (r < 0) {
			if (read == 0)
				return -1;
			assert(errno == EWOULDBLOCK);
			break;
		}
		read += r;
		if (r < iov.iov_len)
			break;
	}

	return read;
}
