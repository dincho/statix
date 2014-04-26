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
#include "stx_write.h"


void stx_write(stx_request_t *req)
{
    ssize_t tx, rx, total_rx = 0;
    const size_t BUFF_SZ = 256; //real -1 because of null
    char send_buff[BUFF_SZ];
    size_t send_buff_len;
    
    const char* response = response_templates[req->status];
    
    if (req->status == STX_STATUS_OK) {
        send_buff_len = snprintf(send_buff, BUFF_SZ, response, req->content_length);
        tx = send(req->conn, send_buff, send_buff_len, 0);
    } else {
        tx = send(req->conn, response, strlen(response), 0);
    }
    
    if (tx <= 0) {
        perror("send");
        close(req->conn);
        free(req);
        return;
    }
    
    //send file
    if (req->fd) {
        do {
            rx = read(req->fd, &send_buff, BUFF_SZ-1);
            if (-1 == rx) {
                perror("read");
                break;
            }
            
            total_rx += rx;
            
            tx = send(req->conn, send_buff, rx, 0);
            if (-1 == tx) {
                perror("send");
                break;
            }
        } while (total_rx < req->content_length);
    }
    
    close(req->fd);
    close(req->conn);
    free(req);
}