//
//  stx_event_queue.h
//  statix
//
//  Created by Dincho Todorov on 8/26/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_event_queue_h
#define statix_stx_event_queue_h

#include <time.h>

struct timespec;

#ifdef __linux //Epoll

    #define STX_EPOLL //use epoll

    #include <sys/epoll.h>

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

    inline int stx_event_wait(int queue,
                              stx_event_t *eventlist,
                              int nevents,
                              const struct timespec *timeout)
    {
        return epoll_wait(queue, eventlist, nevents, -1);
    }

    inline int stx_event_ctl(const int queue, stx_event_t *ev, const int ident,
                             const int op, const int filter)
    {
        ev->events = filter;
        ev->data.fd = ident;
        
        return epoll_ctl(queue, op, ident, ev);
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

    inline int stx_event_wait(int queue, stx_event_t *eventlist,
                              int nevents, const struct timespec *timeout)
    {
        return kevent(queue, 0, 0, eventlist, nevents, timeout);
    }

    inline int stx_event_ctl(const int queue, stx_event_t *ev, const int ident,
                             const int op, const int filter)
    {
        EV_SET(ev, ident, filter, op, 0, 0, NULL);
        
        return kevent(queue, ev, 1, NULL, 0, NULL);
    }

#endif //end epoll/kqueue check

int stx_queue_create();
int stx_queue_close();

#endif
