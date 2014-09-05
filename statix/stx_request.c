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
#include <strings.h> //strncasecmp in linux
#include <fcntl.h> //O_RDONLY
#include <sys/stat.h>
#include <errno.h>

#include "stx_request.h"
#include "stx_log.h"
#include "stx_event_queue.h"

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

stx_request_t* stx_request_init(stx_server_t *server, int conn)
{
    stx_request_t *request;
    
    if (NULL == (request = malloc(sizeof(stx_request_t)))) {
        stx_log(server->logger, STX_LOG_ERR, "cannot allocate memory for request");
        return NULL;
    }
    
    request->server = server;
    request->conn = conn;
    
    stx_request_reset(request);

    return request;
}

void stx_request_reset(stx_request_t *request)
{
    if (request->fd > 0) {
        close(request->fd);
    }

    request->close = 0;
    request->content_length = 0;
    request->fd = 0;
    request->status = STX_STATUS_NOT_IMPL;
    request->buffer_used = 0;
    request->buffer_offset = 0;
    request->uri_start = NULL;
    request->uri_len = 0;
    request->ext_start = NULL;
    request->ext_len = 0;
    request->headers_start = NULL;
}

int stx_request_parse_line(stx_request_t *r)
{
    char   ch;
    char  *p;
    
    typedef enum {
        st_start = 0,
        st_uri_start,
        st_uri,
        st_param,
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
                    case '?':
                        r->uri_len = p - 1 - r->uri_start;
                        if (r->ext_start) {
                            r->ext_len = p - 1 - r->ext_start;
                        }

                        state = (ch == '?') ? st_param : st_http_version;
                        break;
                    case '.':
                        r->ext_start = p;
                        break;
                    case '/':
                        r->ext_start = NULL;
                        break;
                }
                
                break;
                
            //consume all until space
            case st_param:
                if (ch == ' ') {
                    state = st_http_version;
                }

                break;
                
                // "HTTP/"
            case st_http_version:
                *(p-1) = '\0';
                
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
                
                r->headers_start = p;
                
                state = st_done;
                break;
                
            default:
                return -1;
                break;
        }
    }
    
    return -1;
}

long stx_request_parse_headers_line(stx_request_t *r, char *name, char **value)
{
    typedef enum {
        st_name = 0,
        st_space_before_value,
        st_value,
        st_crln,
        st_done
    } state_t;

    char   ch;
    char  *p;
    state_t state;
    
    state = st_name;
    p = name;
    
    while (state != st_done) {
        ch = *p++;
        
        switch (state) {
            case st_name:
                if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '-') {
                    state = st_name; //eat more
                } else if (ch == ':') {
                    *(p-1) = '\0';
                    state = st_space_before_value;
                } else {
                    return -1;
                }

                break;
                
            case st_space_before_value:
                if (ch != ' ') {
                    return -1;
                }
                
                *value = p;
                state = st_value;
                break;

            case st_value:
                switch (ch) {
                    case '\n':
                        *(p-1) = '\0';
                        state = st_done;
                        break;
                    case '\r':
                        *(p-1) = '\0';
                        state = st_crln;
                        break;
                    default:
                        //eat other chars as value
                        break;
                }
                break;

            case st_crln:
                if (ch != '\n') {
                    return -1;
                }

                state = st_done;
                break;

            //warning suspression
            case st_done:
                break;
        }
    }
    
    return p - name;
}

int stx_request_parse_headers(stx_request_t *r)
{
    char *buffer_end;
    char *name;
    char *value;
    long ret;
    
    buffer_end = r->buff + r->buffer_used;
    name = value = r->headers_start;
    
    while (name < buffer_end) {
        ret = stx_request_parse_headers_line(r, name, &value);
        
        if (ret == -1) {
            return -1;
        }
        
        if (0 == strcasecmp(name, "connection")) {
            if (0 == strcasecmp(value, "close")) {
                r->close = 1;
            }
            
            //currently only connection header is supported so don't go further
            return 0;
        }

        name += ret;
        
        if (name[0] == '\r' && name[1] == '\n') {
            break; //all parsed
        }
    }
    
    return 0;
}

void stx_request_process_file(stx_request_t *r)
{
    int fd;
    char filepath[255];
    struct stat sb;
    
    strcpy(filepath, r->server->webroot);
    strncat(filepath, r->uri_start, r->uri_len);
    
    //append default index file when / is requested
    if (*(r->uri_start + r->uri_len - 1) == '/') {
        strncat(filepath, r->server->index, strlen(r->server->index));
    }
    
    if ((fd = open(filepath, O_RDONLY)) == -1) {
        if (ENOENT == errno) {
            r->status = STX_STATUS_NOT_FOUND;
        } else if (EACCES == errno) {
            r->status = STX_STATUS_FORBIDDEN;
        } else {
            r->status = STX_STATUS_ERROR;
            perror("open");
        }
        
        return;
    }
    
    if (fstat(fd, &sb) == -1) {
        r->status = STX_STATUS_ERROR;
        perror("fstat");
        
        return;
    }
    
    r->status = STX_STATUS_OK;
    r->content_length = sb.st_size;
    r->fd = fd;
}

void stx_request_set_content_type(stx_request_t *r)
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

void stx_request_build_response(stx_request_t *r)
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
            "Connection: %s\r\n"
            "\r\n"
            "%s",
            r->status,
            response_reason_phrase[r->status],
            r->content_type,
            r->content_length,
            r->close ? "close" : "keep-alive",
            body);
}

void stx_request_close(stx_request_t *req, stx_list_t *conn_pool)
{
    stx_list_node_t *node;
    
    if (req->fd > 0) {
        close(req->fd);
    }
    
    stx_log(req->server->logger, STX_LOG_DEBUG, "Connection #%d closed", req->conn);

    node = stx_list_find(conn_pool, (void *)req->conn);
    if (node) {
        stx_list_remove(conn_pool, node);
    }
    
    close(req->conn);
    free(req);
}
