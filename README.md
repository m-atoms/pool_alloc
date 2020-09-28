# pool_alloc: Tunable Block Pool Allocator

## Introduction
This allocator is optimzied for allocating objects of specific sizes which are known at initialization time. The allocator is configured by the user with the set of block sizes that are appropriate for the application. Internally, the allocator creates block pools for each of the specified sizes.

## Usage
#### Build Options

By default, the Makefile passes two predefined macros into the test application
* VERBOSE - print test info and operation results
* FUNCTIONAL_TEST - perform more detailed test with step-by-step explanations

If removed, the test application will print only final success/failure result

#### Build
```bash
make all
```
#### Execute
```bash
./pool_test
```

## Design
#### Assumptions
| Assumption | Justification |
|------------|---------------|
| cannot use malloc() | memory footprint of allocator must be fixed |
| allocation time is high priority | assuming high performance embedded application where memory allocation must be time efficient |
| heap space may be divided evenly among pools | equal sized pools produce greater block numbers with smaller block sizes, assume smaller blocks are more commonly allocated than very large blocks - reasonable if lightweight high performance system where speed is critical |
| only sizes specified in list will be allocated | maximizes simplicity and efficiency of using a block pool allocator |
| all block sizes in list are unique | simplifying assumption extending from approximate relative importance of each block size  |

#### Constraints
| Feature | Constraint |
|---------|------------|
| Min Pools | 1 |
| Max Pools | 16 |
| Min Block Size | 1 |
| Max Block Size | 2048 |

Total heap size is only 64kB. Limiting maximum allowable pools to 16 provides a reasonable amount of variety for various applications while ensuring that each pool has a reasonable amount of space (~4kB). Additionally, the benefit of using a block pool allocator decreases as the number of required pools increases. Maximum block size of 2048B means that even in the worst case with 16 pools, pool will have at least 1 block. This constraint prevents any pool from having 0 blocks. Based on the assumption that most blocks will be smaller in size and few pools will be needed (lightweight, highly specific application), most pools will have many blocks.

## Implementation
The block pool allocator uses the lower portion of heap to store management info about the pools. From the remaining space, the pools are divided into even partitions. In the heap management section, the block size, pool base address, and location of the next block to be allocated is stored for each pool. Each pool is filled with as many blocks as can fit into the pool space. To provide O(1) block allocation time and reduce the amount of external state variables required, a pointer list scheme is used to track the available blocks and provide immediate access to next block to be allocated for a given pool.

#### Heap Organization
n = number of pools

m = number of blocks in pool

note: brk = g_pool_heap_max or brk = (g_pool_heap_max - 1) depending on whether n is even or odd
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
#### Pool Organization
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
#### Tradeoff Discussion
Optimizing for O(1) block allocation time requires each block to have an associated pointer. The size of this pointer varies based on processor word size. On a 64-Bit machine, each block header requires 8 bytes which significantly reduces the space efficiency of small block size pools. As block sizes increase, the relative inefficiency of the header decreases. Additionally, on systems with smaller processor word sizes, the space impact of storing a pointer with each data block decreases (ex: 4-Byte pointer on 32-Bit machine, 2-Byte pointer on 16-Bit machine). For better storage space overhead but slower allocation performance, use a bitmap to store heap usage information.

Dividing the heap into even pool sizes presents a fair and reasonable distribution of space without knowing system specifics and assuming generally smaller block sizes. It also simplifies implementation. Note however that this approach can produce significant waste for larger block sizes when the bytes remaining (see pool org) size is only slightly less than the block size itself.
