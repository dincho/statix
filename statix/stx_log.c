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
#include "stx_log.h"

void stx_log(stx_log_t *logger, stx_log_level_t level, const char *fmt, ...)
{    
    pthread_t t = pthread_self();
    va_list arg;

    if (logger->level < level) {
        return;
    }
    
    /* Write the error message */
    pthread_mutex_lock(&logger->mutex);
        fprintf(logger->fp, "[%lu] ", (long)t);
        
        va_start(arg, fmt);
        vfprintf(logger->fp, fmt, arg);
        va_end(arg);
        
        fprintf(logger->fp, "\n");
    pthread_mutex_unlock(&logger->mutex);
}
