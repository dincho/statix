//
//  stx_worker.c
//  statix
//
//  Created by Dincho Todorov on 8/30/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <errno.h>
#include <string.h> //memset
#include <stdio.h>
#include "stx_worker.h"
#include "stx_event_queue.h"
#include "stx_accept.h"
#include "stx_read.h"
#include "stx_write.h"
#include "stx_log.h"


static const int MAX_EVENTS = 1024; //x 32b = 32Kb

void *stx_worker(void *arguments)
{
    int nev;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_event_data_t *ev_data;
    stx_request_t *request;
    stx_worker_t *arg = arguments;
    
    stx_log(arg->server->logger, STX_LOG_DEBUG,
            "Started new worker thread - queue: %d, pool: %p", arg->queue, arg->conn_pool);
    
    for (;;) {
        nev = stx_event_wait(arg->queue, (stx_event_t *)&chlist, MAX_EVENTS, NULL);
        
        if (nev == -1) {
            perror("stx_event_wait()");
            if (errno != EINTR) {
                break;
            }
        }
        
        for (int i = 0; i < nev; i++) {
            ev = chlist[i];
            
            if (ev.flags & EV_ERROR) {
                stx_log(arg->server->logger, STX_LOG_ERR, "Event error: #%d", ev.ident);
                continue;
            }

            ev_data = (stx_event_data_t *) ev.udata;
            
            switch (ev_data->event_type) {
                case STX_EV_READ:
                    stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_READ: #%d", ev.ident);
                    request = (stx_request_t *) ev_data->data;
                    request->close = ev.flags & EV_EOF;

                    if (request->close) {
                        stx_request_close(arg->queue, request, arg->conn_pool);
                    } else {
                        stx_read(arg->queue, request);
                    }
                    
                    break;
                case STX_EV_WRITE:
                    stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_WRITE: #%d", ev.ident);
                    stx_write(arg->queue, ev_data->data); //request
                    break;
                case STX_EV_CLOSE:
                    stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_CLOSE: #%d", ev.ident);
                    stx_request_close(arg->queue, ev_data->data, arg->conn_pool); //request
                    break;
                default:
                    stx_log(arg->server->logger, STX_LOG_ERR, "Unknown event type: %d", ev_data->event_type);
                    break;
            }
            
            if (ev.flags & STX_EVCTL_ONESHOT) {
                free(ev_data);
            }
        }
    }
    
    return NULL;
}
