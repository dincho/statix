//
//  stx_log.h
//  statix
//
//  Created by Dincho Todorov on 3/18/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_log_h
#define statix_stx_log_h

#include <stdio.h> //FILE pointer

typedef enum {
    STX_LOG_ERR = 0,
    STX_LOG_WARN,
    STX_LOG_INFO,
    STX_LOG_DEBUG,
} stx_log_level_t;

typedef struct {
    FILE *fp;
    int level;
} stx_log_t;

void stx_log(stx_log_t *logger, stx_log_level_t level, const char *fmt, ...);

#endif
