//
//  stx_log.c
//  statix
//
//  Created by Dincho Todorov on 3/18/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdarg.h>
#include <stdio.h> //stderr
#include "stx_log.h"

void stx_log(stx_log_t *logger, stx_log_level_t level, const char *fmt, ...)
{
    va_list arg;
    
    /* Write the error message */
    va_start(arg, fmt);
    vfprintf(logger->fp, fmt, arg);
    va_end(arg);
    
    fprintf(stderr, "\n");
}