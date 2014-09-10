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
static const int OPEN_FILES_CACHE_CAPACITY = 16;

static int8_t stx_handle_read_event(stx_hashmap_t *conn_pool, stx_list_t *request_pool, stx_event_t *ev,
                                    stx_server_t *server, stx_hashmap_t *open_files, int max_connections);

static int8_t stx_handle_write_event(stx_hashmap_t *conn_pool, stx_list_t *request_pool, stx_event_t *ev);

void *stx_worker(void *arguments)
{
    int nev, ident, read_ev = 0, write_ev = 0;
    stx_event_t chlist[MAX_EVENTS];
    stx_event_t ev;
    stx_worker_t *arg = arguments;

    stx_hashmap_t *conn_pool = stx_hashmap_init(arg->max_connections);
    
    
    stx_hashmap_t *open_files = stx_hashmap_init(OPEN_FILES_CACHE_CAPACITY);
    stx_list_t *request_pool = stx_list_init();
    
    stx_request_t *request;
    
    for (int r = 0; r < arg->max_connections; r++) {
        request = malloc(sizeof(stx_request_t));
        if (NULL == request) {
            perror("malloc (request pool)");
            break;
        }
        
        stx_list_push(request_pool, request);
        stx_hashmap_put(conn_pool, r+1, NULL); //preallocate map buckets
    }

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
            ident = STX_EV_IDENT(ev);
            
            if (STX_EV_ERROR(ev)) {
                stx_log(arg->server->logger, STX_LOG_ERR,
                        "Event error: #%d", ident);
                continue;
            }
            
            if (STX_EV_READ_ONCE(ev)) {
                read_ev++;
                stx_log(arg->server->logger, STX_LOG_DEBUG,
                        "STX_EV_READ: #%d (eof: %d)", ident, STX_EV_EOF(ev));

                if (stx_handle_read_event(conn_pool, request_pool, &ev, arg->server, open_files, arg->max_connections)) {
                    stx_event_ctl(arg->queue, &ev, STX_EVCTL_MOD_ONCE);
                }
            } else { //write
                write_ev++;
                stx_log(arg->server->logger, STX_LOG_DEBUG,
                        "STX_EV_WRITE: #%d", ident);
                
                if (stx_handle_write_event(conn_pool, request_pool, &ev)) {
                    stx_event_ctl(arg->queue, &ev, STX_EVCTL_MOD_ONCE);
                }
            } //end read/write
        }
        
        stx_log_flush(arg->server->logger);
    }
    
    return NULL;
}

static int8_t stx_handle_read_event(stx_hashmap_t *conn_pool, stx_list_t *request_pool, stx_event_t *ev,
                                    stx_server_t *server, stx_hashmap_t *open_files, int max_connections)
{
    int ident, ret;
    stx_request_t *request;
    
    ident = STX_EV_IDENT((*ev));
    request = stx_hashmap_get(conn_pool, ident);
    
    if (STX_EV_EOF((*ev))) {
        if (NULL != request) {
            stx_request_close(request_pool, request);
        }
        
        stx_hashmap_put(conn_pool, ident, NULL);
        
        return 0;
    }
    
    //first read after accept - init the request
    if (NULL == request) {
        if (conn_pool->elcount >= max_connections) {
            stx_log(server->logger,
                    STX_LOG_ERR,
                    "Connection limit %d reached, closing #%d",
                    max_connections,
                    ident);
            
            close(ident);
            return 0;
        }
        
        request = stx_request_init(request_pool, server, ident);
        if (NULL == request) {
            stx_log(server->logger, STX_LOG_ERR,
                    "Error while initializing request");
            
            close(ident);

            return 0;
        }
        
        stx_hashmap_put(conn_pool, ident, request);
    }
    
    ret = stx_read(request);
    
    if (-1 == ret) {
        STX_EV_SET(ev, ident, STX_EVCTL_MOD_ONCE, STX_EVFILT_READ_ONCE);
        
        return 1;
    }
    
    if (0 == ret) {
        stx_request_close(request_pool, request);
        stx_hashmap_put(conn_pool, ident, NULL);
        
        return 0;
    }
    
    //process request
    stx_request_process(request, open_files);
    STX_EV_SET(ev, ident, STX_EVCTL_MOD_ONCE, STX_EVFILT_WRITE_ONCE);
    
    return 1;
}

static int8_t stx_handle_write_event(stx_hashmap_t *conn_pool, stx_list_t *request_pool, stx_event_t *ev)
{
    int ident;
    stx_request_t *request;
    
    ident = STX_EV_IDENT((*ev));
    request = stx_hashmap_get(conn_pool, ident);
    
    if (-1 == stx_write(request)) {
        STX_EV_SET(ev, ident, STX_EVCTL_MOD_ONCE, STX_EVFILT_WRITE_ONCE);

        return 1;
    }
    
    if (request->close) {
        stx_request_close(request_pool, request);
        stx_hashmap_put(conn_pool, ident, NULL);
        
        return 0;
    }
    
    stx_request_reset(request);
    STX_EV_SET(ev, ident, STX_EVCTL_MOD_ONCE, STX_EVFILT_READ_ONCE);
    
    return 1;
}
