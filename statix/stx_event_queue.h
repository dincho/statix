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
    #define STX_EVCTL_MOD EPOLL_CTL_MOD
//    #define STX_EVCTL_ONESHOT EPOLLET
    #define STX_EVCTL_DISPATCH EPOLLET | EPOLLONESHOT
    #define STX_EVCTL_ENABLE 0x00 //each event modification rearm it

    #define STX_EVFILT_READ EPOLLIN
    #define STX_EVFILT_WRITE EPOLLOUT

#else //Kqueue

    #include <sys/event.h>

    typedef struct kevent stx_event_t;

    #define STX_EVCTL_ADD EV_ADD
    #define STX_EVCTL_MOD EV_ADD
    #define STX_EVCTL_ONESHOT EV_ONESHOT
    #define STX_EVCTL_DISPATCH EV_DISPATCH
    #define STX_EVCTL_ENABLE EV_ENABLE

    #define STX_EVFILT_READ EVFILT_READ
    #define STX_EVFILT_WRITE EVFILT_WRITE

    inline int stx_event_wait(int queue, stx_event_t *eventlist,
                              int nevents, const struct timespec *timeout)
    {
        return kevent(queue, 0, 0, eventlist, nevents, timeout);
    }

    inline int stx_event_ctl(const int queue, stx_event_t *ev, const int ident,
                             const int op, const int filter, void *udata)
    {
        EV_SET(ev, ident, filter, op, 0, 0, udata);
        
        return kevent(queue, ev, 1, NULL, 0, NULL);
    }

#endif //end epoll/kqueue check

int stx_queue_create();
int stx_queue_close();

#endif
