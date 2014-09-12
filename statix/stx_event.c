//
//  stx_event.c
//  statix
//
//  Created by Dincho Todorov on 8/26/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //close()

#include "stx_event.h"

extern int stx_event_wait(int queue, stx_event_t *eventlist,
                          int nevents, const struct timespec *timeout);

extern int stx_event_ctl(const int queue, stx_event_t *ev, const int op);


#ifdef STX_EPOLL //Epoll

int stx_queue_create()
{
    return epoll_create1(0);
}

int stx_queue_close(int queue)
{
    return close(queue);
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

#endif //end epoll/kqueue check

