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
#include "stx_worker.h"

void stx_accept(stx_server_t *server, stx_worker_t *workers, const int nb_workers, int *index);

#endif
