//
//  stx_worker.c
//  statix
//
//  Created by Dincho Todorov on 8/30/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <errno.h>

#include "stx_worker.h"
#include "stx_event_queue.h"
#include "stx_accept.h"
#include "stx_read.h"
#include "stx_write.h"
#include "stx_log.h"
#include "stx_connection.h"

static const int MAX_EVENTS = 1024; //x 32b = 32Kb


void *stx_worker(void *arg)
{
    int queue, nev;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_event_data_t *ev_data;
    stx_server_t *server = arg;
    stx_connection_pool_t conn_pool;
    conn_pool.max_connections = 100;
    conn_pool.current_connections = 0;

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
            ev_data = (stx_event_data_t *) ev.udata;
            
            switch (ev_data->event_type) {
                case STX_EV_ACCEPT:
                    stx_accept(queue, ev_data->data, &conn_pool);
                    break;
                case STX_EV_READ:
                    stx_read(queue, ev_data->data); //request
                    break;
                case STX_EV_WRITE:
                    stx_write(queue, ev_data->data); //request
                    break;
                case STX_EV_CLOSE:
                    stx_request_close(ev_data->data, &conn_pool); //request
                    break;
            }
            
            if (ev.flags & STX_EVCTL_ONESHOT) {
                free(ev_data);
            }
        }
    }
    
    stx_queue_close(queue);
    
    return NULL;
}
