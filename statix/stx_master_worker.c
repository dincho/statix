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
    int master_queue, nev, ident, error = 0, thread_index = 0, read;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    
    if ((master_queue = stx_queue_create()) == -1) {
        perror("kqueue");
        return;
    }
    
    if (-1 == stx_event_ctl(master_queue, &ev, server->sock,
                            STX_EVCTL_ADD, STX_EVFILT_READ, NULL)) {
        perror("stx_event_ctl");
        return;
    }
    
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
            
#ifdef STX_EPOLL
            ident = ev.data.fd;
            error = ev.events & EPOLLERR;
            read = (ev.events & STX_EVFILT_READ);
#else
            ident = (int) ev.ident;
            error = ev.flags & EV_ERROR;
            read = (ev.filter == STX_EVFILT_READ);
#endif
            
            if (error) {
                stx_log(server->logger, STX_LOG_ERR, "Event error: #%d", ident);
                continue;
            }
            
            if (read) {
                thread_index = (thread_index + 1) % nb_threads;
                
                stx_log(server->logger, STX_LOG_DEBUG, "STX_EV_ACCEPT: #%d (backlog: %d)", ident, ev.data);
                stx_accept(workers[thread_index].queue,
                           server,
                           workers[thread_index].conn_pool);
            }
        }
    }
}
