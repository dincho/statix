//
//  main.c
//  statix
//
//  Created by Dincho Todorov on 2/28/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h> //stderr
#include "stx_log.h"
#include "stx_server.h"
#include "stx_listen.h"
#include "stx_accept.h"
#include "stx_read.h"
#include "stx_write.h"

#include <string.h>
#include <unistd.h> //getcwd
#include <errno.h>

#include "stx_event_queue.h"

int main(int argc, const char * argv[])
{
    int queue, nev;
    stx_event_t chlist[10];
    stx_event_t ev;
    stx_event_data_t *ev_data;

    stx_log_t logger;
    logger.level = STX_LOG_DEBUG;
    logger.fp = stderr;
    
    stx_server_t server;
    server.logger = &logger;
    server.port = 8000;
    server.sock = -1;
    strcpy(server.index, "index.html");
    
    if (NULL == getcwd(server.webroot, sizeof(server.webroot))) {
        perror("getcwd");
        return EXIT_FAILURE;
    }

    if ((queue = stx_queue_create()) == -1) {
        perror("kqueue");
        return EXIT_FAILURE;
    }

    if (stx_listen(queue, &server)) {
        return EXIT_FAILURE;
    }
    

    for (;;) {
        nev = stx_event_wait(queue, (stx_event_t *)&chlist, 10, NULL);
        
        if (nev == -1) {
            perror("stx_event_wait()");
            if (errno != EINTR) {
                return EXIT_FAILURE;
            }
        }
        
        for (int i = 0; i < nev; i++) {
            ev = chlist[i];
            ev_data = (stx_event_data_t *) ev.udata;
            
            switch (ev_data->event_type) {
                case STX_EV_ACCEPT:
                    stx_accept(queue, ev_data->data); //server
                    break;
                case STX_EV_READ:
                    stx_read(queue, ev_data->data); //request
                    break;
                case STX_EV_WRITE:
                    stx_write(queue, ev_data->data); //request
                    break;
                case STX_EV_CLOSE:
                    stx_request_close(ev_data->data); //request
                    break;
            }
            
            if (ev.flags & STX_EVCTL_ONESHOT) {
                free(ev_data);
            }
        }
    }

    stx_queue_close(queue);
    return EXIT_SUCCESS;
}
