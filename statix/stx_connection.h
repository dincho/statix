//
//  stx_connection.h
//  statix
//
//  Created by Dincho Todorov on 8/30/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_connection_h
#define statix_stx_connection_h

typedef struct {
    int     current_connections;
    int     max_connections;
    int     connections[200];
} stx_connection_pool_t;

#endif
