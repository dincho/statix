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
#include "stx_worker.h"


int main(int argc, const char * argv[])
{
    const int NB_THREADS = 4;
    pthread_t threads[NB_THREADS];

    stx_log_t logger;
    logger.level = STX_LOG_DEBUG;
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
    strncpy(server.index, "index.html", sizeof(server.index));
    
    if (NULL == getcwd(server.webroot, sizeof(server.webroot))) {
        perror("getcwd");
        return EXIT_FAILURE;
    }

    if (stx_listen(&server)) {
        return EXIT_FAILURE;
    }
    
    for (int i = 0; i < NB_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, stx_worker, &server)) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }
    
    for (int i = 0; i < NB_THREADS; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

