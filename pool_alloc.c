/*****************************************************
 *
 * Author: Michael Adams
 *
 * Description: tunable block pool allocator
 *
 ****************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h> // internal sanity checks

#include "pool_alloc.h"

static uint8_t g_pool_heap[65536];
static bool g_pool_heap_init = false;
static uint8_t* g_pool_heap_max = g_pool_heap + sizeof(g_pool_heap);

static uint8_t blk_sz_cnt = 0;    // # of block sizes = # of pools
static uint16_t* blk_szs;         // block_sizes array
static uint8_t** pool_base_addrs; // base addresses of pools
static uint8_t** blk_alloc;       // next block address to be allocated

bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
    /******* verify pool state and args *******/

    /* verify pool not already init */
    if (g_pool_heap_init) {
#ifdef VERBOSE
        printf("ERROR: pool already init\n");
#endif
        return false;
    }

    /* verify valid number of pools */ 
    if (block_size_count < MIN_POOLS || block_size_count > MAX_POOLS) {
#ifdef VERBOSE
        printf("ERROR: invalid number of pools\n");
#endif
        return false;
    }
    
    /* verify valid block sizes array */
    if (block_sizes == NULL) {
#ifdef VERBOSE
        printf("ERROR: invalid block_size array\n");
#endif
        return false;
    }

    /* verify valid block sizes */
    for (uint8_t i = 0; i < block_size_count; i++) {
        if (block_sizes[i] < MIN_BLOCK_SIZE || block_sizes[i] > MAX_BLOCK_SIZE) {
#ifdef VERBOSE
            printf("ERROR: invalid block size\n");
#endif
            return false;
        }
    }

    /******* init heap management data *******/

    /* write block sizes into beginning of g_pool_heap */
    uint8_t* brk = g_pool_heap;
    blk_szs = (uint16_t*)brk;
    blk_sz_cnt = block_size_count;

    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        *((uint16_t*)brk) = (uint16_t)(block_sizes[i]);
        brk += sizeof(uint16_t);
    }

    /* reserve space for pool base address pointers */
    pool_base_addrs = (uint8_t**)brk;
    brk += blk_sz_cnt * sizeof(uint8_t*);
    
    /* reserve space for pool next block allocation address pointers */
    blk_alloc = (uint8_t**)brk;
    brk += blk_sz_cnt * sizeof(uint8_t*);

    /* compute bytes available after reserving heap memory management and find pool size */
    uint16_t heap_mgmt_size = brk - g_pool_heap;
    uint16_t bytes_free = sizeof(g_pool_heap) - heap_mgmt_size;
    uint16_t pool_size = bytes_free / block_size_count;
    uint16_t heap_remainder = bytes_free % blk_sz_cnt;

    /* verify heap mgmnt + size of each pool * m pools + remainder (if any) == total heap size */
    assert(sizeof(g_pool_heap) == (heap_mgmt_size + (pool_size * blk_sz_cnt) + heap_remainder));

#ifdef VERBOSE
    printf("-- HEAP --\n");
    printf("full heap size: %lu bytes\n", sizeof(g_pool_heap));
    printf("heap mgmt size: %u bytes\n", heap_mgmt_size);
    printf("heap pool size: %u bytes\n", bytes_free);
    printf("each pool size: %u bytes\n", pool_size);
    printf("heap remainder: %u bytes\n", heap_remainder);
#endif

    /******* create pools and init blocks *******/

    /* compute and assign pool_base_addrs */
    /* note: on first itr, brk is after end of blk_alloc which is pool_base_addr[0] */
    /* note: also init blk_alloc because the first blk of each pool init to pool_base_addr[i] */
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        pool_base_addrs[i] = (uint8_t*)brk + (pool_size * i);
        blk_alloc[i] = pool_base_addrs[i];
    }

    /* calculate number of blocks that can fit in each pool, then init free list ptr links */
    uint16_t blk_mem_req = 0;
    uint16_t pool_blks = 0;
    uint16_t bytes_remainder = 0;
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        /* mem requirement for each block = sizeof(uint8_t*) + blk_szs[i] */
        blk_mem_req = blk_szs[i] + sizeof(uint8_t*);

        pool_blks = pool_size / blk_mem_req; 
        bytes_remainder = pool_size % blk_mem_req;

        /* verify number of blocks * block memory + remainder bytes == pool size */
        assert(pool_size == (blk_mem_req * pool_blks + bytes_remainder));

