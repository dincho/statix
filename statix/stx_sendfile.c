//
//  stx_sendfile.c
//  statix
//
//  Created by Dincho Todorov on 9/4/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stddef.h> //NULL
#include <errno.h>
#include <sys/socket.h> //recv, send

#include "stx_sendfile.h"


#ifdef __linux

int stx_sendfile(int fd, int sock, off_t *offset, size_t count,
                 off_t *sent, char *headers, size_t headers_len)
{
    ssize_t tx;
    
    if (NULL != headers) {
        if(-1 == (tx = send(sock, headers, headers_len, 0))) {
            return -1;
        }
    }

    tx = sendfile(sock, fd, offset, count);
    
    if (-1 == tx) {
        return -1;
    } else {
        *sent = tx;
    }
    
    return 0;
}

#else //not linux

int stx_sendfile(int fd, int sock, off_t *offset, size_t count,
                 off_t *sent, char *headers, size_t headers_len)
{
    struct sf_hdtr sf_headers = {NULL, 0, NULL, 0};
    struct iovec ivh;
    int sf_ret;
    
    *sent = count;
    
    //send the header only once
    if (NULL != headers) {
        ivh.iov_base = headers;
        ivh.iov_len = headers_len;
        sf_headers.headers = &ivh;
        sf_headers.hdr_cnt = 1;
        sf_headers.trailers = NULL;
        sf_headers.trl_cnt = 0;
        
        *sent += headers_len;
    }
    
    sf_ret = sendfile(fd, sock, *offset, sent, &sf_headers, 0);
    
    //deduct headers
    if (NULL != headers) {
        *offset -= headers_len;
    }
    
    *offset += *sent;
    
    return sf_ret;
}

#endif //__linux
