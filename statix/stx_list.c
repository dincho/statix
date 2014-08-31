//
//  stx_list.c
//  statix
//
//  Created by Dincho Todorov on 9/1/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include "stx_list.h"

stx_list_t *stx_list_init()
{
    return calloc(1, sizeof(stx_list_t));
}

void stx_list_destroy(stx_list_t *list)
{
    stx_list_node_t *node;

    for (node = list->first; node != NULL; node = node->next) {
        if(node->prev) {
            free(node->prev);
        }
    }
    
    free(list->last);
    free(list);
}

void stx_list_append(stx_list_t *list, void *data)
{
    stx_list_node_t *node = calloc(1, sizeof(stx_list_node_t));
    node->data = data;
    
    if(list->last == NULL) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        node->prev = list->last;
        list->last = node;
    }
    
    list->count++;
}

void stx_list_remove(stx_list_t *list, stx_list_node_t *node)
{
    if(node == list->first && node == list->last) {
        list->first = NULL;
        list->last = NULL;
    } else if(node == list->first) {
        list->first = node->next;
        list->first->prev = NULL;
    } else if (node == list->last) {
        list->last = node->prev;
        list->last->next = NULL;
    } else {
        node->next->prev = node->prev;
        node->prev->next = node->next;
    }
    
    list->count--;
    free(node);
}

stx_list_node_t *stx_list_find(stx_list_t *list, void *data)
{
    stx_list_node_t *node;
    
    for (node = list->first; node != NULL; node = node->next) {
        if (node->data == data) {
            return node;
        }
    }
    
    return NULL; //not found
}