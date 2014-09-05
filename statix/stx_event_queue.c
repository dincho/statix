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

int stx_event(int queue, int ident, stx_ev_t event, void *udata)
{
    stx_event_t ev;
    stx_event_data_t *data;
    int ctl;
    
    data = malloc(sizeof(stx_event_data_t));
    data->event_type = event;
    data->data = udata;
    
    ev.data.fd = ident;
    ev.data.ptr = data;
    
    switch (event) {
        case STX_EV_ACCEPT:
            ev.events = STX_EVFILT_READ;
            ctl = STX_EVCTL_ADD;
            break;
        case STX_EV_FIRST_READ:
            ev.events = STX_EVFILT_READ | STX_EVCTL_DISPATCH;
            ctl = STX_EVCTL_ADD;
            break;
        case STX_EV_READ:
            ev.events = STX_EVFILT_READ | STX_EVCTL_DISPATCH;
            ctl = STX_EVCTL_MOD;
            break;
        case STX_EV_WRITE:
            ev.events = STX_EVFILT_WRITE | STX_EVCTL_DISPATCH;
            ctl = STX_EVCTL_MOD;
            break;
        default:
            return -1;
            break;
    }
    
    if (-1 == epoll_ctl(queue, ctl, ident, &ev)) {
        char err[100];
        sprintf(err, "epoll_ctl (%d, %X, %d, %X)", queue, ctl, ident, ev.events);
        perror(err);
        return -1;
    }
    
    return 0;
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

int stx_event(int queue, int ident, stx_ev_t event, void *udata)
{
    struct kevent ev;
    stx_event_data_t *data;
    
    data = malloc(sizeof(stx_event_data_t));
    data->event_type = event;
    data->data = udata;
    
    ev.ident = ident;
    ev.fflags = 0;
    ev.data = 0;
    ev.udata = data;
    
    switch (event) {
        case STX_EV_ACCEPT:
            ev.filter = STX_EVFILT_READ;
            ev.flags = STX_EVCTL_ADD;
            break;
        case STX_EV_FIRST_READ:
            ev.filter = STX_EVFILT_READ;
            ev.flags = STX_EVCTL_ADD | STX_EVCTL_DISPATCH;
            break;
        case STX_EV_READ:
            ev.filter = STX_EVFILT_READ;
            ev.flags = STX_EVCTL_MOD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE;
            break;
        case STX_EV_WRITE:
            ev.filter = STX_EVFILT_WRITE;
            ev.flags = STX_EVCTL_MOD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE;
            break;
        default:
            return -1;
            break;
    }
    
    if (-1 == kevent(queue, &ev, 1, NULL, 0, NULL)) {
        perror("kevent");
        return -1;
    }
    
    return 0;
}

#endif //end epoll/kqueue check

