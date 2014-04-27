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
    const size_t BUFF_SZ = 1024;
    char send_buff[BUFF_SZ];
    ssize_t tx;
    size_t send_buff_len;
    struct sf_hdtr headers = {NULL};
    struct iovec ivh;
    
    const char* response = response_templates[req->status];
    
    if (req->status != STX_STATUS_OK) {
        tx = send(req->conn, response, strlen(response), 0);
        if(-1 == tx) {
            perror("send");

        }
        
        stx_log(req->server->logger, STX_LOG_DEBUG, "TX: %d bytes", tx);
        close(req->conn);
        free(req);
        return;
    }
    
    //populate response template with content-length
    send_buff_len = snprintf(send_buff, BUFF_SZ, response, req->content_length);

    if (req->fd) {
        ivh.iov_base = &send_buff;
        ivh.iov_len = send_buff_len;
        headers.headers = &ivh;
        headers.hdr_cnt = 1;
        off_t len = 0; //send all
        
        if (sendfile(req->fd, req->conn, 0, &len, &headers, 0)) {
            //handle errors
            perror("sendfile");
            return;
        }
        
        stx_log(req->server->logger, STX_LOG_DEBUG, "Sendfile TX: %d bytes", len);
        close(req->fd);
    }
    
    close(req->conn);
    free(req);
    return;
}