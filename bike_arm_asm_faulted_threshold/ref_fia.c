#include <stdio.h>
#include <stdint.h>

#  define THRESHOLD_COEFF0 1525880000ULL
#  define THRESHOLD_COEFF1 526500ULL
#  define THRESHOLD_MUL_CONST 12379400392853802749ULL
#  define THRESHOLD_SHR_CONST 26
#  define THRESHOLD_MIN 52


#define MUL64HIGH(c, a, b)                           \
  do {                                               \
    uint64_t a_lo, a_hi, b_lo, b_hi;                 \
    a_lo = a & 0xffffffff;                           \
    b_lo = b & 0xffffffff;                           \
    a_hi = a >> 32;                                  \
    b_hi = b >> 32;                                  \
    c = a_hi*b_hi + ((a_hi*b_lo + a_lo*b_hi) >> 32); \
  } while(0)

uint32_t secure_l32_mask(const uint32_t v1, const uint32_t v2)
{
#if defined(__aarch64__)
  uint32_t res;
  __asm__ __volatile__("cmp  %w[V2], %w[V1]; \n "
                       "cset %w[RES], HI; \n"
                       : [RES] "=r"(res)
                       : [V1] "r"(v1), [V2] "r"(v2)
                       : "cc" /*The condition code flag*/);
  return (res - 1);
#elif defined(__x86_64__) || defined(__i386__)
  uint32_t res;
  __asm__ __volatile__("xor  %%edx, %%edx; \n"
                       "cmp  %1, %2; \n "
                       "setl %%dl; \n"
                       "dec %%edx; \n"
                       "mov %%edx, %0; \n"

                       : "=r"(res)
                       : "r"(v2), "r"(v1)
                       : "rdx");

  return res;
#else
  // If v1 >= v2 then the subtraction result is 0^32||(v1-v2).
  // else it is 1^32||(v2-v1+1). Subsequently, negating the upper
  // 32 bits gives 0 if v1 < v2 and otherwise (-1).
  return ~((uint32_t)(((uint64_t)v1 - (uint64_t)v2) >> 32));
#endif
}

uint8_t get_threshold(uint64_t syndrome_weight) {
  // const uint64_t syndrome_weight = r_bits_vector_weight((const r_t *)s->qw);

  uint64_t thr  = THRESHOLD_COEFF0 + (THRESHOLD_COEFF1 * syndrome_weight);
  MUL64HIGH(thr, thr, THRESHOLD_MUL_CONST);
  thr >>= THRESHOLD_SHR_CONST;

  // const uint32_t mask = secure_l32_mask((uint32_t)thr, THRESHOLD_MIN);
  uint32_t mask;
  // if (FIA_CURRENT_BGF_ITERATION == FIA_FAULT_ON_BGF_ITERATION) {
  // mask = thr;
  // }
  // else {
  mask = secure_l32_mask((uint32_t)thr, THRESHOLD_MIN);
  // }


  // thr = (u32_barrier(mask) & thr) | (u32_barrier(~mask) & THRESHOLD_MIN); // Code without any simulated fault

  // if (FIA_CURRENT_BGF_ITERATION == FIA_FAULT_ON_BGF_ITERATION) {
  //   thr = (u32_barrier(mask)) | (u32_barrier(~mask) & THRESHOLD_MIN);  // Simulate the skipping fault of the AND
  // }
  // else {
  thr = ((mask) & thr) | ((~mask) & THRESHOLD_MIN); // Normal behavior
  // }

  return thr;
}


int main(void) {
    for (uint64_t w = 0; w < 10000; w += 100) {
        uint8_t thr = get_threshold(w);
        printf("get_threshold(%lu) = %u\n", w, thr);
    }

    return 0;
}