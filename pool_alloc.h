/*****************************************************
 *
 * Author: Michael Adams
 *
 * Description: tunable block pool allocator header
 *
 ****************************************************/

#ifndef __POOL_ALLOC_H__
#define __POOL_ALLOC_H__

#include <stdbool.h>

// TODO: explain choices
#define MIN_POOLS 1
#define MAX_POOLS 16
#define MIN_BLOCK_SIZE 1
#define MAX_BLOCK_SIZE 4096 
//#define MAX_BLOCK_SIZE 1024

// Initialize the pool allocator with a set of block sizes appropriate 
// Returns true on success, false on failure.
bool pool_init(const size_t* block_sizes, size_t block_size_count);

// Allocate n bytes.
// Returns pointer to allocate memory on success, NULL on failure.
void* pool_malloc(size_t n);

// Release allocation pointed to by ptr.
void pool_free(void* ptr);

void pool_print(void);

#endif // __POOL_ALLOC_H__
