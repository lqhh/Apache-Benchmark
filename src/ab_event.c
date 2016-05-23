
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <ab_event.h>
#include <ab_param.h>

static int ep;
static struct epoll_event *event_list;
static int nevents = 1024;

int
ab_event_init() {

    ep = epoll_create(ab_concurrency);
    if (ep == -1) {
        printf("epoll_create() failed\n");
        return -1;
    }

    event_list = (struct epoll_event *)malloc(
        sizeof(struct epoll_event) * nevents);
    if (event_list == NULL) {
        close(ep);
        return -1;
    }

    return 0;
}


int
ab_event_add_connection(ab_connection_t *c) {

    struct epoll_event ee;

    ee.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLHUP;
    ee.data.ptr = (void *) c;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, c->fd, &ee) == -1) {
        printf("epoll_ctl(EPOLL_CTL_ADD, %d) failed\n", c->fd);
        return -1;
    }

    return 0;
}


int
ab_event_del_connection(ab_connection_t *c) {

    struct epoll_event ee;
    
    if (c->fd == -1)
        return 0;

    ee.events = 0;
    ee.data.ptr = NULL;

    if (epoll_ctl(ep, EPOLL_CTL_DEL, c->fd, &ee) == -1) {
        printf("epoll_ctl(EPOLL_CTL_DEL, %d) failed\n", c->fd);
        return -1;
    }

    return 0;
}


int
ab_event_poll(int timer) {

    int i;
    int rc;
    int events;
    ab_connection_t *c;

    rc = epoll_wait(ep, event_list, nevents, timer);
    if (rc == -1) {
        if (errno == EINTR)
            return 0;
        printf("epoll_wait() failed\n");
        return -1;
    }

    for (i = 0; i < rc; i++) {
        c = event_list[i].data.ptr;
        events = event_list[i].events;
        if (events & EPOLLIN)
            c->event_read = 1;

        if (events & (EPOLLOUT))
            c->event_write = 1;

        if (events & (EPOLLERR | EPOLLHUP))
            c->event_error = 1;
        
        ab_connection_handler(c);
    }

    return 0;
}

