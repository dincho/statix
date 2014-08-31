//
//  stx_conn_pool.c
//  statix
//
//  Created by Dincho Todorov on 9/1/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include "stx_conn_pool.h"
#include "stx_log.h"

stx_conn_pool_t *stx_conn_pool_init(int max)
{
    stx_conn_pool_t *pool;
    stx_list_t *list;
    
    if (NULL == (pool = malloc(sizeof(stx_conn_pool_t)))) {
        return NULL;
    }
    
    if (NULL == (list = stx_list_init())) {
        return NULL;
    }
    
    pool->max = max;
    pool->connections = list;
    
    return pool;
}

void stx_conn_pool_destroy(stx_conn_pool_t *pool)
{
    stx_list_destroy(pool->connections);
    free(pool);
}

int stx_conn_pool_full(stx_conn_pool_t *pool)
{
    return (pool->connections->count >= pool->max);
}

int stx_conn_pool_add(stx_conn_pool_t *pool, int fd)
{
    if (stx_conn_pool_full(pool)) {
        return -1;
    }
    
    stx_list_append(pool->connections, (void *)fd);

    return 0;
}

int stx_conn_pool_remove(stx_conn_pool_t *pool, int fd)
{
    stx_list_node_t *node;
    if (NULL == (node = stx_list_find(pool->connections, (void *)fd))) {
        return -1;
    }
    
    stx_list_remove(pool->connections, node);
    
    return 0;
}
