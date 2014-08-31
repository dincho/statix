//
//  stx_list.h
//  statix
//
//  Created by Dincho Todorov on 9/1/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_list_h
#define statix_stx_list_h

typedef struct stx_list_node_t stx_list_node_t;

struct stx_list_node_t {
    stx_list_node_t *next;
    stx_list_node_t *prev;
    void *data;
};

typedef struct {
    int count;
    stx_list_node_t *first;
    stx_list_node_t *last;
} stx_list_t;

stx_list_t *stx_list_init();
void stx_list_destroy(stx_list_t *list);
void stx_list_append(stx_list_t *list, void *data);
void stx_list_remove(stx_list_t *list, stx_list_node_t *node);
stx_list_node_t *stx_list_find(stx_list_t *list, void *data);

#endif
