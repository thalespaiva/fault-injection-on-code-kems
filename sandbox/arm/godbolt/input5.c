#include <stdint.h>

// Return 0 if v1 < v2, (-1) otherwise
static __inline__ uint32_t secure_l32_mask(const uint32_t v1, const uint32_t v2) {
  // we normally leverage some special CPU instructions such as "setl"
  // (for __x86_64__) and "cset" (for __aarch64__). When dealing with general
  // CPU architectures, the interpretation of the line below is left for the
  // compiler. It could lead to an "insecure" branch. This case needs to be
  // checked individually on such platforms
  // (e.g., by checking the compiler-generated assembly).
  return ~((uint32_t)(((uint64_t)v1 - (uint64_t)v2) >> 32));
}
#define _INLINE_ static inline

#  define THRESHOLD_COEFF0 1787850000ULL
#  define THRESHOLD_COEFF1 402312ULL
#  define THRESHOLD_MUL_CONST 12379400392853802749ULL
#  define THRESHOLD_SHR_CONST 26
#  define THRESHOLD_MIN 69

#define MUL64HIGH(c, a, b)                           \
  do {                                               \
    uint64_t a_lo, a_hi, b_lo, b_hi;                 \
    a_lo = a & 0xffffffff;                           \
    b_lo = b & 0xffffffff;                           \
    a_hi = a >> 32;                                  \
    b_hi = b >> 32;                                  \
    c = a_hi*b_hi + ((a_hi*b_lo + a_lo*b_hi) >> 32); \
  } while(0)

#if(defined(__GNUC__) || defined(__clang__))
#  define VALUE_BARRIER(name, type)            \
    _INLINE_ type name##_barrier(type a)       \
    {                                          \
      __asm__("" : "+r"(a) : /* no inputs */); \
      return a;                                \
    }
#else
#  define VALUE_BARRIER(name, type) \
    _INLINE_ type name##_barrier(type a) { return a; }
#endif


VALUE_BARRIER(u8, uint8_t)
VALUE_BARRIER(u32, uint32_t)
VALUE_BARRIER(u64, uint64_t)


uint8_t get_threshold(uint64_t syndrome_weight) {
  // const uint64_t syndrome_weight = r_bits_vector_weight((const r_t *)s->qw);

  uint64_t thr  = THRESHOLD_COEFF0 + (THRESHOLD_COEFF1 * syndrome_weight);
  MUL64HIGH(thr, thr, THRESHOLD_MUL_CONST);
  thr >>= THRESHOLD_SHR_CONST;

  const uint32_t mask = secure_l32_mask((uint32_t)thr, THRESHOLD_MIN);
  thr = (u32_barrier(mask) & thr) | (u32_barrier(~mask) & THRESHOLD_MIN);

  return thr;
}


