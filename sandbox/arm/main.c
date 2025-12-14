#include <stdio.h>
#include <stdint.h>

uint8_t get_threshold(uint64_t syndrome_weight);  // forward declaration

int main(void) {
    // uint64_t test_values[] = {
    //     0,
    //     10,
    //     50,
    //     52,
    //     53,
    //     1000000ULL,
    //     123456789ULL
    // };

    for (uint64_t w = 0; w < 10000; w += 100) {
        uint8_t thr = get_threshold(w);
        printf("get_threshold(%llu) = %u\n", w, thr);
    }

    return 0;
}