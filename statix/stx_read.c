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
#include "stx_dispatch.h"
#include "stx_write.h"
#include "stx_log.h"


void stx_read(stx_request_t *req)
{
    ssize_t result;
    
//    while (1) {
        result = recv(req->conn, req->buff + req->buffer_used, sizeof(req->buff) - req->buffer_used, 0);
//        if (result <= 0) { //error or done
//            break;
//        }
        stx_log(req->server->logger, STX_LOG_DEBUG, "rx: %d bytes", result);
//    }
    
    //connection closed by peer
    if (result == 0) {
        //close connection & delete event
        stx_log(req->server->logger, STX_LOG_ERR, "Connection closed by peer");
        close(req->conn);
        free(req);
        return;
    }
    
    if (result < 0) {
        if (errno == EAGAIN) {
            stx_log(req->server->logger, STX_LOG_DEBUG, "EAGAIN while reading");
            return;
        }

        perror("recv");
        //close connection & delete event
        close(req->conn);
        free(req);
        return;
    }
    
    
    //process request
    //create write event
    stx_dispatch(req);
    
    stx_write(req);
}