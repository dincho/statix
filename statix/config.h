//
//  config.h
//  statix
//
//  Created by Dincho Todorov on 9/10/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_config_h
#define statix_config_h

#define STX_MAX_PATH 255
#define STX_MAX_NAME 128
#define STX_IP_LEN 15
#define STX_REQUEST_BUFF_SZ 8192 //8k
#define STX_MAX_EVENTS 1024 // kevent x 32b = 32Kb
#define STX_OPEN_FILES_CACHE_CAPACITY 16

//all event loops check this variable on each iteration
//if it's set to 1, all loops quit
extern int STX_RUNNING;
extern const int shutdown_signals[];
extern const int nb_shutdown_signals;

#endif
