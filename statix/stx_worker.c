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
#include "stx_conn_pool.h"

static const int MAX_EVENTS = 1024; //x 32b = 32Kb


void *stx_worker(void *arg)
{
    int queue, nev;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_event_data_t *ev_data;
    stx_server_t *server = arg;
    stx_conn_pool_t *conn_pool;
    
    stx_log(server->logger, STX_LOG_DEBUG, "Started new worker thread");
    
    if (NULL == (conn_pool = stx_conn_pool_init(1000))) {
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
            
            if (ev.flags & EV_EOF) {
                stx_log(server->logger, STX_LOG_WARN, "Connection reset by peer: #%d", ev.ident);
                continue;
            }
            
            if (ev.flags & EV_ERROR) {
                stx_log(server->logger, STX_LOG_ERR, "Event error: #%d", ev.ident);
                continue;
            }

            ev_data = (stx_event_data_t *) ev.udata;
            
            switch (ev_data->event_type) {
                case STX_EV_ACCEPT:
                    stx_accept(queue, ev_data->data, conn_pool);
                    break;
                case STX_EV_READ:
                    stx_read(queue, ev_data->data); //request
                    break;
                case STX_EV_WRITE:
                    stx_write(queue, ev_data->data); //request
                    break;
                case STX_EV_CLOSE:
                    stx_request_close(ev_data->data, conn_pool); //request
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
    stx_conn_pool_destroy(conn_pool);
    
    return NULL;
}
