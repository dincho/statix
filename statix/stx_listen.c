//
//  stx_listen.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <netinet/in.h> /* For sockaddr_in */
#include <arpa/inet.h> //inet_addr
#include <sys/socket.h> /* For socket functions */
#include <stdio.h> //perror
#include <sys/fcntl.h>

#include "stx_listen.h"
#include "stx_event_queue.h"

int stx_listen(stx_server_t *server)
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
        perror("socket");
        return -1;
    }
    
    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
    
    int reusesocket = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reusesocket, sizeof(int));
    
    stx_log(server->logger, STX_LOG_WARN, "Listen %s:%d", server->ip, server->port);
    
    if (server->pfamily == AF_INET) {
        if (bind(fd, (struct sockaddr *)&sin4, sizeof(sin4)) < 0) {
            perror("bind");
            return -1;
        }
    } else {
        if (bind(fd, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
            perror("bind");
            return -1;
        }
    }
    
    if (listen(fd, server->backlog) < 0) {
        perror("listen");
        return -1;
    }
    
    server->sock = fd;
    
    return 0;
}
