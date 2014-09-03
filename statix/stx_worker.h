//
//  stx_worker.h
//  statix
//
//  Created by Dincho Todorov on 8/30/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_worker_h
#define statix_stx_worker_h

#include "stx_server.h"
#include "stx_list.h"

typedef struct {
    stx_server_t    *server;
    stx_list_t      *conn_pool;
    int             queue;
} stx_worker_t;

void *stx_worker(void *arg);

#endif
