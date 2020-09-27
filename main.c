#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "pool_alloc.h"

int main() {
    //printf("size of %ld\n", sizeof(uint16_t *));
    
    bool result = false;
    //size_t blk_szs[] = {32, 16, 128, 512, 1024};
    size_t blk_szs[] = {2048, 4096, 2048, 1000, 3333};
    size_t blk_sz_cnt = 5;

    result = pool_init(blk_szs, blk_sz_cnt);
    printf("%s\n", result ? "true" : "false");

    /* test pointer conversion */
    //uint8_t var = 1;
    //uint8_t* ptr = &var;
    //printf("ptr: %p\n", ptr);
    //printf("hdr_to_data: %p\n", blk_hdr_to_data(ptr));
    //ptr = &var;
    //printf("data_to_hdr: %p\n", blk_data_to_hdr(ptr));

    /* test heap */
    //heap_print();
    //pool_print(4);
    return 0;
}
