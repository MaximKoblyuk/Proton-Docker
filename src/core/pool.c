#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proton.h"

#define POOL_DEFAULT_SIZE 4096

proton_pool_t* proton_pool_create(size_t size) {
    if (size == 0) size = POOL_DEFAULT_SIZE;
    
    proton_pool_t *pool = malloc(sizeof(proton_pool_t));
    if (!pool) return NULL;
    
    pool->size = size;
    pool->used = 0;
    pool->data = malloc(size);
    pool->next = NULL;
    
    if (!pool->data) {
        free(pool);
        return NULL;
    }
    
    return pool;
}

void* proton_pool_alloc(proton_pool_t *pool, size_t size) {
    if (!pool || size == 0) return NULL;
    
    /* Align to 8 bytes */
    size = (size + 7) & ~7;
    
    /* Check if current pool has space */
    if (pool->used + size <= pool->size) {
        void *ptr = (char*)pool->data + pool->used;
        pool->used += size;
        return ptr;
    }
    
    /* Need a new pool block */
    size_t new_size = (size > pool->size) ? size : pool->size;
    proton_pool_t *new_pool = proton_pool_create(new_size);
    if (!new_pool) return NULL;
    
    /* Link to chain */
    new_pool->next = pool->next;
    pool->next = new_pool;
    
    /* Allocate from new pool */
    void *ptr = new_pool->data;
    new_pool->used = size;
    return ptr;
}

void proton_pool_destroy(proton_pool_t *pool) {
    while (pool) {
        proton_pool_t *next = pool->next;
        free(pool->data);
        free(pool);
        pool = next;
    }
}
