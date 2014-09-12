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
    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;
    struct sockaddr *addr;
    socklen_t addr_size;
    char ip_str[INET6_ADDRSTRLEN];
    int conn;
    stx_event_t ev;
    stx_worker_t *worker;
    
    if (server->pfamily == AF_INET) {
        addr = (struct sockaddr *)&sa4;
        addr_size = sizeof(struct sockaddr_in);
    } else {
        addr = (struct sockaddr *)&sa6;
        addr_size = sizeof(struct sockaddr_in6);
    }
    
    for (;;) {
        conn = accept(server->sock, addr, &addr_size);
        if (conn < 0) {
            if (errno != EAGAIN) {
                stx_log_syserr(server->logger, "accept: %s");
            }
            
            break;
        }
        
        if (server->pfamily == AF_INET) {
            inet_ntop(server->pfamily, &sa4.sin_addr, ip_str, sizeof(ip_str));
        } else {
            inet_ntop(server->pfamily, &sa6.sin6_addr, ip_str, sizeof(ip_str));
        }

        stx_log(server->logger, STX_LOG_DEBUG, "Accepted connection #%d from %s",
                conn, ip_str);
        
        if (-1 == fcntl(conn, F_SETFL, fcntl(conn, F_GETFL, 0) | O_NONBLOCK)) {
            stx_log_syserr(server->logger, "fcntl: %s");
            close(conn);
            
            continue;
        }
        
        *idx = (*idx + 1) % nb_workers;
        worker = &workers[*idx];
        
        STX_EV_SET(&ev, conn, STX_EVCTL_ADD_ONCE, STX_EVFILT_READ_ONCE);        
        if(-1 == stx_event_ctl(worker->queue, &ev, STX_EVCTL_ADD_ONCE)) {
            stx_log_syserr(server->logger, "stx_event_ctl: %s");
        }
    }
}
