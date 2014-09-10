//
//  main.c
//  statix
//
//  Created by Dincho Todorov on 2/28/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h> //exit status, atoi
#include <stdio.h> //stderr
#include <string.h> //strcpy
#include <unistd.h> //getcwd, getopt
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "config.h"
#include "stx_log.h"
#include "stx_server.h"
#include "stx_listen.h"
#include "stx_list.h"
#include "stx_event_queue.h"
#include "stx_master_worker.h"
#include "stx_worker.h"
#include "stx_accept.h"

const int shutdown_signals[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM};
const int nb_shutdown_signals = 4;
int STX_RUNNING = 1;

void shutdown_handler(int signum);
int parse_options(int argc, const char *argv[], char *ip, int *port, char *webroot, int *workers, int *connections, char *logfile, int *loglevel);
void print_usage(const char *name);

int main(int argc, const char *argv[])
{
    stx_log_t *logger;
    stx_server_t *server;
    stx_worker_t *workers;
    pthread_t *threads;
    int *queues;
    sigset_t sigset;
    int sigerr;
    
    //options
    char opt_ip[STX_IP_LEN];
    int opt_port;
    char opt_webroot[STX_MAX_PATH];
    char opt_logfile[STX_MAX_PATH];
    int opt_loglevel;
    int opt_workers = 0;
    int opt_connections = 0;

    if (!parse_options(argc, argv, opt_ip, &opt_port, opt_webroot, &opt_workers,
                       &opt_connections, opt_logfile, &opt_loglevel)
    ) {
        return EXIT_FAILURE;
    }
    
    //initialize all main structures
    logger = stx_logger_init(opt_logfile, opt_loglevel);
    if (logger == NULL) {
        perror("logger init");
        return EXIT_FAILURE;
    }
    
    server = stx_server_init(opt_ip, opt_port, opt_webroot, logger);
    if (server == NULL) {
        perror("server init");
        return EXIT_FAILURE;
    }
    
    workers = calloc(opt_workers, sizeof(stx_worker_t));
    threads = calloc(opt_workers, sizeof(pthread_t));
    queues = calloc(opt_workers, sizeof(int));
    
    //install shutdown signals handler and fill sigset to block by threads
    sigemptyset(&sigset);
    for (int i = 0; i < sizeof(shutdown_signals)/sizeof(int); i++) {
        sigaddset(&sigset, shutdown_signals[i]);

        if (SIG_ERR == signal(shutdown_signals[i], shutdown_handler)) {
            fprintf(stderr, "cannot install signal handler for %d: %s",
                    shutdown_signals[i], strerror(errno));
            return EXIT_FAILURE;
        }
    }
    
    //block all threads to receive the signals prior their creation
    sigerr = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    if (sigerr != 0) {
        fprintf(stderr, "pthread_sigmask: %s", strerror(sigerr));
        return EXIT_FAILURE;
    }
    
    //configure and create worker threads
    for (int i = 0; i < opt_workers; i++) {
        workers[i].server = server;
        workers[i].max_connections = opt_connections;
        workers[i].id = i+2; //start from 1, 1 is the master
        
        if ((workers[i].queue = stx_queue_create()) == -1) {
            perror("stx_queue_create");
            return EXIT_FAILURE;
        }

        if (pthread_create(&threads[i], NULL, stx_worker, &workers[i])) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }
    
    //unblock all signals for master thread after worker threads are already created
    sigfillset(&sigset);
    sigerr = pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    if (sigerr != 0) {
        fprintf(stderr, "pthread_sigmask: %s", strerror(sigerr));
        return EXIT_FAILURE;
    }

    
    if (stx_listen(server)) {
        return EXIT_FAILURE;
    }
    
    stx_log_flush(logger);
    
    
    //master loop - returns only if global variable STX_RUNNING is set to true
    stx_master_worker(server, opt_workers, workers);
    
    for (int i = 0; i < opt_workers; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }
        
        stx_queue_close(queues[i]);
    }
    
    //cleanup
    free(queues);
    free(threads);
    free(workers);
    stx_server_destory(server);
    stx_logger_destroy(logger);

    return EXIT_SUCCESS;
}

void shutdown_handler(int signum)
{
    fprintf(stderr, "Caught signal %d, shutting down ...\n", signum);
    STX_RUNNING = 0;
}

int parse_options(int argc, const char *argv[], char *ip, int *port, char *webroot, int *workers, int *connections, char *logfile, int *loglevel)
{
    char o;
    
    //default options
    strcpy(ip, "127.0.0.1");
    *port = 8000;
    *workers = 2;
    *connections = 500;
    strcpy(logfile, "stderr");
    *loglevel = STX_LOG_WARN;
    
    //default webroot
    if (NULL == getcwd(webroot, STX_MAX_PATH)) {
        perror("getcwd");
        return 0;
    }
    
    //parse user supplied options
    while ((o = getopt (argc, argv, "hi:p:r:w:c:l:v:")) != -1) {
        switch (o)
        {
            case 'h': //help
                print_usage(argv[0]);
                return 0;
                break;
            case 'i':
                strcpy(ip, optarg);
                break;
            case 'p':
                *port = atoi(optarg);
                break;
            case 'r':
                strncpy(webroot, optarg, 255);
                break;
            case 'w':
                *workers = atoi(optarg);
                break;
            case 'c':
                *connections = atoi(optarg);
                break;
            case 'l':
                strncpy(logfile, optarg, 255);
                break;
            case 'v':
                *loglevel = atoi(optarg);
                break;
            case '?':
                break;
        }
    }
    
    //validation the options
    if (NULL == ip || 0 == *port || 0 == *workers) {
        print_usage(argv[0]);
        return 0;
    }
    
    return 1;
}

void print_usage(const char *name)
{
    printf("Usage: %s <options>\n\n"
           "  -h                  show help and exit\n"
           "  -i ip_address       listen IP address                     (default: 127.0.0.1)\n"
           "  -p port             listen port                           (default: 8000)\n"
           "  -r webroot          webroot directory                     (default: current directory)\n"
           "  -w nb_workers       number of workers to start            (default: 2)\n"
           "  -l filepath         logfile path                          (default: stderr)\n"
           "  -v level            logging level [0-4], 0 to disable     (default: 2)\n"
           "\n"
           "example: %s -i 192.168.1.1 -p 8080 -l web.log -v 1 -w 4 -r /var/www/public_html\n"
           "\n",
           name, name);
}
