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
    int nev, ident, error = 0, eof = 0;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_event_data_t *ev_data;
    stx_request_t *request;
    stx_worker_t *arg = arguments;
    int8_t ret;
    
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
            
#ifdef STX_EPOLL
            ident = ev.data.fd;
            ev_data = (stx_event_data_t *) ev.data.ptr;
            error = ev.events & EPOLLERR;
            eof = ev.events & EPOLLRDHUP;
#else
            ident = (int) ev.ident;
            ev_data = (stx_event_data_t *) ev.udata;
            error = ev.flags & EV_ERROR;
            eof = ev.flags & EV_EOF;
#endif
            
            request = (stx_request_t *) ev_data->data;
            ident = request->conn;
            if (error) {
                stx_log(arg->server->logger, STX_LOG_ERR, "Event error: #%d", ident);
                continue;
            }
            
            switch (ev_data->event_type) {
                case STX_EV_FIRST_READ:
                    stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_FIRST_READ: #%d", ident);
                    //note the missing break !
                case STX_EV_READ:
                    request->close = eof;
                    
                    stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_READ: #%d (eof: %d)", ident, eof);
                    if (request->close) {
                        free(ev_data);
                        stx_request_close(request, arg->conn_pool);
                    } else {
                        ret = stx_read(arg->queue, request);
                        if (-1 == ret) {
                            stx_event(arg->queue, ident, STX_EV_READ, request);
                        } else if (0 == ret) {
                            free(ev_data);
                            stx_request_close(request, arg->conn_pool);
                        } else {
                            stx_event(arg->queue, ident, STX_EV_WRITE, request);
                        }
                    }
                    
                    break;
                case STX_EV_WRITE:
                    stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_WRITE: #%d", ident);
                    if (-1 == stx_write(arg->queue, ev_data->data)) {
                        stx_event(arg->queue, ident, STX_EV_WRITE, ev_data->data);
                    } else {
                        if (request->close) {
                            free(ev_data);
                            stx_request_close(request, arg->conn_pool);
                        } else {
                            stx_request_reset(request);
                            stx_event(arg->queue, ident, STX_EV_READ, request);
                        }
                    }
                    
                    break;
                default:
                    stx_log(arg->server->logger, STX_LOG_ERR, "Unknown event type: %d", ev_data->event_type);
                    break;
            }
        }
    }
    
    return NULL;
}
