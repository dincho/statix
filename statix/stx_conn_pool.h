//
//  stx_connection.h
//  statix
//
//  Created by Dincho Todorov on 8/30/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_connection_h
#define statix_stx_connection_h

#include "stx_list.h"

typedef struct {
    int             max;
    stx_list_t      *connections;
} stx_conn_pool_t;

stx_conn_pool_t *stx_conn_pool_init();
void stx_conn_pool_destroy(stx_conn_pool_t *pool);
int stx_conn_pool_full(stx_conn_pool_t *pool);
int stx_conn_pool_add(stx_conn_pool_t *pool, int fd);
int stx_conn_pool_remove(stx_conn_pool_t *pool, int fd);

#endif
