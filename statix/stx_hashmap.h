//
//  stx_hashmap.h
//  statix
//
//  Created by Dincho Todorov on 9/7/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#ifndef statix_stx_hashmap_h
#define statix_stx_hashmap_h

#include <stdint.h>

typedef unsigned long stx_key_t;

typedef struct {
	stx_key_t key;
	void *value;
} stx_pair_t;

typedef struct {
	unsigned int count;
	stx_pair_t *pairs;
} stx_bucket_t;

typedef struct {
    unsigned int elcount;
	unsigned int bcount;
	stx_bucket_t *buckets;
} stx_hashmap_t;

stx_hashmap_t* stx_hashmap_init(unsigned int capacity);
void stx_hashmap_destory(stx_hashmap_t *map);
void *stx_hashmap_get(const stx_hashmap_t *map, const stx_key_t key);
void *stx_hashmap_cget(const stx_hashmap_t *map, const unsigned char *key);
uint8_t stx_hashmap_put(stx_hashmap_t *map, const stx_key_t key, void *value);
uint8_t stx_hashmap_cput(stx_hashmap_t *map, const unsigned char *key, void *value);

#endif
