//
//  stx_master_worker.h
//  statix
//
//  Created by Dincho Todorov on 9/3/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_master_worker_h
#define statix_stx_master_worker_h

#include "stx_server.h"
#include "stx_list.h"
#include "stx_worker.h"

void stx_master_worker(stx_server_t *server,
                       const int nb_threads,
                       stx_worker_t *workers);

#endif
