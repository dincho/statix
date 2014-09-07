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
#include <unistd.h> //close()

#include "stx_worker.h"
#include "stx_event_queue.h"
#include "stx_accept.h"
#include "stx_read.h"
#include "stx_write.h"
#include "stx_log.h"
#include "stx_hashmap.h"

static const int MAX_EVENTS = 1024; //x 32b = 32Kb

void *stx_worker(void *arguments)
{
    int nev, ident, error = 0, eof = 0, read, read_ev = 0, write_ev = 0;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_request_t *request;
    stx_worker_t *arg = arguments;
    int8_t ret;

    stx_hashmap_t *conn_pool = stx_hashmap_init(arg->server->max_connections);
    
    struct timespec tmout = {
        5,     /* block for 5 seconds at most */
        0       /* nanoseconds */
    };
    
    stx_log(arg->server->logger, STX_LOG_DEBUG,
            "Started new worker thread - queue: %d, pool: %p", arg->queue, conn_pool);
    
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
        }
        
        for (int i = 0; i < nev; i++) {
            ev = chlist[i];
            
#ifdef STX_EPOLL
            error = ev.events & EPOLLERR;
            eof = ev.events & EPOLLRDHUP;
            read = (ev.events & STX_EVFILT_READ);
            request = (stx_request_t *) ev.data.ptr;
            ident = ev.data.fd;
#else
            error = ev.flags & EV_ERROR;
            eof = ev.flags & EV_EOF;
            read = (ev.filter == STX_EVFILT_READ);
            request = (stx_request_t *) ev.udata;
            ident = (int) ev.ident;
#endif
            
//            if (request) {
//                ident = request->conn;
//            }

            if (error) {
                stx_log(arg->server->logger, STX_LOG_ERR, "Event error: #%d", ident);
                continue;
            }
            
            if (read) {
                read_ev++;
                
                //first read after accept - init the request
                if (NULL == stx_hashmap_get(conn_pool, ident)) {
                    if (conn_pool->elcount >= arg->server->max_connections) {
                        stx_log(arg->server->logger,
                                STX_LOG_ERR,
                                "Connection limit %d reached, closing #%d",
                                arg->server->max_connections,
                                ident);
                        
                        close(ident);
                        continue;
                    }
                    
                    request = stx_request_init(arg->server, ident);
                    if (NULL == request) {
                        stx_log(arg->server->logger, STX_LOG_ERR,
                                "Error while initializing request");
                        
                        close(ident);
                        continue;
                    }
                    
                    stx_hashmap_put(conn_pool, ident, request);
                }

                stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_READ: #%d (eof: %d)", ident, eof);

                if (eof) {
                    stx_request_close(request);
                    stx_hashmap_put(conn_pool, request->conn, NULL);
                    
                    continue;
                }

                ret = stx_read(arg->queue, request);

                if (-1 == ret) {
                    stx_event_ctl(arg->queue, &ev, ident,
                                  STX_EVCTL_ADD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                                  STX_EVFILT_READ, request);
                    continue;
                }
                
                if (0 == ret) {
                    stx_request_close(request);
                    stx_hashmap_put(conn_pool, request->conn, NULL);
                    
                    continue;
                }

                stx_event_ctl(arg->queue, &ev, ident,
                              STX_EVCTL_MOD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                              STX_EVFILT_WRITE, request);
            } else { //write
                write_ev++;
                stx_log(arg->server->logger, STX_LOG_DEBUG, "STX_EV_WRITE: #%d", ident);
                if (-1 == stx_write(arg->queue, request)) {
                    stx_event_ctl(arg->queue, &ev, ident,
                                  STX_EVCTL_MOD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                                  STX_EVFILT_WRITE, request);
                    continue;
                }
                
                if (request->close) {
                    stx_request_close(request);
                    stx_hashmap_put(conn_pool, request->conn, NULL);
                    
                    continue;
                }
                
                stx_request_reset(request);
                stx_event_ctl(arg->queue, &ev, ident,
                              STX_EVCTL_ADD | STX_EVCTL_DISPATCH | STX_EVCTL_ENABLE,
                              STX_EVFILT_READ, request);
            } //end read/write
        }
    }
    
    return NULL;
}
