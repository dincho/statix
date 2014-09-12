//
//  stx_log.c
//  statix
//
//  Created by Dincho Todorov on 3/18/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h> //stderr
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "stx_log.h"

stx_log_t *stx_logger_init(const char *filepath, stx_log_level_t level)
{
    stx_log_t *logger;
    int err;
    
    logger = malloc(sizeof(stx_log_t));
    if (NULL == logger) {
        return NULL;
    }
    
    if ((err = pthread_mutex_init(&logger->mutex, NULL))) {
        fprintf(stderr, "logger pthread_mutex_init(): %s", strerror(err));
        free(logger);

        return NULL;
    }
    
    logger->level = level;
    
    if (0 == strcmp("stderr", filepath)) {
        logger->fp = stderr;
    } else {
        logger->fp = fopen(filepath, "a+");
        if (NULL == logger->fp) {
            free(logger);
            pthread_mutex_destroy(&logger->mutex);

            return NULL;
        }
    }
    
    return logger;
}

void stx_logger_destroy(stx_log_t *logger)
{
    if (logger->fp != stderr) {
        fclose(logger->fp);
    }

    free(logger);
}

void stx_log_flush(stx_log_t *logger)
{
    int err;
    
    if((err = pthread_mutex_lock(&logger->mutex))) {
        fprintf(stderr, "logger pthread_mutex_lock(): %s", strerror(err));
    }
    
    fflush(logger->fp);
    
    if((err = pthread_mutex_unlock(&logger->mutex))) {
        fprintf(stderr, "logger pthread_mutex_unlock(): %s", strerror(err));
    }
}

void _stx_log(stx_log_t *logger, stx_log_level_t level, const char *fmt, ...)
{    
    va_list arg;
    int err;
    time_t rawtime;
    struct tm * timeinfo;
    char timestring[30];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestring, 30, "%d/%m/%Y %H:%M:%S", timeinfo);
    
    /* Write the error message */
    if((err = pthread_mutex_lock(&logger->mutex))) {
        fprintf(stderr, "logger pthread_mutex_lock(): %s", strerror(err));
    }
    
        fprintf(logger->fp, "[%s] ", timestring);
        
        va_start(arg, fmt);
        vfprintf(logger->fp, fmt, arg);
        va_end(arg);
        
        fprintf(logger->fp, "\n");
    
    if((err = pthread_mutex_unlock(&logger->mutex))) {
        fprintf(stderr, "logger pthread_mutex_unlock(): %s", strerror(err));
    }
}
