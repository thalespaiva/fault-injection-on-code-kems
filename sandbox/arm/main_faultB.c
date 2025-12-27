#include <stdio.h>
#include <stdint.h>
#include <assert.h>

uint8_t get_threshold1(uint64_t syndrome_weight);
uint8_t get_threshold3(uint64_t syndrome_weight);
uint8_t get_threshold5(uint64_t syndrome_weight);

#define R_BITS_LEVEL1 12323
#define R_BITS_LEVEL3 24659
#define R_BITS_LEVEL5 40973


uint8_t expected_get_threshold1(uint64_t w) {
    if (w < 3223) return 36;
    return 255;
}


uint8_t expected_get_threshold3(uint64_t w) {
    if (w < 6979) return 52;
    return 255;
}

uint8_t expected_get_threshold5(uint64_t w) {
    if (w < 12707) return 69;
    return 255;
}

int test(void) {
    printf("x = [");
    // for (uint64_t w = 0; w < 10000; w += 100) {
    for (uint64_t w = 0; w <= R_BITS_LEVEL5; w += 1) {
        uint8_t thr = get_threshold5(w);
        // assert(thr == expected_get_threshold5(w));
        // printf("get_threshold(%llu) = %u\n", w, thr);
        printf("%u,", thr);
        fflush(stdout);
    }
    printf("]\n");

    return 0;
}

int main(void) {

    for (uint64_t w = 0; w <= R_BITS_LEVEL1; w += 1) {
        uint8_t thr = get_threshold1(w);
        assert(thr == expected_get_threshold1(w));
    }
    printf("Level 1 (FAULT B) expected get_threshold OK\n");


    for (uint64_t w = 0; w <= R_BITS_LEVEL3; w += 1) {
        uint8_t thr = get_threshold3(w);
        assert(thr == expected_get_threshold3(w));
    }
    printf("Level 3 (FAULT B) expected get_threshold OK\n");

    for (uint64_t w = 0; w <= R_BITS_LEVEL5; w += 1) {
        uint8_t thr = get_threshold5(w);
        assert(thr == expected_get_threshold5(w));
    }
    printf("Level 5 (FAULT B) expected get_threshold OK\n");

    return 0;
}