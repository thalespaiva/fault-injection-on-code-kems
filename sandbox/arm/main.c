#include <stdio.h>
#include <stdint.h>

uint8_t get_threshold1(uint64_t syndrome_weight);
uint8_t get_threshold2(uint64_t syndrome_weight);
uint8_t get_threshold3(uint64_t syndrome_weight);

int main(void) {
    for (uint64_t w = 0; w < 10000; w += 100) {
        uint8_t thr = get_threshold3(w);
        printf("get_threshold(%llu) = %u\n", w, thr);
    }

    return 0;
}
