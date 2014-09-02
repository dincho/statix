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

void *stx_worker(void *arg)
{
    int queue, nev;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_event_data_t *ev_data;
    stx_server_t *server = arg;
    stx_list_t *conn_pool;
    stx_request_t *request;
    
    stx_log(server->logger, STX_LOG_DEBUG, "Started new worker thread");
    
    if (NULL == (conn_pool = stx_list_init())) {
        stx_log(server->logger, STX_LOG_ERR, "Cannot create connection pool");
        return NULL;
    }

    if ((queue = stx_queue_create()) == -1) {
        perror("kqueue");
        return NULL;
    }

    stx_event(queue, server->sock, STX_EV_ACCEPT, server);
    
    for (;;) {
        nev = stx_event_wait(queue, (stx_event_t *)&chlist, MAX_EVENTS, NULL);
        
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
                    stx_log(server->logger, STX_LOG_DEBUG, "STX_EV_ACCEPT: #%d", ev.ident);
                    stx_accept(queue, ev_data->data, conn_pool);
                    break;
                case STX_EV_READ:
                    stx_log(server->logger, STX_LOG_DEBUG, "STX_EV_READ: #%d", ev.ident);
                    request = (stx_request_t *) ev_data->data;
                    request->close = ev.flags & EV_EOF;

                    if (request->close) {
                        stx_request_close(queue, request, conn_pool);
                    } else {
                        stx_read(queue, request);
                    }
                    
                    break;
                case STX_EV_WRITE:
                    stx_log(server->logger, STX_LOG_DEBUG, "STX_EV_WRITE: #%d", ev.ident);
                    stx_write(queue, ev_data->data); //request
                    break;
                case STX_EV_CLOSE:
                    stx_log(server->logger, STX_LOG_DEBUG, "STX_EV_CLOSE: #%d", ev.ident);
                    stx_request_close(queue, ev_data->data, conn_pool); //request
                    break;
                default:
                    stx_log(server->logger, STX_LOG_ERR, "Unknown event type: %d", ev_data->event_type);
                    break;
            }
            
            if (ev.flags & STX_EVCTL_ONESHOT) {
                free(ev_data);
            }
        }
    }
    
    stx_queue_close(queue);
    stx_list_destroy(conn_pool);
    
    return NULL;
}
