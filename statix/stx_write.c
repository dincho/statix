//
//  stx_write.c
//  statix
//
//  Created by Dincho Todorov on 4/24/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <sys/socket.h> //recv, send
#include <unistd.h> //read, close

#include <string.h>
#include <errno.h>

#include "stx_write.h"
#include "stx_sendfile.h"
#include "stx_event_queue.h"


int8_t stx_write(int queue, stx_request_t *req)
{
    ssize_t tx;
    off_t sendfile_tx = 0; //sendfile sent bytes, it's in/out param
    int sf_ret;
    char *headers = NULL;
    size_t headers_len = 0;
    
    if (req->fd > 0) {
        //send the header only once
        if (req->buffer_offset == 0) {
            headers = req->buff;
            headers_len = req->buffer_used;
        }
        
        sf_ret = stx_sendfile(req->fd,
                              req->conn,
                              &req->buffer_offset,
                              req->content_length,
                              &sendfile_tx,
                              headers,
                              headers_len);

        stx_log(req->server->logger, STX_LOG_DEBUG,
                "[sendfile] offset: %d, tx: %d bytes",
                req->buffer_offset,
                sendfile_tx);

        if (-1 == sf_ret) {
            if (errno == EAGAIN) {
                stx_log(req->server->logger, STX_LOG_WARN, "[sendfile] EAGAIN/EWOULDBLOCK conn:#%d, fd:#%d", req->conn,  req->fd);

                return -1;
            } else {
                perror("sendfile");
            }
        }
    } else {
        tx = send(req->conn, req->buff, req->buffer_used, 0);
        if(-1 == tx) {
            perror("send");
        }
        
        stx_log(req->server->logger, STX_LOG_DEBUG, "tx: %d bytes", tx);
    }
    
    return 0;
}
