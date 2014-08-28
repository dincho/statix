//
//  stx_request.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include "stx_request.h"
#include "stx_log.h"


stx_request_t* stx_init_request(stx_server_t *server, int conn)
{
    stx_request_t *request;
    
    if (NULL == (request = malloc(sizeof(stx_request_t)))) {
        stx_log(server->logger, STX_LOG_ERR, "cannot allocate memory for request");
        return NULL;
    }
    
    request->server = server;
    request->conn = conn;
    request->content_length = 0;
    request->fd = 0;
    request->status = STX_STATUS_NOT_IMPL;
    request->buffer_used = 0;
    request->uri_len = 0;
    
    //@todo init buffers & events
    return request;
}

int stx_parse_request_line(stx_request_t *r)
{
    char   ch;
    char  *p;
    
    typedef enum {
        st_start = 0,
        st_uri_start,
        st_uri,
        st_http_version,
        st_first_major_digit,
        st_major_digit,
        st_first_minor_digit,
        st_minor_digit,
        st_crln,
        st_done
    } state_t;
    
    state_t state = st_start;
    
    p = r->buff;
    
    while (state != st_done) {
        ch = *p++;
        
        switch (state) {
            case st_start:
                switch (ch) {
                    case 'G':
                        if (*p != 'E' || *(p + 1) != 'T' || *(p + 2) != ' ') {
                            return -1;
                        }
                        
                        r->method = STX_METHOD_GET;
                        p += 3;
                        break;
                        
                    default:
                        return -1;
                        break;
                }
                
                state = st_uri_start;
                break;
                
            case st_uri_start:
                if ('/' != ch) {
                    return -1;
                }
                
                r->uri_start = p - 1;
                state = st_uri;
                break;
                
                //consume everything until space
            case st_uri:
                if (' ' == ch) {
                    r->uri_len = p - 1 - r->uri_start;
                    state = st_http_version;
                }
                
                break;
                
                // "HTTP/"
            case st_http_version:
                
                if (ch != 'H' || *p != 'T' || *(p + 1) != 'T' || *(p + 2) != 'P' || *(p + 3) != '/') {
                    return -1;
                }
                
                p += 4;
                state = st_first_major_digit;
                break;
                
            case st_first_major_digit:
                if (ch < '0' || ch > '9') {
                    return -1;
                }
                
                r->major = ch - '0';
                state = st_major_digit;
                break;
                
            case st_major_digit:
                if ('.' == ch) {
                    state = st_first_minor_digit;
                    break;
                }
                
                if (ch < '0' || ch > '9') {
                    return -1;
                }
                
                r->major = r->major * 10 + ch - '0';
                break;
                
            case st_first_minor_digit:
                if (ch < '0' || ch > '9') {
                    return -1;
                }
                
                r->minor = ch - '0';
                state = st_minor_digit;
                break;
                
            case st_minor_digit:
                if ('\r' == ch) {
                    state = st_crln;
                    break;
                }
                
                if (ch < '0' || ch > '9') {
                    return -1;
                }
                
                r->minor = r->minor * 10 + ch - '0';
                break;
                
            case st_crln:
                //set request end pointer or headers start
                if ('\n' != ch) {
                    return -1;
                }
                
                state = st_done;
                break;
                
            default:
                return -1;
                break;
        }
    }
    
    return -1;
}

void stx_close_request(stx_request_t *req)
{
    if (req->fd > 0) {
        close(req->fd);
    }

    close(req->conn);
    free(req);
}
