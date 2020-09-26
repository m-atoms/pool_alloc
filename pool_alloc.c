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
#include <pool_alloc.h>

static uint8_t g_pool_heap[65536];

bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
 // Implement me!
}

void* pool_malloc(size_t n)
{
 // Implement me!
}

void pool_free(void* ptr)
{
 // Implement me!
}
