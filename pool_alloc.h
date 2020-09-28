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

/* define constraints */
#define MIN_POOLS 1
#define MAX_POOLS 16
#define MIN_BLOCK_SIZE 1
#define MAX_BLOCK_SIZE 2048 

/* Description: initialize pool allocator
 * Args: block_sizes - array containing block size of each pool
 *       block_size_count - number of pools
 * Return: true on success, false on failure
 */
bool pool_init(const size_t* block_sizes, size_t block_size_count);

/* Description: allocate n bytes
 * Args: n - size of memory to be allocated
 * Return: pointer to allocated memory on success, NULL on failure
 */
void* pool_malloc(size_t n);

/* Description: free allocated block 
 * Args: ptr - pointer to block to be freed
 * Return: void
 */
void pool_free(void* ptr);

/* Description: convert pointer to block header to pointer to block data section 
 * Args: ptr - pointer to block header
 * Return: ptr to block data section 
 */
uint8_t* blk_hdr_to_data(uint8_t* ptr);

/* Description: convert pointer to block data section to pointer to block header 
 * Args: ptr - pointer to block data section 
 * Return: ptr to block header
 */
uint8_t* blk_data_to_hdr(uint8_t* ptr);

/* Description: print heap mgmt data and all pools
 * Args: void
 * Return: void
 */
void heap_print(void);

/* Description: print info and blocks for a given pool
 * Args: pool index 
 * Return: void
 */
void pool_print(uint8_t pool);

#endif // __POOL_ALLOC_H__
