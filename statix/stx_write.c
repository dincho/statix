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


void stx_write(stx_request_t *req)
{
    struct sf_hdtr headers = {NULL};
    struct iovec ivh;
    ssize_t tx;
    off_t sendfile_tx = 0; //sendfile sent bytes, it's in/out param

    if (req->fd > 0) {
        ivh.iov_base = req->buff;
        ivh.iov_len = req->buffer_used;
        headers.headers = &ivh;
        headers.hdr_cnt = 1;
        
        if (sendfile(req->fd, req->conn, 0, &sendfile_tx, &headers, 0)) {
            perror("sendfile");
        }
        
        stx_log(req->server->logger, STX_LOG_DEBUG, "Sendfile TX: %d bytes", sendfile_tx);
    } else {
        tx = send(req->conn, req->buff, req->buffer_used, 0);
        if(-1 == tx) {
            perror("send");
        }
        
        stx_log(req->server->logger, STX_LOG_DEBUG, "TX: %d bytes", tx);
    }
    
    stx_close_request(req);
}
