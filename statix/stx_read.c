//
//  stx_read.c
//  statix
//
//  Created by Dincho Todorov on 4/21/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <sys/socket.h> //recv
#include <stdio.h> //perror
#include <errno.h>
#include <unistd.h> //close

#include "stx_read.h"
#include "stx_log.h"
#include "stx_event_queue.h"

/*
 * returns 1 when read is done
 * returns 0 if error or EOF (so connection can be closed)
 * returns -1 if EAGAIN
 */

int8_t stx_read(int queue, stx_request_t *req)
{
    ssize_t rx, buff_sz;
    
    buff_sz = sizeof(req->buff) - req->buffer_used;

    //request buffer is too small, drop the request
    if (buff_sz == 0) {
        stx_log(req->server->logger, STX_LOG_ERR, "Request too big");
        req->status = STX_STATUS_BAD_REQ;

        return 1;
    }

    rx = recv(req->conn, req->buff + req->buffer_used, buff_sz, 0);
    
    if (rx > 0) {
        req->buffer_used += rx;

        stx_log(req->server->logger, STX_LOG_DEBUG, "rx: %d bytes", rx);

        return 1;
    }

    //connection closed by peer
    if (rx == 0) {
        stx_log(req->server->logger, STX_LOG_INFO, "Connection closed by peer");
        req->close = 1;
        
        return 0;
    }
    
    if (rx == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            stx_log(req->server->logger,
                    STX_LOG_WARN,
                    "EAGAIN/EWOULDBLOCK while reading #%d",
                    req->conn);
            
            return -1;
        } else {
            perror("recv");
            
            return 0;
        }
    }
    
    return 0;
}
