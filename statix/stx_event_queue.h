//
//  stx_event_queue.h
//  statix
//
//  Created by Dincho Todorov on 8/26/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_event_queue_h
#define statix_stx_event_queue_h

#include <sys/event.h>

typedef enum {
    STX_EV_ACCEPT,
    STX_EV_READ,
    STX_EV_WRITE,
    STX_EV_CLOSE
} stx_ev_t;

typedef struct kevent stx_event_t;

typedef struct {
    stx_ev_t    event_type;
    void		*data;
} stx_event_data_t;

int stx_queue_create();
int stx_queue_close();

int stx_event_wait(int queue,
                          struct kevent *eventlist,
                          int nevents,
                          const struct timespec *timeout);

int stx_event(int queue,
                        int ident,
                        stx_ev_t event,
                        void *udata);

#define STX_EVCTL_ADD EV_ADD
#define STX_EVCTL_ONESHOT EV_ONESHOT

#define STX_EVFILT_READ EVFILT_READ
#define STX_EVFILT_WRITE EVFILT_WRITE

#endif
