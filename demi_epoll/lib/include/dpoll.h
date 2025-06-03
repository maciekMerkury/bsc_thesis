#pragma once

#include <sys/epoll.h>

int dpoll_create(void);

int dpoll_ctl(int dpollfd, int op, int fd, struct epoll_event *event);

int dpoll_pwait(int dpollfd, struct epoll_event *events, int maxevents,
                const struct timespec *timeout, const sigset_t *sigmask);

int dsoc_close(int dpollfd);
