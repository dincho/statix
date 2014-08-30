//
//  stx_server.h
//  statix
//
//  Created by Dincho Todorov on 4/25/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_server_h
#define statix_stx_server_h

#include "stx_log.h"

typedef struct {
    stx_log_t   *logger;
    int         port;
    int         backlog;
    char        webroot[1024];
    char        index[64];
    int         sock;
} stx_server_t;

#endif
