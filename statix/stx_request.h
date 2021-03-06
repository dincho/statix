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

#include "config.h"
#include "stx_server.h"
#include "stx_list.h"
#include "stx_hashmap.h"

typedef enum {
    STX_METHOD_GET = 1,
    STX_METHOD_PUT
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
    
    char            *ext_start;
    size_t          ext_len;
    
    char            buff[STX_REQUEST_BUFF_SZ]; //8K
    size_t          buffer_used;
    off_t           buffer_offset;
    
    char            *headers_start;

    int             close;

    //response
    int                 fd;
    response_status_t   status;
    size_t              content_length;
    const char          *content_type;
} stx_request_t;

static const char * const status_body[] = {
    [STX_STATUS_OK] = NULL,
    [STX_STATUS_BAD_REQ] = NULL,
    [STX_STATUS_FORBIDDEN] = NULL,
    [STX_STATUS_NOT_FOUND] =
        "<html>\n"
        " <body>\n"
        " <h1>Error 404 (Not Found)</h1>\n"
        " <p>The requested URL was not found on this server.</p>\n"
        " </body>\n"
        "</html>\n",
    [STX_STATUS_URI_TOO_LONG] =
        "<html>\n"
        " <body>\n"
        " <h1>Request-URI Too Long</h1>\n"
        " </body>\n"
        "</html>\n",
    [STX_STATUS_ERROR] =
        "<html>\n"
        " <body>\n"
        " <h1>Internal Server Error</h1>\n"
        " </body>\n"
        "</html>\n",
    [STX_STATUS_NOT_IMPL] =
        "<html>\n"
        " <body>\n"
        " <h1>Method Not Implemented</h1>\n"
        " <p>This method is not implemented by this server.</p>\n"
        " </body>\n"
        "</html>\n",
};

static const char * const response_reason_phrase[] = {
    [STX_STATUS_OK] = "OK",
    [STX_STATUS_BAD_REQ] = "Bad Request",
    [STX_STATUS_FORBIDDEN] = "Forbidden",
    [STX_STATUS_NOT_FOUND] = "Not Found",
    [STX_STATUS_URI_TOO_LONG] = "Request-URI Too Long",
    [STX_STATUS_ERROR] = "Internal Server Error",
    [STX_STATUS_NOT_IMPL] = "Not Implemented"
};

stx_request_t* stx_request_init(stx_list_t *request_pool, stx_server_t *, int conn);
void stx_request_reset(stx_request_t *request);
void stx_request_close(stx_list_t *request_pool, stx_request_t *);
void stx_request_process(stx_request_t *request, stx_hashmap_t *open_files);


#endif
