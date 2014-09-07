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
    int nev, ident, error = 0, eof = 0, read;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_request_t *request;
    stx_worker_t *arg = arguments;
    int8_t ret;
    
    int read_ev = 0, write_ev = 0;
    
    struct timespec tmout = {
        5,     /* block for 5 seconds at most */
        0       /* nanoseconds */
    };
    
    stx_log(arg->server->logger, STX_LOG_DEBUG,
            "Started new worker thread - queue: %d, pool: %p", arg->queue, arg->conn_pool);
    
    for (;;) {
        nev = stx_event_wait(arg->queue, (stx_event_t *)&chlist, MAX_EVENTS, &tmout);
        
        if (nev == -1) {
            perror("stx_event_wait()");
            if (errno != EINTR) {
                break;
            }
        }
        
        if (nev == 0) { //handle timeout
            stx_log(arg->server->logger, STX_LOG_WARN,
                    "Events: %d %d %d", arg->id, read_ev, write_ev);
            continue;
        }
        
        for (int i = 0; i < nev; i++) {
            ev = chlist[i];
            
#ifdef STX_EPOLL
            error = ev.events & EPOLLERR;
            eof = ev.events & EPOLLRDHUP;
            read = (ev.events & STX_EVFILT_READ);
#else
            error = ev.flags & EV_ERROR;
            eof = ev.flags & EV_EOF;
            read = (ev.filter == STX_EVFILT_READ);
            request = (stx_request_t *) ev.udata;
#endif
            
            ident = request->conn;

            if (error) {
                stx_log(arg->server->logger, STX_LOG_ERR, "Event error: #%d", ident);
                continue;
            }
            
            if (read) {
                read_ev++;
                request->close = eof;
                
                stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_READ: #%d (eof: %d)", ident, eof);
                if (request->close) {
                    stx_request_close(request, arg->conn_pool);
                } else {
                    ret = stx_read(arg->queue, request);
                    if (-1 == ret) {
                        stx_event_ctl(arg->queue, &ev, ident,
                                      STX_EVCTL_ADD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                                      STX_EVFILT_READ, request);
                    } else if (0 == ret) {
                        stx_request_close(request, arg->conn_pool);
                    } else {
                        stx_event_ctl(arg->queue, &ev, ident,
                                      STX_EVCTL_MOD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                                      STX_EVFILT_WRITE, request);
                    }
                }
            } else { //write
                write_ev++;
                stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_WRITE: #%d", ident);
                if (-1 == stx_write(arg->queue, request)) {
                    stx_event_ctl(arg->queue, &ev, ident,
                                  STX_EVCTL_MOD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                                  STX_EVFILT_WRITE, request);
                } else {
                    if (request->close) {
                        stx_request_close(request, arg->conn_pool);
                    } else {
                        stx_request_reset(request);
                        stx_event_ctl(arg->queue, &ev, ident,
                                      STX_EVCTL_ADD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                                      STX_EVFILT_READ, request);
                    }
                }
            }
        }
    }
    
    return NULL;
}
