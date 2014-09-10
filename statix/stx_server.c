//
//  stx_server.c
//  statix
//
//  Created by Dincho Todorov on 4/25/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include <unistd.h> //close
#include <string.h>

#include "stx_server.h"

stx_server_t *stx_server_init(const sa_family_t family, const char *ip, const int port,
                              const char *webroot, stx_log_t *logger)
{
    stx_server_t *server;
    
    if( NULL == (server = malloc(sizeof(stx_server_t)))) {
        return NULL;
    }
    
    server->pfamily = family;
    strncpy(server->ip, ip, STX_IP_LEN);
    server->port = port;
    server->backlog = 10000;
    
    strncpy(server->index, "index.html", sizeof(server->index));
    server->index_len = strlen(server->index);
    
    strncpy(server->webroot, webroot, sizeof(server->webroot));
    server->webroot_len = strlen(server->webroot);
    
    server->logger = logger;
    
    return server;
}

void stx_server_destory(stx_server_t *server)
{
    close(server->sock);
    free(server);
}
