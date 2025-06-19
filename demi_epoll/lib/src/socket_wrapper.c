#include "socket_wrapper.h"
#include "log.h"
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <demi/libos.h>
#include <demi/wait.h>
#include <stdio.h>

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

int maybe_accept(socket_t *soc, struct sockaddr_in *addr)
{
	if (soc->accept.base.pending) {
		demi_qresult_t res;
		const int ret = demi_wait(&res, soc->accept.base.tok, &ZERO);
		if (ret == ETIMEDOUT)
			goto would_block;

		if (ret != 0) {
			errno = ret;
			perror(__func__);
			abort();
		}
		assert(ret == 0);
		assert(res.qr_opcode == DEMI_OPC_ACCEPT ||
			res.qr_opcode == DEMI_OPC_FAILED);
		if (res.qr_opcode == DEMI_OPC_ACCEPT)
			soc->accept.elem = res.qr_value.ares;
		else
			demi_log("accept failed with reason: %s\n",
			         strerror(res.qr_ret));
	} else if (accept_is_empty(&soc->accept)) {
		assert(demi_accept(&soc->accept.base.tok, soc->qd) == 0);
		soc->accept.base.pending = true;
		goto would_block;
	}

	*addr = soc->accept.elem.addr;
	int qd = soc->accept.elem.qd;
	accept_free(&soc->accept);
	return qd;

	// demi_log("unreachable state in %s\n", __func__);
	// abort();

would_block:
	errno = EWOULDBLOCK;
	return -1;
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
	const int ret = demi_socket(&soc->qd, AF_INET, SOCK_STREAM, 0);
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