#ifdef VERBOSE
        printf("\n-- POOL[%d] --\n", i);
        printf("blk_szs[%d]:  %d\n", i, blk_szs[i]);
        printf("blk_mem_req: %d\n", blk_mem_req);
        printf("pool_blks:   %d\n", pool_blks);
        printf("bytes_rmdr:  %d\n", bytes_remainder);
#endif

        /* init pointer at header of each block to 'next' block in order to create free list */
        for (uint16_t j = 0; j < (pool_blks - 1); j++) {

            /* set current blk header = next blk addr */
            *((uint8_t**)brk) = brk + sizeof(uint8_t*) + blk_szs[i];

            /* increment brk by total block mem requirement */
            brk += sizeof(uint8_t*) + blk_szs[i];
        }

        /* set last block 'next' ptr to NULL */
        *((uint8_t**)brk) = NULL;
        brk += sizeof(uint8_t*) + blk_szs[i];

        /* add remaining bytes to get to next pool_base_addr */
        brk += bytes_remainder;
    }

    /* no byte left behind */
    assert((brk + (bytes_free % block_size_count)) - g_pool_heap == sizeof(g_pool_heap));

    /* heap init complete */
    g_pool_heap_init = true;
    
    return true;
}

void* pool_malloc(size_t n)
{
    /******* verify pool state and args *******/

    /* verify pool already init */
    if (!g_pool_heap_init) {
#ifdef VERBOSE
        printf("ERROR: pool not init\n");
#endif
        return NULL;
    }

    /* verify selected block size is valid and if so, find index */
    int8_t pool = -1;
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        if (blk_szs[i] == n)
            pool = i;
    }

    if (pool == -1) {
#ifdef VERBOSE
        printf("ERROR: invalid block size\n");
#endif
        return NULL;
    }

    /* check if pool has available blocks */
    if (blk_alloc[pool] == NULL) {
#ifdef VERBOSE
        printf("ERROR: no block available\n");
#endif
        return NULL;
    }

    /******* allocate block and identify next block to be allocated *******/

    /* allocate free block */
    uint8_t* blk = blk_alloc[pool];

    /* bump next blk alloc */
    blk_alloc[pool] = *((uint8_t**)blk_alloc[pool]);

    /* convert block header ptr to block data ptr */
    return (void*)blk_hdr_to_data(blk);
}

void pool_free(void* ptr)
{
    /******* verify pool state and args *******/

    /* verify pool already init */
    if (!g_pool_heap_init) {
#ifdef VERBOSE
        printf("ERROR: pool not init\n");
#endif
        return;
    }

    /* verify pointer not NULL */
    if (ptr == NULL) {
#ifdef VERBOSE
        printf("ERROR: cannot free NULL ptr\n");
#endif
        return;
    }

    /* verify pointer within valid boundary pool_base_addr[0] and max_heap pointer */
    if ((uint8_t*)ptr < pool_base_addrs[0] || (uint8_t*)ptr > g_pool_heap_max) {
#ifdef VERBOSE
        printf("ERROR: ptr not within valid heap boundaries\n");
#endif
        return;
    }

    /* find pool */
    uint8_t pool = -1;
    for (uint8_t i = 0; i < (blk_sz_cnt - 1); i++) {
        if ((uint8_t*)ptr >= pool_base_addrs[i] && (uint8_t*)ptr <= pool_base_addrs[i+1]) {
            pool = i;
        }
        else {
            pool = i+1;
        }
    }

    /* convert ptr from data to header */
    uint8_t* hdr_ptr = blk_data_to_hdr((uint8_t*)ptr);

    /* verify ptr is aligned with blocks in pool otherwise invalid pointer */
    if (((hdr_ptr - pool_base_addrs[pool]) % (blk_szs[pool] + sizeof(uint8_t*))) != 0) {
#ifdef VERBOSE
        printf("ERROR: unaligned block pointer\n");
#endif
        return;
    }

    /* verify block is not already freed by walking free list */
    uint8_t* blk = NULL;
    blk = blk_alloc[pool];
    while (blk != NULL) {
        if (blk == hdr_ptr) {
#ifdef VERBOSE
            printf("ERROR: block already free\n");
#endif
            return;
        }
        blk = *((uint8_t**)blk);
    }

    /******* free block and add to front of block alloc list *******/

    /* assign block hdr ptr to current first in free list */
    *((uint8_t**)hdr_ptr) = blk_alloc[pool];

    /* insert freed block at front of free list */
    blk_alloc[pool] = hdr_ptr;

    return;
}

