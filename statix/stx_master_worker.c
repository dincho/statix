//
//  stx_master_worker.c
//  statix
//
//  Created by Dincho Todorov on 9/3/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include "config.h"
#include "stx_master_worker.h"
#include "stx_event_queue.h"
#include "stx_accept.h"

void stx_master_worker(stx_server_t *server,
                       const int nb_threads,
                       stx_worker_t *workers)
{
    int master_queue, nev, ident, idx = 0;
    stx_event_t chlist[STX_MAX_EVENTS];
    stx_event_t ev;
    /* block for 5 seconds at most */
    struct timespec tmout = {5, 0};

    if ((master_queue = stx_queue_create()) == -1) {
        stx_log_syserr(server->logger, "stx_queue_create: %s");
        return;
    }
    
    STX_EV_SET(&ev, server->sock, STX_EVCTL_ADD, STX_EVFILT_READ);    
    if (-1 == stx_event_ctl(master_queue, &ev, STX_EVCTL_ADD)) {
        stx_log_syserr(server->logger, "stx_event_ctl: %s");
        return;
    }
    
    while (STX_RUNNING) {
        nev = stx_event_wait(master_queue, (stx_event_t *)&chlist, STX_MAX_EVENTS, &tmout);
        
        if (nev == -1) {
            stx_log_syserr(server->logger, "stx_event_wait: %s");
            if (errno != EINTR) {
                break;
            }
        }
        
        for (int i = 0; i < nev; i++) {
            ev = chlist[i];
            ident = STX_EV_IDENT(ev);
            
            if (STX_EV_EOF(ev)) {
                stx_log(server->logger, STX_LOG_ERR, "Event error: #%d", ident);
                continue;
            }
            
            if (STX_EV_READ(ev)) {
                stx_log(server->logger, STX_LOG_EVENT, "STX_EV_ACCEPT: #%d", ident);
                stx_accept(server, workers, nb_threads, &idx);
            }
        }
    }
    
    stx_log(server->logger, STX_LOG_WARN, "Master worker is shutting down");
}
