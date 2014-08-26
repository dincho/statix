//
//  stx_accept.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <unistd.h> //getpid, close

#include <netinet/in.h> /* For sockaddr_in */
#include <sys/socket.h> /* For socket functions */
#include <netdb.h> //getnameinfo
#include <sys/fcntl.h>

#include "stx_accept.h"
#include "stx_request.h"
#include "stx_dispatch.h"
#include "stx_read.h"
#include "stx_log.h"
#include "stx_event_queue.h"


void stx_accept(int queue, stx_server_t *server)
{
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int conn, rc;
    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    stx_request_t *request;
    
    conn = accept(server->sock, (struct sockaddr*)&ss, &slen);
    if (conn < 0) {
        perror("accept");
        return;
    }
    
    if (-1 == fcntl(conn, F_SETFL, fcntl(conn, F_GETFL, 0) | O_NONBLOCK)) {
        perror("fcntl");
        return;
    }
    
    //getaddrinfo ?
    rc = getnameinfo((struct sockaddr *)&ss, slen,
                         host, sizeof(host), port, sizeof(port),
                         NI_NUMERICHOST | NI_NUMERICSERV);
    if (rc == 0) {
        stx_log(server->logger, STX_LOG_INFO,
                "Accepted connection from %s:%s", host, port);
    }
    
    request = stx_init_request(server, conn);
    if (NULL == request) {
        stx_log(server->logger, STX_LOG_ERR,
                "Error while initializing request");
        return;
    }

    stx_event(queue, conn, STX_EV_READ, request);
}