uint8_t* blk_hdr_to_data(uint8_t* ptr) {
    return (ptr == NULL ? NULL : (ptr += sizeof(uint8_t*)));
}

uint8_t* blk_data_to_hdr(uint8_t* ptr) {
    return (ptr == NULL ? NULL : (ptr -= sizeof(uint8_t*)));
}

void heap_print() {
    printf("+-----------------------------------------------------+\n");
    printf("|                  heap mgmt data                     |\n");
    printf("+-----------------------------------------------------+\n");

    /* check that blk_szs array starts at heap start */
    printf("blk_szs = g_pool_heap [%p == %p]\n", blk_szs, g_pool_heap);
    printf("\n");

    /* print blk_szs address and value */
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        if (i == 0)
            printf("blk_szs[%d]:         [addr: %p] [val: %d] \n", i, &blk_szs[i], blk_szs[i]);
        else
            printf("blk_szs[%d]:         [addr: %p] [val: %d] [delta addr: %ld]\n", i, &blk_szs[i], blk_szs[i], (uint64_t)&blk_szs[i] - (uint64_t)&blk_szs[i-1]);
    }
    printf("\n");

    /* print pool_base_addrs address and value */
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        if (i == 0)
            printf("pool_base_addrs[%d]: [addr: %p] [val: %p]\n", i, &pool_base_addrs[i], pool_base_addrs[i]);
        else
            printf("pool_base_addrs[%d]: [addr: %p] [val: %p] [delta addr: %ld] [delta val: %ld]\n", i, &pool_base_addrs[i], pool_base_addrs[i], (uint64_t)&pool_base_addrs[i] - (uint64_t)&pool_base_addrs[i-1], (pool_base_addrs[i] - pool_base_addrs[i-1]));
    }
    printf("\n");

    /* print blk_alloc address and value */
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        if (i == 0)
            printf("blk_alloc[%d]:       [addr: %p] [val: %p]\n", i, &blk_alloc[i], blk_alloc[i]);
        else
            printf("blk_alloc[%d]:       [addr: %p] [val: %p] [delta addr: %ld] [delta val: %ld]\n", i, &blk_alloc[i], blk_alloc[i], (uint64_t)&blk_alloc[i] - (uint64_t)&blk_alloc[i-1], (blk_alloc[i] - blk_alloc[i-1]));
    }
    printf("\n");

    /* print heap header size */
    printf("heap header size: %ld\n\n", pool_base_addrs[0] - g_pool_heap);

    printf("+-----------------------------------------------------+\n");
    printf("|                  pool block data                    |\n");
    printf("+-----------------------------------------------------+\n");

    /* traverse each pool list to find each block */
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        pool_print(i);
    }
}

void pool_print(uint8_t pool) {
    /* traverse each pool list to find each block */
    uint8_t* blk = NULL;
    printf("------- POOL[%u] -------\n", pool);
    printf("blk_szs[%d]:         [addr: %p] [val: %d] \n", pool, &blk_szs[pool], blk_szs[pool]);
    printf("pool_base_addrs[%d]: [addr: %p] [val: %p]\n", pool, &pool_base_addrs[pool], pool_base_addrs[pool]);
    printf("blk_alloc[%d]:       [addr: %p] [val: %p]\n", pool, &blk_alloc[pool], blk_alloc[pool]);
    printf("block mem req: %ld\n", blk_szs[pool] + sizeof(uint8_t*));
    blk = blk_alloc[pool];

    printf("------- BLOCKS -------\n");
    uint16_t blks = 0;
    while (blk != NULL) {
        printf("blk[%u]: [addr: %p] [*blk[%u]: %p] [delta val: %ld]\n", blks, blk, blks, *((uint8_t**)blk), *((uint8_t**)blk) - blk);
        blk = *((uint8_t**)blk);
        blks++;
    }
    printf("\n");
}
