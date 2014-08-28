//
//  stx_listen.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <netinet/in.h> /* For sockaddr_in */
#include <sys/socket.h> /* For socket functions */
#include <stdio.h> //perror
#include <sys/fcntl.h>

#include "stx_listen.h"
#include "stx_event_queue.h"

int stx_listen(int queue, stx_server_t *server)
{
    int fd;
    struct sockaddr_in sin;
    
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(server->port);
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
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
    
    if (bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return -1;
    }
    
    if (listen(fd, 10000) < 0) {
        perror("listen");
        return -1;
    }
    
    server->sock = fd;
    
    stx_log(server->logger, STX_LOG_INFO, "Listening for new connections....");
    
    return stx_event(queue, fd, STX_EV_ACCEPT, server);
}