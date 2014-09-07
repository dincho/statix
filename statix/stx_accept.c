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

void stx_accept(stx_server_t *server, stx_worker_t *workers, const int nb_workers, int *idx)
{
    struct sockaddr addr;
    socklen_t addr_size = sizeof(struct sockaddr);
    struct sockaddr_in *sin;
    char ip_str[INET_ADDRSTRLEN];
    int conn;
    stx_request_t *request;
    stx_event_t ev;
    stx_worker_t *worker;
    
    for (;;) {
        conn = accept(server->sock, &addr, &addr_size);
        if (conn < 0) {
            if (errno != EAGAIN) {
                perror("accept");
            }
            
            break;
        }
        
        *idx = (*idx + 1) % nb_workers;
        worker = &workers[*idx];
        
        
        if (worker->conn_pool->count >= server->max_connections) {
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
        
        stx_list_append(worker->conn_pool, (void *)conn);
        
        stx_event_ctl(worker->queue,
                      &ev,
                      conn,
                      STX_EVCTL_ADD | STX_EVCTL_DISPATCH,
                      STX_EVFILT_READ,
                      request);
    }
}
