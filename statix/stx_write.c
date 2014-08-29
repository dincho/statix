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
#include "stx_event_queue.h"

void stx_write(int queue, stx_request_t *req)
{
    struct sf_hdtr headers = {NULL};
    struct iovec ivh;
    ssize_t tx;
    off_t sendfile_tx = 0; //sendfile sent bytes, it's in/out param
    int sf_ret;

    if (req->fd > 0) {
        //send the header only once
        if (req->buffer_offset == 0) {
            ivh.iov_base = req->buff;
            ivh.iov_len = req->buffer_used;
            headers.headers = &ivh;
            headers.hdr_cnt = 1;
        }
        
        sf_ret = sendfile(req->fd, req->conn, req->buffer_offset, &sendfile_tx, &headers, 0);
        stx_log(req->server->logger, STX_LOG_DEBUG,
                "[sendfile] offset: %d, tx: %d bytes",
                req->buffer_offset,
                sendfile_tx);

        if (sf_ret) {
            if (errno == EAGAIN) {
                //deduct headers
                if (req->buffer_offset == 0) {
                    req->buffer_offset -= req->buffer_used;
                }

                req->buffer_offset += sendfile_tx;
                stx_event(queue, req->conn, STX_EV_WRITE, req);
                return;
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
    
    stx_close_request(req);
}
