//
//  stx_log.c
//  statix
//
//  Created by Dincho Todorov on 3/18/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdarg.h>
#include <stdio.h> //stderr
#include <pthread.h>
#include <sys/time.h>
#include "stx_log.h"

void _stx_log(stx_log_t *logger, stx_log_level_t level, const char *fmt, ...)
{    
    pthread_t t;
    va_list arg;
    struct timeval tv;
    
    t = pthread_self();
    
    gettimeofday(&tv, NULL);

    
    /* Write the error message */
    pthread_mutex_lock(&logger->mutex);
        fprintf(logger->fp, "[%lu][%lu.%d] ", (long)t, tv.tv_sec, tv.tv_usec);
        
        va_start(arg, fmt);
        vfprintf(logger->fp, fmt, arg);
        va_end(arg);
        
        fprintf(logger->fp, "\n");
    pthread_mutex_unlock(&logger->mutex);
}
