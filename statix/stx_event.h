//
//  stx_event.h
//  statix
//
//  Created by Dincho Todorov on 8/26/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_event_h
#define statix_stx_event_h

#include <time.h>
#include "stx_log.h"

struct timespec;

#ifdef __linux //Epoll

    #define STX_EPOLL //use epoll

    #include <sys/epoll.h>
    #include <sys/socket.h> //getsockopt

    typedef struct epoll_event stx_event_t;

    #define STX_EVCTL_ADD EPOLL_CTL_ADD
    #define STX_EVCTL_ADD_ONCE EPOLL_CTL_ADD
    #define STX_EVCTL_MOD_ONCE EPOLL_CTL_MOD

    #define STX_EVFILT_READ EPOLLIN
    #define STX_EVFILT_READ_ONCE EPOLLIN | EPOLLET | EPOLLONESHOT
    #define STX_EVFILT_WRITE_ONCE EPOLLOUT | EPOLLET | EPOLLONESHOT

    #define STX_EV_ERROR(ev) ev.events & EPOLLERR
    #define STX_EV_EOF(ev) ev.events & EPOLLRDHUP
    #define STX_EV_READ(ev) ev.events & STX_EVFILT_READ
    #define STX_EV_READ_ONCE(ev) ev.events & STX_EVFILT_READ
    #define STX_EV_IDENT(ev) ev.data.fd

    #define STX_EV_SET(evp, id, op, fl) do {    \
        struct epoll_event *__evp__ = (evp);    \
        __evp__->data.fd = (id);                  \
        __evp__->events = (fl);                  \
    } while(0)




    inline int stx_event_wait(int queue,
                              stx_event_t *eventlist,
                              int nevents,
                              const struct timespec *timeout)
    {
        return epoll_wait(queue, eventlist, nevents, (timeout->tv_sec * 1000));
    }

    inline int stx_event_ctl(const int queue, stx_event_t *ev, const int op)
    {
        return epoll_ctl(queue, op, ev->data.fd, ev);
    }

    inline void stx_event_log_error(stx_event_t *ev, stx_log_t *logger)
    {
        int error = 0;
        int ident = STX_EV_IDENT((*ev));
        socklen_t errlen = sizeof(error);
        
        if (getsockopt(ident, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0) {
            stx_log(logger, STX_LOG_ERR,
                    "Event error (#%d): %s", ident, strerror(error));
        }
    }

#else //Kqueue

    #include <sys/event.h>

    typedef struct kevent stx_event_t;

    #define STX_EVCTL_ADD EV_ADD | EV_ENABLE
    #define STX_EVCTL_ADD_ONCE EV_ADD | EV_DISPATCH | EV_ENABLE
    #define STX_EVCTL_MOD_ONCE EV_ADD | EV_DISPATCH | EV_ENABLE

    #define STX_EVFILT_READ EVFILT_READ
    #define STX_EVFILT_READ_ONCE EVFILT_READ
    #define STX_EVFILT_WRITE_ONCE EVFILT_WRITE

    #define STX_EV_ERROR(ev) ev.flags & EV_ERROR
    #define STX_EV_EOF(ev) ev.flags & EV_EOF
    #define STX_EV_READ(ev) ev.filter == STX_EVFILT_READ
    #define STX_EV_READ_ONCE(ev) ev.filter == STX_EVFILT_READ_ONCE
    #define STX_EV_IDENT(ev) (int) ev.ident

    #define STX_EV_SET(evp, id, op, fl) do {    \
        struct kevent *__evp__ = (evp);         \
        __evp__->ident = (id);                  \
        __evp__->flags = (op);                  \
        __evp__->filter = (fl);                 \
    } while(0)

    inline int stx_event_wait(int queue, stx_event_t *eventlist,
                              int nevents, const struct timespec *timeout)
    {
        return kevent(queue, 0, 0, eventlist, nevents, timeout);
    }

    inline int stx_event_ctl(const int queue, stx_event_t *ev, const int op)
    {
        return kevent(queue, ev, 1, NULL, 0, NULL);
    }

    inline void stx_event_log_error(stx_event_t *ev, stx_log_t *logger)
    {
        stx_log(logger, STX_LOG_ERR,
                "Event error (#%d): %s", STX_EV_IDENT((*ev)), ev->data);
    }

#endif //end epoll/kqueue check

int stx_queue_create();
int stx_queue_close(int queue);

#endif
