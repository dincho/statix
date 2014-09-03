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
#include <pthread.h>

typedef enum {
    STX_LOG_ERR = 0,
    STX_LOG_WARN,
    STX_LOG_INFO,
    STX_LOG_DEBUG,
    STX_LOG_NONE = -1,
} stx_log_level_t;

typedef struct {
    pthread_mutex_t mutex;
    FILE *fp;
    int level;
} stx_log_t;

void _stx_log(stx_log_t *logger, stx_log_level_t level, const char *fmt, ...);

#define stx_log(logger, log_level, fmt, ...) \
    do { if (logger->level >= log_level) _stx_log(logger, log_level, fmt, ##__VA_ARGS__); } while (0)

#endif
