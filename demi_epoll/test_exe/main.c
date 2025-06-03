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

	int s = dsoc_socket(AF_INET, SOCK_STREAM, 0);
	assert(s > -1);
	printf("s: %d\n", s);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(2137),
	};
	int ret = dsoc_bind(s, (void *)&addr, sizeof(addr));
	assert(ret == 0);
	ret = dsoc_listen(s, 1);
	assert(ret == 0);
	int other = dsoc_accept(s, NULL, NULL);
	assert(other > -1);
	printf("other: %d\n", other);

	char buf[100];
	ssize_t read = dsoc_recv(other, buf, sizeof(buf) - 1);
	buf[read] = '\0';
	printf("read: %s\n", buf);
	dsoc_send(other, buf, read);
	dsoc_close(other);
	dsoc_close(s);
	printf("done :)\n");
}
