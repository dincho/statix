//
//  stx_hashmap.c
//  statix
//
//  Created by Dincho Todorov on 9/7/14.
//  Copyright (c) 2014 Dincho Todorov. All rights reserved.
//

#include <stdlib.h>
#include <string.h> //memset
#include "stx_hashmap.h"

static unsigned long stx_hash(const char *str);
static stx_pair_t * get_pair(stx_bucket_t *bucket, const stx_key_t key);

stx_hashmap_t* stx_hashmap_init(unsigned int capacity)
{
	stx_hashmap_t *map;
	
	map = malloc(sizeof(stx_hashmap_t));
    
	if (map == NULL) {
		return NULL;
	}
    
    map->elcount = 0;
	map->bcount = capacity;
	map->buckets = malloc(map->bcount * sizeof(stx_bucket_t));
    
	if (map->buckets == NULL) {
		free(map);
		return NULL;
	}

	memset(map->buckets, 0, map->bcount * sizeof(stx_bucket_t));

	return map;
}

void stx_hashmap_destory(stx_hashmap_t *map)
{
	unsigned int i, j, n, m;
	stx_bucket_t *bucket;
	stx_pair_t *pair;
    
	if (map == NULL) {
		return;
	}
    
	n = map->bcount;
	bucket = map->buckets;
	i = 0;

	while (i < n) {
		m = bucket->count;
		pair = bucket->pairs;
		j = 0;
        
		while(j < m) {
			free(pair->value);
			pair++;
			j++;
		}

		free(bucket->pairs);
		bucket++;
		i++;
	}

	free(map->buckets);
	free(map);
}

void *stx_hashmap_cget(const stx_hashmap_t *map, const char *key)
{
    return stx_hashmap_get(map, stx_hash(key));
}

void *stx_hashmap_get(const stx_hashmap_t *map, const stx_key_t key)
{
	unsigned int index;
	stx_bucket_t *bucket;
	stx_pair_t *pair;
    
	if (map == NULL) {
		return NULL;
	}

	index = key % map->bcount;
	bucket = &(map->buckets[index]);
	pair = get_pair(bucket, key);

	if (pair == NULL) {
		return NULL;
	}
    
    return pair->value;
}

uint8_t stx_hashmap_cput(stx_hashmap_t *map, const char *key, void *value)
{
    return stx_hashmap_put(map, stx_hash(key), value);
}

uint8_t stx_hashmap_put(stx_hashmap_t *map, const stx_key_t key, void *value)
{
	unsigned int index;
	stx_bucket_t *bucket;
	stx_pair_t *tmp_pairs, *pair;
    
	if (map == NULL) {
		return 0;
	}
    
	/* Get a pointer to the bucket the key string hashes to */
	index = key % map->bcount;
	bucket = &(map->buckets[index]);
    
	/* Check if we can handle insertion by simply replacing
	 * an existing value in a key-value pair in the bucket.
	 */
	if ((pair = get_pair(bucket, key)) != NULL) {
        pair->value = value;

		return 1;
	}
    
	/* Create a key-value pair */
	if (bucket->count == 0) {
		/* The bucket is empty, lazily allocate space for a single
		 * key-value pair.
		 */
		bucket->pairs = malloc(sizeof(stx_pair_t));
		if (bucket->pairs == NULL) {
			return 0;
		}

		bucket->count = 1;
	} else {
		/* The bucket wasn't empty but no pair existed that matches the provided
		 * key, so create a new key-value pair.
		 */
		tmp_pairs = realloc(bucket->pairs,
                            (bucket->count + 1) * sizeof(stx_pair_t));

		if (tmp_pairs == NULL) {
			return 0;
		}

		bucket->pairs = tmp_pairs;
		bucket->count++;
        map->elcount++;
	}

	/* Get the last pair in the chain for the bucket */
	pair = &(bucket->pairs[bucket->count - 1]);
	pair->key = key;
	pair->value = value;

	return 1;
}

int stx_hashmap_count(const stx_hashmap_t *map)
{
	unsigned int i, j, count;
	stx_bucket_t *bucket_ptr;
	stx_pair_t *pair;
    
	bucket_ptr = map->buckets;
	i = 0;
	count = 0;
    
	while (i < map->bcount) {
		pair = bucket_ptr->pairs;
		j = 0;

		while (j < bucket_ptr->count) {
			count++;
			pair++;
			j++;
		}

		bucket_ptr++;
		i++;
	}

	return count;
}

/*
 * Returns a pair from the bucket that matches the provided key,
 * or null if no such pair exist.
 */
static stx_pair_t * get_pair(stx_bucket_t *bucket, const stx_key_t key)
{
	unsigned int i, n;
	stx_pair_t *pair;
    
	n = bucket->count;
	if (n == 0) {
		return NULL;
	}
    
	pair = bucket->pairs;
	i = 0;

	while (i++ < n) {
		if (pair->key == key) {
            return pair;
		}

		pair++;
	}

	return NULL;
}

// djb2 - http://www.cse.yorku.ca/~oz/hash.html
static unsigned long stx_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
}
