//
//  stx_accept.h
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_accept_h
#define statix_stx_accept_h

#include "stx_server.h"
#include "stx_list.h"

void stx_accept(int, stx_server_t *, stx_list_t *conn_pool);

#endif