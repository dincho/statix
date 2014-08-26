//
//  stx_dispatch.c
//  statix
//
//  Created by Dincho Todorov on 3/17/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <fcntl.h> //O_RDONLY
#include <sys/stat.h>
#include <errno.h>
#include "stx_dispatch.h"


void stx_dispatch(stx_request_t *r)
{
    size_t namelen;
    char *filename;
    int fd;
    struct stat sb;
    
    stx_parse_request_line(r);
    
    if (r->method != STX_METHOD_GET) {
        r->status = STX_STATUS_NOT_IMPL;
        return;
    }
    
    namelen = strlen(r->server->webroot) + r->uri_len;
    if (*(r->uri_start + r->uri_len - 1) == '/') {
        namelen += strlen(r->server->index);
    }
    
    filename = malloc(namelen * sizeof(char));
    strcpy(filename, r->server->webroot);
    strncat(filename, r->uri_start, r->uri_len);
    
    if (*(r->uri_start + r->uri_len - 1) == '/') {
        strncat(filename, r->server->index, strlen(r->server->index));
    }
    
    if ((fd = open(filename, O_RDONLY)) == -1) {
        if (ENOENT == errno) {
            r->status = STX_STATUS_NOT_FOUND;
        } else if (EACCES == errno) {
            r->status = STX_STATUS_FORBIDDEN;
        } else {
            r->status = STX_STATUS_ERROR;
            perror("open");
        }
        
        free(filename);

        return;
    }
    
    if (fstat(fd, &sb) == -1) {
        r->status = STX_STATUS_ERROR;
        perror("fstat");
        free(filename);

        return;
    }
    
    free(filename);
    
    //sb.mtime
    //content type
    //charset ?
    //date
    r->status = STX_STATUS_OK;
    r->content_length = sb.st_size;
    r->fd = fd;
    
    return;
}
