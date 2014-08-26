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

int stx_queue_create()
{
    return kqueue();
}

int stx_queue_close(int queue)
{
    return close(queue);
}

int stx_event_wait(int queue,
                          struct kevent *eventlist,
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
        case STX_EV_READ:
            ev.filter = STX_EVFILT_READ;
            ev.flags = STX_EVCTL_ADD | STX_EVCTL_ONESHOT;
            break;
        case STX_EV_WRITE:
            ev.filter = STX_EVFILT_WRITE;
            ev.flags = STX_EVCTL_ADD | STX_EVCTL_ONESHOT;
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
