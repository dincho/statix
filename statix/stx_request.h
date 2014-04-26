//
//  stx_request.h
//  statix
//
//  Created by Dincho Todorov on 3/9/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_request_h
#define statix_stx_request_h

#include <stdlib.h> //size_t
#include "stx_server.h"

typedef enum {
    STX_METHOD_GET = 1,
    STX_METHOD_POST
} method_t;

typedef enum {
    STX_STATUS_OK = 200,
    STX_STATUS_BAD_REQ = 400,
    STX_STATUS_FORBIDDEN = 403,
    STX_STATUS_NOT_FOUND = 404,
    STX_STATUS_URI_TOO_LONG = 414,
    STX_STATUS_ERROR = 500,
    STX_STATUS_NOT_IMPL = 501
} response_status_t;

typedef struct {
    stx_server_t    *server;
    int             conn;
    method_t        method;
    int             major;
    int             minor;
    char            *uri_start;
    size_t          uri_len;
    
    char            buff[8192]; //8K
    size_t          buffer_used;
    
    //response
    int                     fd;
    response_status_t       status;
    size_t                  content_length;
} stx_request_t;

static const char * const response_templates[] = {
    [STX_STATUS_OK] =
        "HTTP/1.0 200 OK\r\n"
        "Server: Statix/0.1.0\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "\r\n",
    [STX_STATUS_NOT_FOUND] =
        "HTTP/1.0 404 Not found\r\n"
        "Server: Statix/0.1.0\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: 120\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>\n"
        " <body>\n"
        " <h1>Error 404 (Not Found)</h1>\n"
        " <p>The requested URL was not found on this server.</p>\n"
        " </body>\n"
        "</html>\n",
    [STX_STATUS_NOT_IMPL] =
        "HTTP/1.0 501 Method Not Implemented\r\n"
        "Server: Statix/0.1.0\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: 120\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>\n"
        " <body>\n"
        " <h1>Method Not Implemented</h1>\n"
        " <p>This method is not implemented by this server.</p>\n"
        " </body>\n"
        "</html>\n"
};

stx_request_t* stx_init_request(stx_server_t *, int conn);
int stx_parse_request_line(stx_request_t *);
void stx_close_request(stx_request_t *);

#endif
