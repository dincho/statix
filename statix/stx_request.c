//
//  stx_request.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "stx_request.h"
#include "stx_log.h"


typedef struct {
    char  *ext;
    char  *type;
} stx_content_type_t;

static stx_content_type_t content_types[] = {
    {"html", "text/html; charset=UTF-8"}, //default
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"txt", "text/plain; charset=UTF-8"}
};

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
    request->uri_start = NULL;
    request->uri_len = 0;
    request->ext_start = NULL;
    request->ext_len = 0;
    
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

            //consume everything until space and mark the extension
            case st_uri:
                switch (ch) {
                    case ' ':
                        r->uri_len = p - 1 - r->uri_start;
                        if (r->ext_start) {
                            r->ext_len = p - 1 - r->ext_start;
                        }
                        state = st_http_version;
                        break;
                    case '.':
                        r->ext_start = p;
                        break;
                    case '/':
                        r->ext_start = NULL;
                        break;
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

void stx_set_reqesut_content_type(stx_request_t *r)
{
    const int content_types_cnt = sizeof(content_types)/sizeof(stx_content_type_t);
    
    r->content_type = content_types[0].type;
    
    if (NULL == r->ext_start) {
        return;
    }
    
    for (int i = 0; i < content_types_cnt; i++) {
        if (0 == strncasecmp(r->ext_start, content_types[i].ext, r->ext_len)) {
            r->content_type = content_types[i].type;
            break;
        }
    }
}

void stx_build_response(stx_request_t *r)
{
    const char *body = "";

    if (status_body[r->status]) {
        body = status_body[r->status];
        r->content_length = strlen(body);
    }
    
    r->buffer_used = sprintf(r->buff,
            "HTTP/1.1 %d %s\r\n"
            "Server: Statix/0.1.0\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            r->status,
            response_reason_phrase[r->status],
            r->content_type,
            r->content_length,
            body);
}
