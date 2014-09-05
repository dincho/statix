//
//  stx_sendfile.h
//  statix
//
//  Created by Dincho Todorov on 9/4/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_sendfile_h
#define statix_stx_sendfile_h

#ifdef __linux
#include <sys/sendfile.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif //__linux


int stx_sendfile(int fd, int sock, off_t *offset, size_t count, off_t *sent, char *headers, size_t headers_len);

#endif //statix_stx_sendfile_h
