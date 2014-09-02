//
//  stx_accept.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <unistd.h> //getpid, close
#include <errno.h>

#include <netinet/in.h> /* For sockaddr_in */
#include <sys/socket.h> /* For socket functions */
#include <netdb.h> //getnameinfo
#include <sys/fcntl.h>
#include <arpa/inet.h>

#include "stx_accept.h"
#include "stx_request.h"
#include "stx_read.h"
#include "stx_log.h"
#include "stx_event_queue.h"

void stx_accept(int queue, stx_server_t *server, stx_list_t *conn_pool)
{
    struct sockaddr addr;
    socklen_t addr_size = sizeof(struct sockaddr);
    struct sockaddr_in *sin;
    char ip_str[INET_ADDRSTRLEN];
    int conn;
    stx_request_t *request;
    
    for (;;) {
        conn = accept(server->sock, &addr, &addr_size);
        if (conn < 0) {
            if (errno != EAGAIN) {
                perror("accept");
            }
            
            break;
        }

        if (conn_pool->count >= server->max_connections) {
            stx_log(server->logger,
                    STX_LOG_ERR,
                    "Connection limit %d reached, closing #%d",
                    server->max_connections,
                    conn);

            close(conn);

            break;
        }
        
        sin = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
        
        stx_log(server->logger, STX_LOG_INFO, "Accepted connection #%d from %s",
                conn, ip_str);
        
        if (-1 == fcntl(conn, F_SETFL, fcntl(conn, F_GETFL, 0) | O_NONBLOCK)) {
            perror("fcntl");
            close(conn);
            
            continue;
        }
        
        request = stx_request_init(server, conn);
        if (NULL == request) {
            stx_log(server->logger, STX_LOG_ERR,
                    "Error while initializing request");
            close(conn);
            
            continue;
        }
        
        stx_list_append(conn_pool, (void *)conn);

        stx_event(queue, conn, STX_EV_READ, request);
    }
}
