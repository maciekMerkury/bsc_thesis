#pragma once

#include <sys/epoll.h>

int dpoll_epoll_create(int flags);

int dpoll_epoll_ctl(int dpollfd, int op, int fd, struct epoll_event *event);

int dpoll_epoll_pwait(int dpollfd, struct epoll_event *events, int maxevents,
                      int timeout, const sigset_t *sigmask);

/// functions only used when I want to print something
void debug_print(void);

void dpoll_init(void);
