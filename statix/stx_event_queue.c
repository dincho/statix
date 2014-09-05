//
//  stx_event_queue.c
//  statix
//
//  Created by Dincho Todorov on 8/26/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //close()

#include "stx_event_queue.h"

#ifdef STX_EPOLL //Epoll

int stx_queue_create()
{
    return epoll_create1(0);
}

int stx_queue_close(int queue)
{
    return close(queue);
}

int stx_event_wait(int queue,
                   stx_event_t *eventlist,
                   int nevents,
                   const struct timespec *timeout)
{
    return epoll_wait(queue, eventlist, nevents, -1);
}

int stx_event_ctl(const int queue, stx_event_t *ev, const int ident,
                  const int op, const int filter, void *udata)
{
    ev.events = filter;
    ev.data.fd = ident;
    ev->data.ptr = udata;
    
    return epoll_ctl(queue, ctl, ident, ev);
}

#else //Kqueue

int stx_queue_create()
{
    return kqueue();
}

int stx_queue_close(int queue)
{
    return close(queue);
}

int stx_event_wait(int queue,
                   stx_event_t *eventlist,
                   int nevents,
                   const struct timespec *timeout)
{
    return kevent(queue, 0, 0, eventlist, nevents, 0);
}

int stx_event_ctl(const int queue, stx_event_t *ev, const int ident,
                  const int op, const int filter, void *udata)
{
    EV_SET(ev, ident, filter, op, 0, 0, udata);
    
    return kevent(queue, ev, 1, NULL, 0, NULL);
}

#endif //end epoll/kqueue check

