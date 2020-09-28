#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "pool_alloc.h"

int main() {
    /********************************/
    /******* pool_init() test *******/
    /********************************/
#ifdef VERBOSE
    printf("\n------- pool_init() test -------\n");
#endif

    /* test valid pool array */
    {
        bool result = true;
        size_t* block_sizes = NULL;
        size_t block_size_count = 5;
        result = pool_init(block_sizes, block_size_count);
        assert(result == false);
    }

    /* test num pool constraints */
    {   // too few pools
        bool result = true;
        size_t block_sizes[] = { MIN_BLOCK_SIZE, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE };
        size_t block_size_count = MIN_POOLS - 1;
        result = pool_init(block_sizes, block_size_count);
        assert(result == false);
    }

    {   // too many pools
        bool result = true;
        size_t block_sizes[] = { MIN_BLOCK_SIZE, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE };
        size_t block_size_count = MAX_POOLS + 1;
        result = pool_init(block_sizes, block_size_count);
        assert(result == false);
    }

    /* test block size constraints */
    {   // block size too small
        bool result = true;
        size_t block_sizes[] = { MIN_BLOCK_SIZE - 1, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE };
        size_t block_size_count = 4;
        result = pool_init(block_sizes, block_size_count);
        assert(result == false);
    }

    {   // block size too large
        bool result = true;
        size_t block_sizes[] = { MAX_BLOCK_SIZE + 1, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE, MIN_BLOCK_SIZE };
        size_t block_size_count = 4;
        result = pool_init(block_sizes, block_size_count);
        assert(result == false);
    }
    
    /* [pool_malloc] - test pool uninit */
    {
        void* blk0 = pool_malloc(10);
        assert(blk0 == NULL);
    }

    /* [pool_free] - test pool uninit - enable VERBOSE to verify */
    {
        pool_free(NULL);
    }

    /* test valid pool init */
    {
        bool result = false;
        size_t block_sizes[] = { 32, 128, 400, 512, 2048 };
        size_t block_size_count = 5;
        result = pool_init(block_sizes, block_size_count);
        assert(result == true);
    }

    /* test pool reinit */
    {
        bool result = true;
        size_t block_sizes[] = { 32, 128, 400, 512, 2048 };
        size_t block_size_count = 5;
        result = pool_init(block_sizes, block_size_count);
        assert(result == false);
    }
    
    /**********************************/
    /******* pool_malloc() test *******/
    /**********************************/
#ifdef VERBOSE
    printf("\n------- pool_malloc() test -------\n");
#endif

    /* test malloc on uninit pool - see pool_init() test */

    /* test invalid malloc sizes */
    {   // too small
        void* result_ptr = pool_malloc(MIN_BLOCK_SIZE - 1);
        assert(result_ptr == NULL);
    }

    {   // too large 
        void* result_ptr = pool_malloc(MAX_BLOCK_SIZE + 1);
        assert(result_ptr == NULL);
    }

    {   // too large 
        void* result_ptr = pool_malloc(33);
        assert(result_ptr == NULL);
    }

    /* declare 7 blk pointers to test 6 size 2048 blocks */
    void* blk0 = NULL;
    void* blk1 = NULL;
    void* blk2 = NULL;
    void* blk3 = NULL;
    void* blk4 = NULL;
    void* blk5 = NULL;
    void* blk6 = NULL;

    /* test valid malloc - all blocks */
    {
        blk0 = pool_malloc(2048);
        blk1 = pool_malloc(2048); 
        blk2 = pool_malloc(2048);
        blk3 = pool_malloc(2048);
        blk4 = pool_malloc(2048);
        blk5 = pool_malloc(2048);

        assert(blk0 != NULL &&
               blk1 != NULL &&
               blk2 != NULL &&
               blk3 != NULL &&
               blk4 != NULL &&
               blk5 != NULL); 
    }

    /* test malloc when no blocks are free */
    {
        blk6 = pool_malloc(2048);
        assert(blk6 == NULL);
    }

    /********************************/
    /******* pool_free() test *******/
    /********************************/
#ifdef VERBOSE
    printf("\n------- pool_free() test -------\n");
#endif

    /* test free on uninit pool - see pool_init() test */

    /* test free when NULL pointer - enable VERBOSE to verify */
    {   
        pool_free(NULL);
    }

    /* test free with invalid pointer - enable VERBOSE to verify */
    {   // too low
        pool_free((void*)0x00000001);
    }

    {   // too low
        pool_free((void*)0xFFFFFFFF);
    }

    {   // not aligned to block 
        pool_free((void*)blk0 - 2);
    }

    /* test valid free - prove no free blocks, then free one, then retest malloc */
    {
        /* make sure no free block to allocate */
        blk6 = pool_malloc(2048);
        assert(blk6 == NULL);

        /* free block */
        pool_free(blk0);

        /* allocate to prove block was freed */
        blk6 = pool_malloc(2048);
        assert(blk6 != NULL);
    }

    /* test free on already freed block - enable VERBOSE to verify */
    {
        pool_free(blk6);
        pool_free(blk6);
    }

    /********************************/
    /******* functional test ********/
    /********************************/
#ifdef FUNCTIONAL_TEST
    printf("\n------- functional test -------\n");

    /* STEP[0] */
    printf("\nSTEP[0]: free all blocks, expect all 6 blocks listed (therefore free)\n");
    pool_free(blk1);
    pool_free(blk2);
    pool_free(blk3);
    pool_free(blk4);
    pool_free(blk5);
    pool_print(4);

    /* STEP[1] */
    printf("\nSTEP[1]: allocate 1 block, expect 5 free\n");
    blk0 = pool_malloc(2048);
    pool_print(4);

    /* STEP[2] */
    printf("\nSTEP[2]: allocate all remaining blocks and try one extra, expect 0 free\n");
    blk1 = pool_malloc(2048);
    blk2 = pool_malloc(2048);
    blk3 = pool_malloc(2048);
    blk4 = pool_malloc(2048);
    blk5 = pool_malloc(2048);
    blk6 = pool_malloc(2048);
    pool_print(4);

    /* STEP[3] */
    printf("\nSTEP[3]: free blocks at beginning, middle, and end of list, expect 3 free \n");
    pool_free(blk0);
    pool_free(blk3);
    pool_free(blk5);
    pool_print(4);

    /* STEP[4] */
    printf("\nSTEP[4]: reallocate 3 freed, expect 0 free \n");
    blk0 = NULL;
    blk3 = NULL;
    blk5 = NULL;
    blk0 = pool_malloc(2048);
    blk3 = pool_malloc(2048);
    blk5 = pool_malloc(2048);
    assert(blk0 != NULL && blk3 != NULL && blk5 != NULL);
    pool_print(4);

    /* STEP[5] */
    printf("\nSTEP[5]: free all, expect 6 free - note free list same as STEP[0] but in new order\n");
    pool_free(blk0);
    pool_free(blk1);
    pool_free(blk2);
    pool_free(blk3);
    pool_free(blk4);
    pool_free(blk5);
    pool_print(4);
#endif

    /* if we made it here then everything passed */
    printf("\nSUCCESS: all assertions passed\n:");
    return 0;
}
