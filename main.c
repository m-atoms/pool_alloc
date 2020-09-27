#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "pool_alloc.h"

int main() {
    //printf("size of %ld\n", sizeof(uint16_t *));
    
    bool result = false;
    //size_t blk_szs[] = {32, 16, 128, 512, 1024};
    size_t blk_szs[] = {4096, 4096, 4096, 4096, 4096};
    size_t blk_sz_cnt = 5;

    result = pool_init(blk_szs, blk_sz_cnt);
    printf("%s\n", result ? "true" : "false");
    return 0;
}
