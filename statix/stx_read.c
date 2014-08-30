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

void stx_read(int queue, stx_request_t *req)
{
    ssize_t rx, buff_sz;
    
    buff_sz = sizeof(req->buff) - req->buffer_used;

    //this should set request status and schedule write ev
    if (buff_sz == 0) {
        stx_log(req->server->logger, STX_LOG_ERR, "Request too long");
        req->status = STX_STATUS_URI_TOO_LONG;

        stx_event(queue, req->conn, STX_EV_WRITE, req);
        
        return;
    }

    rx = recv(req->conn, req->buff + req->buffer_used, buff_sz, 0);
    
    if (rx > 0) {
        req->buffer_used += rx;

        stx_log(req->server->logger, STX_LOG_DEBUG, "rx: %d bytes", rx);
        
        //process request
        stx_request_parse_line(req);
        stx_request_set_content_type(req);
        stx_request_process_file(req);
        stx_request_build_response(req);

        stx_event(queue, req->conn, STX_EV_WRITE, req);
    }

    //connection closed by peer
    if (rx == 0) {
        stx_log(req->server->logger, STX_LOG_ERR, "Connection closed by peer");
        stx_event(queue, req->conn, STX_EV_CLOSE, req);
    }
    
    if (rx == -1) {
        if (errno == EAGAIN) {
            stx_log(req->server->logger, STX_LOG_WARN, "EAGAIN while reading #%d", req->conn);
            stx_event(queue, req->conn, STX_EV_READ, req);
        } else {
            perror("recv");
            stx_event(queue, req->conn, STX_EV_CLOSE, req);
        }
    }
}
