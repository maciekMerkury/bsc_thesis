#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "dpoll.h"
#include "sockets.h"
#include "log.h"
#include <errno.h>
#include <stdlib.h>

#define spin(func, tmp) do { tmp = func; if (tmp >= 0) break; if  (tmp < 0 && errno == EWOULDBLOCK) continue; perror(#func); abort(); } while (1);

int main(void)
{
	dpoll_init();

	int s = dpoll_socket(AF_INET, SOCK_STREAM, 0);
	assert(s > -1);
	printf("s: %d\n", s);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(0x7f000001),
		.sin_port = htons(2137),
	};
	int ret = dpoll_bind(s, (void *)&addr, sizeof(addr));
	assert(ret == 0);
	int pollfd = dpoll_epoll_create(0);
	assert(ret >= 0);

	spin(dpoll_listen(s, 1), ret);

	socklen_t len = sizeof(addr);
	ret = dpoll_getsockname(s, (void *)&addr, &len);
	assert(ret == 0);

	int other;
	struct epoll_event ev = {
		.events = EPOLLIN,
		.data.fd = s,
	};
	ret = dpoll_epoll_ctl(pollfd, EPOLL_CTL_ADD, s, &ev);
	assert(ret == 0);
	ret = dpoll_epoll_pwait(pollfd, &ev, 1, -1, NULL);
	assert(ret == 1);
	other = dpoll_accept(s, NULL, NULL);
	assert(other >= 0);
	printf("other: %d\n", other);

	ret = dpoll_epoll_ctl(pollfd, EPOLL_CTL_DEL, s, NULL);
	assert(ret == 0);
	ev.events = EPOLLIN;
	ev.data.fd = other;

	ret = dpoll_epoll_ctl(pollfd, EPOLL_CTL_ADD, other, &ev);
	assert(ret == 0);
	ret = dpoll_epoll_pwait(pollfd, &ev, 1, -1, NULL);
	assert(ret == 1);

	char buf[100];
	ssize_t read = dpoll_read(other, &buf, sizeof(buf) - 1);
	assert(read >= 0);
	buf[read] = '\0';
	printf("read: %s\n", buf);
	ssize_t bruh;
	spin(dpoll_write(other, &buf, read), bruh);
	dpoll_close(pollfd);
	dpoll_close(other);
	dpoll_close(s);
	printf("done :)\n");
}
