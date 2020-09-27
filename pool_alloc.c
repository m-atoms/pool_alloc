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
#include <assert.h> //test cases

#include "pool_alloc.h"

static uint8_t g_pool_heap[65536];
static bool g_pool_heap_init = false;

static uint8_t blk_sz_cnt = 0;
static uint16_t* blk_szs; // stores block_sizes array
static uint8_t** pool_base_addrs; // stores base addresses of pools
static uint8_t** blk_alloc; // stores next block address to be allocated

bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
    /* verify pool not already init */
    if (g_pool_heap_init) {
        printf("ERROR: pool already init\n");
        return false;
    }

    /* verify valid number of pools */ 
    if (block_size_count < MIN_POOLS || block_size_count > MAX_POOLS) {
        printf("ERROR: invalid number of pools\n");
        return false;
    }
    
    /* verify valid block sizes array */
    if (block_sizes == NULL) {
        printf("ERROR: invalid block_size array\n");
        return false;
    }

    /* verify valid block sizes */
    for (uint8_t i = 0; i < block_size_count; i++) {
        if (block_sizes[i] < MIN_BLOCK_SIZE || block_sizes[i] > MAX_BLOCK_SIZE) {
            printf("ERROR: invalid block size\n");
            return false;
        }
    }

    /*********************************************
     * init heap data management 
     ********************************************/

    /* write block sizes into beginning of g_pool_heap */
    // TODO: explain dont need to store this as size_t which is huge (8B on 64bit machine)
    blk_sz_cnt = block_size_count;
    uint8_t* brk = g_pool_heap;
    blk_szs = (uint16_t*)brk;

    printf("g_pool_heap: %p\n", g_pool_heap);
    printf("blk_szs:     %p\n", blk_szs);
    printf("brk:         %p\n", brk);
    printf("bytes used:  %ld\n", brk - g_pool_heap);
    printf("\n");

    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        *((uint16_t*)brk) = (uint16_t)(block_sizes[i]);
        brk += sizeof(uint16_t);
        //printf("bytes used: %ld\n", brk - g_pool_heap);
    }

    /* init heap management data at beginning of heap */
    printf("g_pool_heap: %p\n", g_pool_heap);
    printf("blk_szs:     %p\n", blk_szs);
    printf("brk:         %p\n", brk);
    printf("bytes used:  %ld\n", brk - g_pool_heap);
    printf("\n");

    //printf("blk_szs size: %ld\n", brk - g_pool_heap);
    //for (uint8_t i = 0; i < blk_sz_cnt; i++) {
    //    printf("blk_szs{%d]: %p\n",i, &blk_szs[i]);
    //    printf("blk_szs[%d]: %d\n",i, blk_szs[i]);
    //}

    /* reserve space for pool base address pointers */
    pool_base_addrs = (uint8_t**)brk;
    brk += blk_sz_cnt * sizeof(uint8_t*);

    printf("g_pool_heap: %p\n", g_pool_heap);
    printf("blk_szs:     %p\n", blk_szs);
    printf("pool_bs_add: %p\n", pool_base_addrs);
    printf("brk:         %p\n", brk);
    printf("bytes used:  %ld\n", brk - g_pool_heap);
    printf("\n");
    
    /* reserve space for pool base address pointers */
    blk_alloc = (uint8_t**)brk;
    brk += blk_sz_cnt * sizeof(uint8_t*);

    printf("g_pool_heap: %p\n", g_pool_heap);
    printf("blk_szs:     %p\n", blk_szs);
    printf("pool_bs_add: %p\n", pool_base_addrs);
    printf("blk_alloc:   %p\n", blk_alloc);
    printf("brk:         %p\n", brk);
    printf("bytes used:  %ld\n", brk - g_pool_heap);
    printf("\n");

    /* compute bytes available after reserving heap memory management */
    uint16_t bytes_free = sizeof(g_pool_heap) - (brk - g_pool_heap);
    printf("bytes [total: %ld] [free: %d] [used: %ld]\n", bytes_free + (brk - g_pool_heap), bytes_free, brk - g_pool_heap);

    uint16_t pool_size = bytes_free / block_size_count;
    printf("heap partitions: %d\n", pool_size);
    printf("heap remainder: %ld\n", bytes_free % block_size_count);

    /* compute and assign pool_base_addrs */
    /* note: at this point brk is after next_alloc which should be pool_base_addr[0] */
    /* note: we can also init blk_alloc because the first portion of each blk is ptr */
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        printf("pool_bases_addrs: %p\n", pool_base_addrs[i]);
        printf("blk_alloc:        %p\n", blk_alloc[i]);
        pool_base_addrs[i] = (uint8_t*)brk + (pool_size * i);
        blk_alloc[i] = pool_base_addrs[i];
        printf("pool_bases_addrs: %p\n", pool_base_addrs[i]);
        printf("blk_alloc:        %p\n", blk_alloc[i]);
    }
    printf("\n");

    /* calculate number of blocks that can fit in each pool */
    /* note: memory size for each block sizeof(uint8_t*) + blk_szs[i] */
    uint16_t blk_mem_req = 0;
    uint16_t pool_blks = 0;
    uint16_t bytes_remainder = 0;
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        printf("blk_szs[%d]: %d\n", i, blk_szs[i]);
        blk_mem_req = blk_szs[i] + sizeof(uint8_t*);
        printf("blk_mem_req: %d\n", blk_mem_req);
        pool_blks = pool_size / blk_mem_req; 
        bytes_remainder = pool_size % blk_mem_req;
        printf("pool_blks:   %d\n", pool_blks);
        printf("bytes_rmdr:  %d\n", bytes_remainder);
        printf("req*blks+rmd:%d\n", blk_mem_req * pool_blks + bytes_remainder);

        assert(pool_base_addrs[i] == brk);

        printf("pool_bs_add: %p\n", pool_base_addrs[i]);
        printf("brk:         %p\n", brk);
        uint8_t* temp;
        /* create next ptr links */
        /* loop one short of pool_blks bc last blk needs to point to null - we'll do it manually */
        for (uint16_t j = 0; j < (pool_blks - 1); j++) {
            /* cast brk to uint8_t prt and deref, assign to brk + sizeof(uint8_t*) + blk_sz[i] */
            printf("brk:         %p\n", brk);
            temp = brk;
            *((uint8_t**)brk) = brk + sizeof(uint8_t*) + blk_szs[i];
            printf("brk:         %p\n", brk);
            printf("*brk:         %p\n", *((uint8_t**)brk));
            brk += sizeof(uint8_t*) + blk_szs[i];
            printf("brk:         %p\n", brk);
            printf("size:       %ld\n", brk - temp);
        }

        /* do last block manually */
        *((uint8_t**)brk) = NULL;
        printf("*brk:         %p\n", *((uint8_t**)brk));
        brk += sizeof(uint8_t*) + blk_szs[i];
        printf("brk:         %p\n", brk);

        brk += bytes_remainder;
        printf("brk:         %p\n", brk);
        printf("pool_bases_addrs: %p\n", pool_base_addrs[i]);
        printf("\n");
    }

    assert((brk + (bytes_free % block_size_count)) - g_pool_heap == sizeof(g_pool_heap));
    printf("brk:         %p\n", brk);
    printf("heap remainder: %ld\n", bytes_free % block_size_count);
    printf("total: %p\n", brk + (bytes_free % block_size_count));
    printf("net: %lu\n", (brk + (bytes_free % block_size_count)) - g_pool_heap);

    /* heap init complete */
    g_pool_heap_init = true;
    
    return true;
}

void* pool_malloc(size_t n)
{
 // Implement me!
 return NULL;
}

void pool_free(void* ptr)
{
 // Implement me!
}

void pool_print() {
    printf("+-----------------------------------------------------+\n");
    printf("|                  heap data header                   |\n");
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
    uint8_t* blk = NULL;
    for (uint8_t i = 0; i < blk_sz_cnt; i++) {
        printf("------- POOL[%u] -------\n", i);
        printf("blk_szs[%d]:         [addr: %p] [val: %d] \n", i, &blk_szs[i], blk_szs[i]);
        printf("pool_base_addrs[%d]: [addr: %p] [val: %p]\n", i, &pool_base_addrs[i], pool_base_addrs[i]);
        printf("block mem req: %ld\n", blk_szs[i] + sizeof(uint8_t*));
        blk = pool_base_addrs[i];

        uint16_t blks = 0;
        while (blk != NULL) {
            printf("blk[%u]: [addr: %p] [*brk[%u]: %p] [delta val: %lu]\n", blks, blk, blks, *((uint8_t**)blk), *((uint8_t**)blk) - blk);
            blk = *((uint8_t**)blk);
            blks++;
        }
        printf("\n");
    }
}














