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
#include <arpa/inet.h> //inet_addr
#include <sys/fcntl.h>

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

int stx_server_listen(stx_server_t *server)
{
    int fd;
    struct sockaddr_in sin4;
    struct sockaddr_in6 sin6;
    
    if (server->pfamily == AF_INET) {
        sin4.sin_family = AF_INET;
        sin4.sin_port = htons(server->port);
        inet_pton(AF_INET, server->ip, &(sin4.sin_addr));
    } else {
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = htons(server->port);
        inet_pton(AF_INET6, server->ip, &(sin6.sin6_addr));
    }
    
    fd = socket(server->pfamily, SOCK_STREAM, 0);
    if (-1 == fd) {
        stx_log_syserr(server->logger, "socket: %s");
        return -1;
    }
    
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) {
        stx_log_syserr(server->logger, "fcntl: %s");
        return -1;
    }
    
    int reusesocket = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reusesocket, sizeof(int));
    
    stx_log(server->logger, STX_LOG_INFO, "Listen %s:%d", server->ip, server->port);
    
    if (server->pfamily == AF_INET) {
        if (bind(fd, (struct sockaddr *)&sin4, sizeof(sin4)) < 0) {
            stx_log_syserr(server->logger, "bind: %s");
            return -1;
        }
    } else {
        if (bind(fd, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
            stx_log_syserr(server->logger, "bind: %s");
            return -1;
        }
    }
    
    if (listen(fd, server->backlog) < 0) {
        stx_log_syserr(server->logger, "listen: %s");
        return -1;
    }
    
    server->sock = fd;
    
    return 0;
}
