//
//  stx_master_worker.c
//  statix
//
//  Created by Dincho Todorov on 9/3/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h> //perror
#include <errno.h>

#include "stx_master_worker.h"
#include "stx_event_queue.h"
#include "stx_accept.h"

static const int MAX_EVENTS = 1024; //x 32b = 32Kb

void stx_master_worker(stx_server_t *server,
                       int nb_threads,
                       stx_worker_t *workers)
{
    int master_queue, nev, thread_index = 0;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_event_data_t *ev_data;
    
    if ((master_queue = stx_queue_create()) == -1) {
        perror("kqueue");
        return;
    }
    
    stx_event(master_queue, server->sock, STX_EV_ACCEPT, NULL);
    
    for (;;) {
        nev = stx_event_wait(master_queue, (stx_event_t *)&chlist, MAX_EVENTS, NULL);
        
        if (nev == -1) {
            perror("stx_event_wait()");
            if (errno != EINTR) {
                break;
            }
        }
        
        for (int i = 0; i < nev; i++) {
            ev = chlist[i];
            
            if (ev.flags & EV_ERROR) {
                stx_log(server->logger, STX_LOG_ERR, "Event error: #%d", ev.ident);
                continue;
            }
            
            ev_data = (stx_event_data_t *) ev.udata;
            
            switch (ev_data->event_type) {
                case STX_EV_ACCEPT:
                    thread_index = (thread_index + 1) % nb_threads;
                    
                    stx_log(server->logger, STX_LOG_DEBUG, "STX_EV_ACCEPT: #%d", ev.ident);
                    stx_accept(workers[thread_index].queue,
                               server,
                               workers[thread_index].conn_pool);
                    break;
                default:
                    stx_log(server->logger, STX_LOG_ERR, "Unknown event type: %d", ev_data->event_type);
                    break;
            }
        }
    }
}
