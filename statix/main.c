//
//  main.c
//  statix
//
//  Created by Dincho Todorov on 2/28/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h> //stderr
#include "stx_log.h"
#include "stx_server.h"
#include "stx_listen.h"
#include "stx_accept.h"

#include <string.h>
#include <unistd.h> //getcwd
#include <errno.h>

int main(int argc, const char * argv[])
{
    stx_log_t logger;
    logger.level = STX_LOG_DEBUG;
    logger.fp = stderr;
    
    stx_server_t server;
    server.logger = &logger;
    server.port = 8000;
    server.sock = -1;
    strcpy(server.index, "index.html");
    
    if (NULL == getcwd(server.webroot, sizeof(server.webroot))) {
        perror("getcwd");
        return EXIT_FAILURE;
    }
    
    if (stx_listen(&server)) {
        return EXIT_FAILURE;
    }
    
    while (1) {
        stx_accept(&server);
    }
    
    return EXIT_SUCCESS;
}
