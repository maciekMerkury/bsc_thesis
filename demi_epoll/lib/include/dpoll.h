#pragma once

#include <sys/epoll.h>

int dpoll_create(int flags);

int dpoll_ctl(int dpollfd, int op, int fd, struct epoll_event *event);

int dpoll_pwait(int dpollfd, struct epoll_event *events, int maxevents,
                int timeout, const sigset_t *sigmask);

/// functions only used when I want to print something
void debug_print(void);

