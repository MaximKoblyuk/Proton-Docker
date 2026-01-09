#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include "proton.h"
#include "event.h"

proton_event_loop_t* proton_event_loop_create(int max_events) {
    proton_event_loop_t *loop = malloc(sizeof(proton_event_loop_t));
    if (!loop) return NULL;
    
    /* Create epoll instance */
    loop->epfd = epoll_create1(0);
    if (loop->epfd < 0) {
        free(loop);
        return NULL;
    }
    
    loop->max_events = max_events;
    loop->events = calloc(max_events, sizeof(proton_event_t*));
    if (!loop->events) {
        close(loop->epfd);
        free(loop);
        return NULL;
    }
    
    return loop;
}

int proton_event_add(proton_event_loop_t *loop, proton_event_t *ev, int events) {
    if (!loop || !ev) return PROTON_ERROR;
    
    struct epoll_event epev;
    memset(&epev, 0, sizeof(epev));
    
    /* Convert proton events to epoll events */
    if (events & PROTON_EVENT_READ) epev.events |= EPOLLIN;
    if (events & PROTON_EVENT_WRITE) epev.events |= EPOLLOUT;
    epev.events |= EPOLLET; /* Edge-triggered */
    
    epev.data.ptr = ev;
    ev->events = events;
    
    if (epoll_ctl(loop->epfd, EPOLL_CTL_ADD, ev->fd, &epev) < 0) {
        if (errno == EEXIST) {
            /* Already exists, modify instead */
            return epoll_ctl(loop->epfd, EPOLL_CTL_MOD, ev->fd, &epev);
        }
        return PROTON_ERROR;
    }
    
    return PROTON_OK;
}

int proton_event_del(proton_event_loop_t *loop, proton_event_t *ev) {
    if (!loop || !ev) return PROTON_ERROR;
    
    if (epoll_ctl(loop->epfd, EPOLL_CTL_DEL, ev->fd, NULL) < 0) {
        return PROTON_ERROR;
    }
    
    return PROTON_OK;
}

int proton_event_process(proton_event_loop_t *loop, int timeout) {
    if (!loop) return PROTON_ERROR;
    
    struct epoll_event events[loop->max_events];
    int nfds = epoll_wait(loop->epfd, events, loop->max_events, timeout);
    
    if (nfds < 0) {
        if (errno == EINTR) {
            return 0;
        }
        return PROTON_ERROR;
    }
    
    /* Process events */
    for (int i = 0; i < nfds; i++) {
        proton_event_t *ev = (proton_event_t*)events[i].data.ptr;
        if (!ev) continue;
        
        /* Handle read events */
        if ((events[i].events & EPOLLIN) && ev->read_handler) {
            ev->read_handler(ev);
        }
        
        /* Handle write events */
        if ((events[i].events & EPOLLOUT) && ev->write_handler) {
            ev->write_handler(ev);
        }
        
        /* Handle errors */
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            proton_log(LOG_DEBUG, "Socket error or hangup on fd %d", ev->fd);
            if (ev->read_handler) {
                ev->read_handler(ev); /* Let handler deal with it */
            }
        }
    }
    
    return nfds;
}

void proton_event_loop_destroy(proton_event_loop_t *loop) {
    if (!loop) return;
    
    if (loop->epfd >= 0) {
        close(loop->epfd);
    }
    
    free(loop->events);
    free(loop);
}

proton_event_t* proton_event_create(int fd) {
    proton_event_t *ev = calloc(1, sizeof(proton_event_t));
    if (!ev) return NULL;
    
    ev->fd = fd;
    ev->events = 0;
    ev->data = NULL;
    ev->read_handler = NULL;
    ev->write_handler = NULL;
    
    return ev;
}

void proton_event_destroy(proton_event_t *ev) {
    if (ev) {
        free(ev);
    }
}
