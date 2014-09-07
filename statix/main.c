//
//  main.c
//  statix
//
//  Created by Dincho Todorov on 2/28/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h> //exit status
#include <stdio.h> //stderr
#include <string.h> //strcpy
#include <unistd.h> //getcwd
#include <pthread.h>

#include "stx_log.h"
#include "stx_server.h"
#include "stx_listen.h"
#include "stx_list.h"
#include "stx_event_queue.h"
#include "stx_master_worker.h"
#include "stx_worker.h"
#include "stx_accept.h"


int main(int argc, const char * argv[])
{
    const int NB_THREADS = 1;

    int queues[NB_THREADS];
    pthread_t threads[NB_THREADS];
    stx_worker_t workers[NB_THREADS];
    stx_list_t *conn_pools[NB_THREADS];

    stx_log_t logger;
    logger.level = STX_LOG_WARN;
    logger.fp = stderr;

    if (pthread_mutex_init(&logger.mutex, NULL)) {
        perror("pthread_mutex_init");
        return EXIT_FAILURE;
    }

    stx_server_t server;
    server.logger = &logger;
    server.port = 8000;
    server.backlog = 10000;
    server.sock = -1;
    server.max_connections = 1000;
    
    strncpy(server.index, "index.html", sizeof(server.index));
    
    if (NULL == getcwd(server.webroot, sizeof(server.webroot))) {
        perror("getcwd");
        return EXIT_FAILURE;
    }

    if (stx_listen(&server)) {
        return EXIT_FAILURE;
    }
    
    for (int i = 0; i < NB_THREADS; i++) {
        workers[i].server = &server;
        workers[i].id = i+1;

        if (NULL == (workers[i].conn_pool = stx_list_init())) {
            stx_log(server.logger, STX_LOG_ERR, "Cannot create connection pool");
            return EXIT_FAILURE;
        }
        
        if ((workers[i].queue = stx_queue_create()) == -1) {
            perror("stx_queue_create");
            return EXIT_FAILURE;
        }

        if (pthread_create(&threads[i], NULL, stx_worker, &workers[i])) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }
    
    stx_master_worker(&server, NB_THREADS, workers); //master loop
    
    for (int i = 0; i < NB_THREADS; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }
        
        stx_queue_close(queues[i]);
        stx_list_destroy(conn_pools[i]);
    }

    return EXIT_SUCCESS;
}

