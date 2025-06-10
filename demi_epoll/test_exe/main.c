#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "dpoll.h"
#include "sockets.h"
#include "log.h"

int main(void)
{
	demi_log_init();

	int s = dpoll_socket(AF_INET, SOCK_STREAM, 0);
	assert(s > -1);
	printf("s: %d\n", s);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(2137),
	};
	int ret = dpoll_bind(s, (void *)&addr, sizeof(addr));
	assert(ret == 0);
	ret = dpoll_listen(s, 1);
	assert(ret == 0);
	int other = dpoll_accept(s, NULL, NULL);
	assert(other > -1);
	printf("other: %d\n", other);

	char buf[100];
	ssize_t read = dpoll_recv(other, buf, sizeof(buf) - 1);
	buf[read] = '\0';
	printf("read: %s\n", buf);
	dpoll_send(other, buf, read);
	dpoll_close(other);
	dpoll_close(s);
	printf("done :)\n");
}
