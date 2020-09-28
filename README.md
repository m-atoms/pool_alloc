# pool_alloc: Tunable Block Pool Allocator

## Introduction
This allocator is optimzied for allocating objects of specific sizes which are known at initialization time. The allocator is configured by the user with the set of block sizes that are appropriate for the application. Internally, the allocator creates block pools for each of the specified sizes.

## Usage
**Build Options**

By default, the Makefile passes two predefined macros into the test application
* VERBOSE - print test info and operation results
* FUNCTIONAL_TEST - perform more detailed test with step-by-step explanations

If removed, the test application will print only final success/failure result

**Build**
```bash
make all
```
**Execute**
```bash
./pool_test
```

## Design
## Constraints
## Tradeoffs

### Heap Organization
beginning of heap reserved for heap management data
remaining bytes divided evenly among n pools
brk = g_pool_heap_max or brk = (g_pool_heap_max - 1) depending on whether n is even or odd
n = number of pools
m = number of blocks in pool
```
       data locations                heap                 data sizes
                                     
       g_pool_heap_max -> +--------------------------+
                          |                          |
                          |         pool[n]          | <- (sizeof(uint8_t*) + blk_szs[n]) * m
                          |                          |
    pool_base_addrs[n] -> |--------------------------|
                          |          . . .           |
                          |--------------------------|
                          |                          |
                          |         pool[0]          | <- (sizeof(uint8_t*) + blk_szs[0]) * m
                          |                          |
    pool_base_addrs[0] -> |--------------------------|
                          |  next block alloc addrs  | <- sizeof(uint8_t*) * n
             blk_alloc -> |--------------------------|
                          |   pool base addresses    | <- sizeof(uint8_t*) * n
       pool_base_addrs -> |--------------------------| 
                          |      block sizes         | <- sizeof(uint16_t) * n
g_pool_heap && blk_szs -> +--------------------------+
```
### Pool Organization
```
       data locations                pool                 data sizes
                                     
  pool_base_addrs[n+1] -> +--------------------------+
                          |      bytes remaining     | <- pool_size % (sizeof(uint8_t) + blk_szs[n])
                          |--------------------------|
                          |                          |
                          |       block_data[m]      | <- blk_szs[n]
                          |                          | 
                          |--------------------------|
                          |      block_header[m]     | <- sizeof(uint8_t*)
                          |--------------------------|
                          |          . . .           |
                          |--------------------------|
                          |                          |
                          |       block_data[0]      | <- blk_szs[n]
                          |                          |
                          |--------------------------|
                          |      block_header[0]     | <- sizeof(uint8_t*)
    pool_base_addrs[n] -> +--------------------------+
```
