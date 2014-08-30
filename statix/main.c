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

#include "stx_log.h"
#include "stx_server.h"
#include "stx_listen.h"
#include "stx_worker.h"


int main(int argc, const char * argv[])
{
    stx_log_t logger;
    logger.level = STX_LOG_DEBUG;
    logger.fp = stderr;
    
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
    
    stx_worker(&server);

    return EXIT_SUCCESS;
}
