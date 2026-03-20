/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0"
 *
 * Written by Nir Drucker, Shay Gueron and Dusan Kostic,
 * AWS Cryptographic Algorithms Group.
 *
 * [1] The optimizations are based on the description developed in the paper:
 *     Drucker, Nir, and Shay Gueron. 2019. “A Toolbox for Software Optimization
 *     of QC-MDPC Code-Based Cryptosystems.” Journal of Cryptographic Engineering,
 *     January, 1–17. https://doi.org/10.1007/s13389-018-00200-4.
 *
 * [2] The decoder algorithm is the Black-Gray decoder in
 *     the early submission of CAKE (due to N. Sandrier and R Misoczki).
 *
 * [3] The analysis for the constant time implementation is given in
 *     Drucker, Nir, Shay Gueron, and Dusan Kostic. 2019.
 *     “On Constant-Time QC-MDPC Decoding with Negligible Failure Rate.”
 *     Cryptology EPrint Archive, 2019. https://eprint.iacr.org/2019/1289.
 *
 * [4] it was adapted to BGF in:
 *     Drucker, Nir, Shay Gueron, and Dusan Kostic. 2019.
 *     “QC-MDPC decoders with several shades of gray.”
 *     Cryptology EPrint Archive, 2019. To be published.
 *
 * [5] Chou, T.: QcBits: Constant-Time Small-Key Code-Based Cryptography.
 *     In: Gier-lichs, B., Poschmann, A.Y. (eds.) Cryptographic Hardware
 *     and Embedded Systems– CHES 2016. pp. 280–300. Springer Berlin Heidelberg,
 *     Berlin, Heidelberg (2016)
 *
 * [6] The rotate512_small funciton is a derivative of the code described in:
 *     Guimarães, Antonio, Diego F Aranha, and Edson Borin. 2019.
 *     “Optimized Implementation of QC-MDPC Code-Based Cryptography.”
 *     Concurrency and Computation: Practice and Experience 31 (18):
 *     e5089. https://doi.org/10.1002/cpe.5089.
 */

#include "decode.h"
#include "cleanup.h"
#include "decode_internal.h"
#include "gf2x.h"
#include "utilities.h"

extern int FIA_FAULT_ON_BGF_ITERATION; // The iteration where the fault will be applied.
int FIA_CURRENT_BGF_ITERATION = -1;  // Globally stores the current BGF iteration so that `get_threshold` can see it.

extern int global_synd_weight;

// Decoding (bit-flipping) parameter
#if defined(BG_DECODER)
#  if(LEVEL == 1)
#    define MAX_IT 3
#  elif(LEVEL == 3)
#    define MAX_IT 4
#  else
#    error "Level can only be 1/3"
#  endif
#elif defined(BGF_DECODER)
#  define MAX_IT 5
#endif

void compute_syndrome(OUT syndrome_t *syndrome,
                      IN const pad_r_t *c0,
                      IN const pad_r_t *h0,
                      IN const decode_ctx *ctx)
{
  DEFER_CLEANUP(pad_r_t pad_s, pad_r_cleanup);

  gf2x_mod_mul(&pad_s, c0, h0);

  bike_memcpy((uint8_t *)syndrome->qw, pad_s.val.raw, R_BYTES);
  ctx->dup(syndrome);
}

_INLINE_ void recompute_syndrome(OUT syndrome_t *syndrome,
                                 IN const pad_r_t *c0,
                                 IN const pad_r_t *h0,
                                 IN const pad_r_t *pk,
                                 IN const e_t *e,
                                 IN const decode_ctx *ctx)
{
  DEFER_CLEANUP(pad_r_t tmp_c0, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t e0 = {0}, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t e1 = {0}, pad_r_cleanup);

  e0.val = e->val[0];
  e1.val = e->val[1];

  // tmp_c0 = pk * e1 + c0 + e0
  gf2x_mod_mul(&tmp_c0, &e1, pk);
  gf2x_mod_add(&tmp_c0, &tmp_c0, c0);
  gf2x_mod_add(&tmp_c0, &tmp_c0, &e0);

  // Recompute the syndrome using the updated ciphertext
  compute_syndrome(syndrome, &tmp_c0, h0, ctx);
}

#define MUL64HIGH(c, a, b)                           \
  do {                                               \
    uint64_t a_lo, a_hi, b_lo, b_hi;                 \
    a_lo = a & 0xffffffff;                           \
    b_lo = b & 0xffffffff;                           \
    a_hi = a >> 32;                                  \
    b_hi = b >> 32;                                  \
    c = a_hi*b_hi + ((a_hi*b_lo + a_lo*b_hi) >> 32); \
  } while(0)

_INLINE_ uint8_t get_threshold_faulted(IN const syndrome_t *s);

_INLINE_ uint8_t get_threshold(IN const syndrome_t *s)
{
  if (FIA_CURRENT_BGF_ITERATION == FIA_FAULT_ON_BGF_ITERATION) {
    return get_threshold_faulted(s);
  }

  bike_static_assert(sizeof(*s) >= sizeof(r_t), syndrome_is_large_enough);

  const uint64_t syndrome_weight = r_bits_vector_weight((const r_t *)s->qw);

  // The threshold coefficients are defined in the spec as floating point values.
  // Since we want to avoid floating point operations for constant-timeness,
  // we use integer arithmetic to compute the threshold.
  // For example, in the case of Level-1 parameters, instead of having:
  //   T0 = 13.530 and T1 = 0.0069722,
  // we multipy the values by 10^8 and work with integers:
  //   T0' = 1353000000 and T1' = 697220.
  // Then, instead of computing the threshold by:
  //   T0 + T1*S,
  // we compute:
  //   (T0' + T1'*S)/10^8,
  // where S is the syndrome weight. Additionally, instead of dividing by 10^8,
  // we compute the result by a multiplication and a right shift (both
  // constant-time operations), as described in:
  //   https://dl.acm.org/doi/pdf/10.1145/178243.178249

  uint64_t thr  = THRESHOLD_COEFF0 + (THRESHOLD_COEFF1 * syndrome_weight);
  MUL64HIGH(thr, thr, THRESHOLD_MUL_CONST);
  thr >>= THRESHOLD_SHR_CONST;

  const uint32_t mask = secure_l32_mask((uint32_t)thr, THRESHOLD_MIN);
  thr = (u32_barrier(mask) & thr) | (u32_barrier(~mask) & THRESHOLD_MIN); // Normal behavior

  DMSG("    Threshold: %ld\n", thr);
  return thr;
}

#if LEVEL == 1
_INLINE_ uint8_t get_threshold_faulted(IN const syndrome_t *s) {
  const uint64_t w = r_bits_vector_weight((const r_t *)s->qw);
  if (w < 3223) return 36;
  return 255;
}

#elif LEVEL == 3

_INLINE_ uint8_t get_threshold_faulted(IN const syndrome_t *s) {
  const uint64_t w = r_bits_vector_weight((const r_t *)s->qw);
  if (w < 46) return 52;
  if (w < 141) return 15;
  if (w < 236) return 52;
  if (w < 331) return 16;
  if (w < 426) return 52;
  if (w < 521) return 17;
  if (w < 616) return 52;
  if (w < 711) return 18;
  if (w < 806) return 52;
  if (w < 901) return 19;
  if (w < 996) return 52;
  if (w < 1091) return 20;
  if (w < 1186) return 52;
  if (w < 1281) return 21;
  if (w < 1376) return 52;
  if (w < 1471) return 22;
  if (w < 1566) return 52;
  if (w < 1661) return 23;
  if (w < 1756) return 52;
  if (w < 1851) return 24;
  if (w < 1946) return 52;
  if (w < 2041) return 25;
  if (w < 2136) return 52;
  if (w < 2231) return 26;
  if (w < 2326) return 52;
  if (w < 2420) return 27;
  if (w < 2515) return 52;
  if (w < 2610) return 28;
  if (w < 2705) return 52;
  if (w < 2800) return 29;
  if (w < 2895) return 52;
  if (w < 2990) return 30;
  if (w < 3085) return 52;
  if (w < 3180) return 31;
  if (w < 3275) return 52;
  if (w < 3370) return 32;
  if (w < 3465) return 52;
  if (w < 3560) return 33;
  if (w < 3655) return 52;
  if (w < 3750) return 34;
  if (w < 3845) return 52;
  if (w < 3940) return 35;
  if (w < 4035) return 52;
  if (w < 4130) return 36;
  if (w < 4225) return 52;
  if (w < 4320) return 37;
  if (w < 4415) return 52;
  if (w < 4510) return 38;
  if (w < 4605) return 52;
  if (w < 4700) return 39;
  if (w < 4795) return 52;
  if (w < 4890) return 40;
  if (w < 4985) return 52;
  if (w < 5080) return 41;
  if (w < 5175) return 52;
  if (w < 5269) return 42;
  if (w < 5364) return 52;
  if (w < 5459) return 43;
  if (w < 5554) return 52;
  if (w < 5649) return 44;
  if (w < 5744) return 52;
  if (w < 5839) return 45;
  if (w < 5934) return 52;
  if (w < 6029) return 46;
  if (w < 6124) return 52;
  if (w < 6219) return 47;
  if (w < 6314) return 52;
  if (w < 6409) return 48;
  if (w < 6504) return 52;
  if (w < 6599) return 49;
  if (w < 6694) return 52;
  if (w < 6789) return 50;
  if (w < 6884) return 52;
  if (w < 6979) return 51;
  if (w < 7264) return 52;
  if (w < 7359) return 53;
  if (w < 7454) return 52;
  if (w < 7549) return 54;
  if (w < 7644) return 52;
  if (w < 7739) return 55;
  if (w < 7834) return 52;
  if (w < 7929) return 56;
  if (w < 8024) return 52;
  if (w < 8118) return 57;
  if (w < 8213) return 52;
  if (w < 8308) return 58;
  if (w < 8403) return 52;
  if (w < 8498) return 59;
  if (w < 8593) return 52;
  if (w < 8688) return 60;
  if (w < 8783) return 52;
  if (w < 8878) return 61;
  if (w < 8973) return 52;
  if (w < 9068) return 62;
  if (w < 9163) return 52;
  if (w < 9258) return 63;
  if (w < 9353) return 52;
  if (w < 9448) return 64;
  if (w < 9543) return 52;
  if (w < 9638) return 65;
  if (w < 9733) return 52;
  if (w < 9828) return 66;
  if (w < 9923) return 52;
  if (w < 10018) return 67;
  if (w < 10113) return 52;
  if (w < 10208) return 68;
  if (w < 10303) return 52;
  if (w < 10398) return 69;
  if (w < 10493) return 52;
  if (w < 10588) return 70;
  if (w < 10683) return 52;
  if (w < 10778) return 71;
  if (w < 10873) return 52;
  if (w < 10967) return 72;
  if (w < 11062) return 52;
  if (w < 11157) return 73;
  if (w < 11252) return 52;
  if (w < 11347) return 74;
  if (w < 11442) return 52;
  if (w < 11537) return 75;
  if (w < 11632) return 52;
  if (w < 11727) return 76;
  if (w < 11822) return 52;
  if (w < 11917) return 77;
  if (w < 12012) return 52;
  if (w < 12107) return 78;
  if (w < 12202) return 52;
  if (w < 12297) return 79;
  if (w < 12392) return 52;
  if (w < 12487) return 80;
  if (w < 12582) return 52;
  if (w < 12677) return 81;
  if (w < 12772) return 52;
  if (w < 12867) return 82;
  if (w < 12962) return 52;
  if (w < 13057) return 83;
  if (w < 13152) return 52;
  if (w < 13247) return 84;
  if (w < 13342) return 52;
  if (w < 13437) return 85;
  if (w < 13532) return 52;
  if (w < 13627) return 86;
  if (w < 13722) return 52;
  if (w < 13816) return 87;
  if (w < 13911) return 52;
  if (w < 14006) return 88;
  if (w < 14101) return 52;
  if (w < 14196) return 89;
  if (w < 14291) return 52;
  if (w < 14386) return 90;
  if (w < 14481) return 52;
  if (w < 14576) return 91;
  if (w < 14671) return 52;
  if (w < 14766) return 92;
  if (w < 14861) return 52;
  if (w < 14956) return 93;
  if (w < 15051) return 52;
  if (w < 15146) return 94;
  if (w < 15241) return 52;
  if (w < 15336) return 95;
  if (w < 15431) return 52;
  if (w < 15526) return 96;
  if (w < 15621) return 52;
  if (w < 15716) return 97;
  if (w < 15811) return 52;
  if (w < 15906) return 98;
  if (w < 16001) return 52;
  if (w < 16096) return 99;
  if (w < 16191) return 52;
  if (w < 16286) return 100;
  if (w < 16381) return 52;
  if (w < 16476) return 101;
  if (w < 16571) return 52;
  if (w < 16665) return 102;
  if (w < 16760) return 52;
  if (w < 16855) return 103;
  if (w < 16950) return 52;
  if (w < 17045) return 104;
  if (w < 17140) return 52;
  if (w < 17235) return 105;
  if (w < 17330) return 52;
  if (w < 17425) return 106;
  if (w < 17520) return 52;
  if (w < 17615) return 107;
  if (w < 17710) return 52;
  if (w < 17805) return 108;
  if (w < 17900) return 52;
  if (w < 17995) return 109;
  if (w < 18090) return 52;
  if (w < 18185) return 110;
  if (w < 18280) return 52;
  if (w < 18375) return 111;
  if (w < 18470) return 52;
  if (w < 18565) return 112;
  if (w < 18660) return 52;
  if (w < 18755) return 113;
  if (w < 18850) return 52;
  if (w < 18945) return 114;
  if (w < 19040) return 52;
  if (w < 19135) return 115;
  if (w < 19230) return 52;
  if (w < 19325) return 116;
  if (w < 19420) return 52;
  if (w < 19514) return 117;
  if (w < 19609) return 52;
  if (w < 19704) return 118;
  if (w < 19799) return 52;
  if (w < 19894) return 119;
  if (w < 19989) return 52;
  if (w < 20084) return 120;
  if (w < 20179) return 52;
  if (w < 20274) return 121;
  if (w < 20369) return 52;
  if (w < 20464) return 122;
  if (w < 20559) return 52;
  if (w < 20654) return 123;
  if (w < 20749) return 52;
  if (w < 20844) return 124;
  if (w < 20939) return 52;
  if (w < 21034) return 125;
  if (w < 21129) return 52;
  if (w < 21224) return 126;
  if (w < 21319) return 52;
  if (w < 21414) return 127;
  if (w < 21509) return 52;
  if (w < 21604) return 128;
  if (w < 21699) return 52;
  if (w < 21794) return 129;
  if (w < 21889) return 52;
  if (w < 21984) return 130;
  if (w < 22079) return 52;
  if (w < 22174) return 131;
  if (w < 22269) return 52;
  if (w < 22364) return 132;
  if (w < 22458) return 52;
  if (w < 22553) return 133;
  if (w < 22648) return 52;
  if (w < 22743) return 134;
  if (w < 22838) return 52;
  if (w < 22933) return 135;
  if (w < 23028) return 52;
  if (w < 23123) return 136;
  if (w < 23218) return 52;
  if (w < 23313) return 137;
  if (w < 23408) return 52;
  if (w < 23503) return 138;
  if (w < 23598) return 52;
  if (w < 23693) return 139;
  if (w < 23788) return 52;
  if (w < 23883) return 140;
  if (w < 23978) return 52;
  if (w < 24073) return 141;
  if (w < 24168) return 52;
  if (w < 24263) return 142;
  if (w < 24358) return 52;
  if (w < 24453) return 143;
  if (w < 24548) return 52;
  if (w < 24643) return 144;
  return 52;
}

#elif LEVEL == 5
_INLINE_ uint8_t get_threshold_faulted(IN const syndrome_t *s) {
  const uint64_t w = r_bits_vector_weight((const r_t *)s->qw);
  if (w < 31) return 17;
  if (w < 155) return 69;
  if (w < 279) return 18;
  if (w < 404) return 69;
  if (w < 528) return 19;
  if (w < 652) return 69;
  if (w < 776) return 20;
  if (w < 901) return 69;
  if (w < 1025) return 21;
  if (w < 1149) return 69;
  if (w < 1274) return 22;
  if (w < 1398) return 69;
  if (w < 1522) return 23;
  if (w < 1646) return 69;
  if (w < 1771) return 24;
  if (w < 1895) return 69;
  if (w < 2019) return 25;
  if (w < 2143) return 69;
  if (w < 2268) return 26;
  if (w < 2392) return 69;
  if (w < 2516) return 27;
  if (w < 2641) return 69;
  if (w < 2765) return 28;
  if (w < 2889) return 69;
  if (w < 3013) return 29;
  if (w < 3138) return 69;
  if (w < 3262) return 30;
  if (w < 3386) return 69;
  if (w < 3511) return 31;
  if (w < 3635) return 69;
  if (w < 3759) return 32;
  if (w < 3883) return 69;
  if (w < 4008) return 33;
  if (w < 4132) return 69;
  if (w < 4256) return 34;
  if (w < 4381) return 69;
  if (w < 4505) return 35;
  if (w < 4629) return 69;
  if (w < 4753) return 36;
  if (w < 4878) return 69;
  if (w < 5002) return 37;
  if (w < 5126) return 69;
  if (w < 5251) return 38;
  if (w < 5375) return 69;
  if (w < 5499) return 39;
  if (w < 5623) return 69;
  if (w < 5748) return 40;
  if (w < 5872) return 69;
  if (w < 5996) return 41;
  if (w < 6121) return 69;
  if (w < 6245) return 42;
  if (w < 6369) return 69;
  if (w < 6493) return 43;
  if (w < 6618) return 69;
  if (w < 6742) return 44;
  if (w < 6866) return 69;
  if (w < 6990) return 45;
  if (w < 7115) return 69;
  if (w < 7239) return 46;
  if (w < 7363) return 69;
  if (w < 7488) return 47;
  if (w < 7612) return 69;
  if (w < 7736) return 48;
  if (w < 7860) return 69;
  if (w < 7985) return 49;
  if (w < 8109) return 69;
  if (w < 8233) return 50;
  if (w < 8358) return 69;
  if (w < 8482) return 51;
  if (w < 8606) return 69;
  if (w < 8730) return 52;
  if (w < 8855) return 69;
  if (w < 8979) return 53;
  if (w < 9103) return 69;
  if (w < 9228) return 54;
  if (w < 9352) return 69;
  if (w < 9476) return 55;
  if (w < 9600) return 69;
  if (w < 9725) return 56;
  if (w < 9849) return 69;
  if (w < 9973) return 57;
  if (w < 10098) return 69;
  if (w < 10222) return 58;
  if (w < 10346) return 69;
  if (w < 10470) return 59;
  if (w < 10595) return 69;
  if (w < 10719) return 60;
  if (w < 10843) return 69;
  if (w < 10967) return 61;
  if (w < 11092) return 69;
  if (w < 11216) return 62;
  if (w < 11340) return 69;
  if (w < 11465) return 63;
  if (w < 11589) return 69;
  if (w < 11713) return 64;
  if (w < 11837) return 69;
  if (w < 11962) return 65;
  if (w < 12086) return 69;
  if (w < 12210) return 66;
  if (w < 12335) return 69;
  if (w < 12459) return 67;
  if (w < 12583) return 69;
  if (w < 12707) return 68;
  if (w < 13080) return 69;
  if (w < 13205) return 70;
  if (w < 13329) return 69;
  if (w < 13453) return 71;
  if (w < 13577) return 69;
  if (w < 13702) return 72;
  if (w < 13826) return 69;
  if (w < 13950) return 73;
  if (w < 14075) return 69;
  if (w < 14199) return 74;
  if (w < 14323) return 69;
  if (w < 14447) return 75;
  if (w < 14572) return 69;
  if (w < 14696) return 76;
  if (w < 14820) return 69;
  if (w < 14944) return 77;
  if (w < 15069) return 69;
  if (w < 15193) return 78;
  if (w < 15317) return 69;
  if (w < 15442) return 79;
  if (w < 15566) return 69;
  if (w < 15690) return 80;
  if (w < 15814) return 69;
  if (w < 15939) return 81;
  if (w < 16063) return 69;
  if (w < 16187) return 82;
  if (w < 16312) return 69;
  if (w < 16436) return 83;
  if (w < 16560) return 69;
  if (w < 16684) return 84;
  if (w < 16809) return 69;
  if (w < 16933) return 85;
  if (w < 17057) return 69;
  if (w < 17182) return 86;
  if (w < 17306) return 69;
  if (w < 17430) return 87;
  if (w < 17554) return 69;
  if (w < 17679) return 88;
  if (w < 17803) return 69;
  if (w < 17927) return 89;
  if (w < 18052) return 69;
  if (w < 18176) return 90;
  if (w < 18300) return 69;
  if (w < 18424) return 91;
  if (w < 18549) return 69;
  if (w < 18673) return 92;
  if (w < 18797) return 69;
  if (w < 18922) return 93;
  if (w < 19046) return 69;
  if (w < 19170) return 94;
  if (w < 19294) return 69;
  if (w < 19419) return 95;
  if (w < 19543) return 69;
  if (w < 19667) return 96;
  if (w < 19791) return 69;
  if (w < 19916) return 97;
  if (w < 20040) return 69;
  if (w < 20164) return 98;
  if (w < 20289) return 69;
  if (w < 20413) return 99;
  if (w < 20537) return 69;
  if (w < 20661) return 100;
  if (w < 20786) return 69;
  if (w < 20910) return 101;
  if (w < 21034) return 69;
  if (w < 21159) return 102;
  if (w < 21283) return 69;
  if (w < 21407) return 103;
  if (w < 21531) return 69;
  if (w < 21656) return 104;
  if (w < 21780) return 69;
  if (w < 21904) return 105;
  if (w < 22029) return 69;
  if (w < 22153) return 106;
  if (w < 22277) return 69;
  if (w < 22401) return 107;
  if (w < 22526) return 69;
  if (w < 22650) return 108;
  if (w < 22774) return 69;
  if (w < 22899) return 109;
  if (w < 23023) return 69;
  if (w < 23147) return 110;
  if (w < 23271) return 69;
  if (w < 23396) return 111;
  if (w < 23520) return 69;
  if (w < 23644) return 112;
  if (w < 23768) return 69;
  if (w < 23893) return 113;
  if (w < 24017) return 69;
  if (w < 24141) return 114;
  if (w < 24266) return 69;
  if (w < 24390) return 115;
  if (w < 24514) return 69;
  if (w < 24638) return 116;
  if (w < 24763) return 69;
  if (w < 24887) return 117;
  if (w < 25011) return 69;
  if (w < 25136) return 118;
  if (w < 25260) return 69;
  if (w < 25384) return 119;
  if (w < 25508) return 69;
  if (w < 25633) return 120;
  if (w < 25757) return 69;
  if (w < 25881) return 121;
  if (w < 26006) return 69;
  if (w < 26130) return 122;
  if (w < 26254) return 69;
  if (w < 26378) return 123;
  if (w < 26503) return 69;
  if (w < 26627) return 124;
  if (w < 26751) return 69;
  if (w < 26876) return 125;
  if (w < 27000) return 69;
  if (w < 27124) return 126;
  if (w < 27248) return 69;
  if (w < 27373) return 127;
  if (w < 27497) return 69;
  if (w < 27621) return 128;
  if (w < 27746) return 69;
  if (w < 27870) return 129;
  if (w < 27994) return 69;
  if (w < 28118) return 130;
  if (w < 28243) return 69;
  if (w < 28367) return 131;
  if (w < 28491) return 69;
  if (w < 28615) return 132;
  if (w < 28740) return 69;
  if (w < 28864) return 133;
  if (w < 28988) return 69;
  if (w < 29113) return 134;
  if (w < 29237) return 69;
  if (w < 29361) return 135;
  if (w < 29485) return 69;
  if (w < 29610) return 136;
  if (w < 29734) return 69;
  if (w < 29858) return 137;
  if (w < 29983) return 69;
  if (w < 30107) return 138;
  if (w < 30231) return 69;
  if (w < 30355) return 139;
  if (w < 30480) return 69;
  if (w < 30604) return 140;
  if (w < 30728) return 69;
  if (w < 30853) return 141;
  if (w < 30977) return 69;
  if (w < 31101) return 142;
  if (w < 31225) return 69;
  if (w < 31350) return 143;
  if (w < 31474) return 69;
  if (w < 31598) return 144;
  if (w < 31723) return 69;
  if (w < 31847) return 145;
  if (w < 31971) return 69;
  if (w < 32095) return 146;
  if (w < 32220) return 69;
  if (w < 32344) return 147;
  if (w < 32468) return 69;
  if (w < 32592) return 148;
  if (w < 32717) return 69;
  if (w < 32841) return 149;
  if (w < 32965) return 69;
  if (w < 33090) return 150;
  if (w < 33214) return 69;
  if (w < 33338) return 151;
  if (w < 33462) return 69;
  if (w < 33587) return 152;
  if (w < 33711) return 69;
  if (w < 33835) return 153;
  if (w < 33960) return 69;
  if (w < 34084) return 154;
  if (w < 34208) return 69;
  if (w < 34332) return 155;
  if (w < 34457) return 69;
  if (w < 34581) return 156;
  if (w < 34705) return 69;
  if (w < 34830) return 157;
  if (w < 34954) return 69;
  if (w < 35078) return 158;
  if (w < 35202) return 69;
  if (w < 35327) return 159;
  if (w < 35451) return 69;
  if (w < 35575) return 160;
  if (w < 35700) return 69;
  if (w < 35824) return 161;
  if (w < 35948) return 69;
  if (w < 36072) return 162;
  if (w < 36197) return 69;
  if (w < 36321) return 163;
  if (w < 36445) return 69;
  if (w < 36570) return 164;
  if (w < 36694) return 69;
  if (w < 36818) return 165;
  if (w < 36942) return 69;
  if (w < 37067) return 166;
  if (w < 37191) return 69;
  if (w < 37315) return 167;
  if (w < 37439) return 69;
  if (w < 37564) return 168;
  if (w < 37688) return 69;
  if (w < 37812) return 169;
  if (w < 37937) return 69;
  if (w < 38061) return 170;
  if (w < 38185) return 69;
  if (w < 38309) return 171;
  if (w < 38434) return 69;
  if (w < 38558) return 172;
  if (w < 38682) return 69;
  if (w < 38807) return 173;
  if (w < 38931) return 69;
  if (w < 39055) return 174;
  if (w < 39179) return 69;
  if (w < 39304) return 175;
  if (w < 39428) return 69;
  if (w < 39552) return 176;
  if (w < 39677) return 69;
  if (w < 39801) return 177;
  if (w < 39925) return 69;
  if (w < 40049) return 178;
  if (w < 40174) return 69;
  if (w < 40298) return 179;
  if (w < 40422) return 69;
  if (w < 40547) return 180;
  if (w < 40671) return 69;
  if (w < 40795) return 181;
  if (w < 40919) return 69;
  return 182;
}
#endif



// Calculate the Unsatisfied Parity Checks (UPCs) and update the errors
// vector (e) accordingly. In addition, update the black and gray errors vector
// with the relevant values.
_INLINE_ void find_err1(OUT e_t *e,
                        OUT e_t *black_e,
                        OUT e_t *gray_e,
                        IN const syndrome_t *          syndrome,
                        IN const compressed_idx_d_ar_t wlist,
                        IN const uint8_t               threshold,
                        IN const decode_ctx *ctx)
{
  // This function uses the bit-slice-adder methodology of [5]:
  DEFER_CLEANUP(syndrome_t rotated_syndrome = {0}, syndrome_cleanup);
  DEFER_CLEANUP(upc_t upc, upc_cleanup);

  for(uint32_t i = 0; i < N0; i++) {
    // UPC must start from zero at every iteration
    bike_memset(&upc, 0, sizeof(upc));

    // 1) Right-rotate the syndrome for every secret key set bit index
    //    Then slice-add it to the UPC array.
    for(size_t j = 0; j < D; j++) {
      ctx->rotate_right(&rotated_syndrome, syndrome, wlist[i].val[j]);
      ctx->bit_sliced_adder(&upc, &rotated_syndrome, LOG2_MSB(j + 1));
    }

    // 2) Subtract the threshold from the UPC counters
    ctx->bit_slice_full_subtract(&upc, threshold);

    // 3) Update the errors and the black errors vectors.
    //    The last slice of the UPC array holds the MSB of the accumulated values
    //    minus the threshold. Every zero bit indicates a potential error bit.
    //    The errors values are stored in the black array and xored with the
    //    errors Of the previous iteration.
    const r_t *last_slice = &(upc.slice[SLICES - 1].u.r.val);
    for(size_t j = 0; j < R_BYTES; j++) {
      const uint8_t sum_msb  = (~last_slice->raw[j]);
      black_e->val[i].raw[j] = sum_msb;
      e->val[i].raw[j] ^= sum_msb;
    }

    // Ensure that the padding bits (upper bits of the last byte) are zero so
    // they will not be included in the multiplication and in the hash function.
    e->val[i].raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;

    // 4) Calculate the gray error array by adding "DELTA" to the UPC array.
    //    For that we reuse the rotated_syndrome variable setting it to all "1".
    for(size_t l = 0; l < DELTA; l++) {
      bike_memset((uint8_t *)rotated_syndrome.qw, 0xff, R_BYTES);
      ctx->bit_sliced_adder(&upc, &rotated_syndrome, SLICES);
    }

    // 5) Update the gray list with the relevant bits that are not
    //    set in the black list.
    for(size_t j = 0; j < R_BYTES; j++) {
      const uint8_t sum_msb = (~last_slice->raw[j]);
      gray_e->val[i].raw[j] = (~(black_e->val[i].raw[j])) & sum_msb;
    }
  }
}

// Recalculate the UPCs and update the errors vector (e) according to it
// and to the black/gray vectors.
_INLINE_ void find_err2(OUT e_t *e,
                        IN e_t * pos_e,
                        IN const syndrome_t *          syndrome,
                        IN const compressed_idx_d_ar_t wlist,
                        IN const uint8_t               threshold,
                        IN const decode_ctx *ctx)
{
  DEFER_CLEANUP(syndrome_t rotated_syndrome = {0}, syndrome_cleanup);
  DEFER_CLEANUP(upc_t upc, upc_cleanup);

  for(uint32_t i = 0; i < N0; i++) {
    // UPC must start from zero at every iteration
    bike_memset(&upc, 0, sizeof(upc));

    // 1) Right-rotate the syndrome, for every index of a set bit in the secret
    // key. Then slice-add it to the UPC array.
    for(size_t j = 0; j < D; j++) {
      ctx->rotate_right(&rotated_syndrome, syndrome, wlist[i].val[j]);
      ctx->bit_sliced_adder(&upc, &rotated_syndrome, LOG2_MSB(j + 1));
    }

    // 2) Subtract the threshold from the UPC counters
    ctx->bit_slice_full_subtract(&upc, threshold);

    // 3) Update the errors vector.
    //    The last slice of the UPC array holds the MSB of the accumulated values
    //    minus the threshold. Every zero bit indicates a potential error bit.
    const r_t *last_slice = &(upc.slice[SLICES - 1].u.r.val);
    for(size_t j = 0; j < R_BYTES; j++) {
      const uint8_t sum_msb = (~last_slice->raw[j]);
      e->val[i].raw[j] ^= (pos_e->val[i].raw[j] & sum_msb);
    }

    // Ensure that the padding bits (upper bits of the last byte) are zero, so
    // they are not included in the multiplication, and in the hash function.
    e->val[i].raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;
  }
}

#include <stdio.h>

void decode(OUT e_t *e, IN const ct_t *ct, IN const sk_t *sk)
{
  FIA_CURRENT_BGF_ITERATION = 1;
  // Initialize the decode methods struct
  decode_ctx ctx;
  decode_ctx_init(&ctx);

  DEFER_CLEANUP(e_t black_e = {0}, e_cleanup);
  DEFER_CLEANUP(e_t gray_e = {0}, e_cleanup);

  DEFER_CLEANUP(pad_r_t c0 = {0}, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t h0 = {0}, pad_r_cleanup);
  pad_r_t pk = {0};

  // Pad ciphertext (c0), secret key (h0), and public key (h)
  c0.val = ct->c0;
  h0.val = sk->bin[0];
  pk.val = sk->pk;

  DEFER_CLEANUP(syndrome_t s = {0}, syndrome_cleanup);
  DMSG("  Computing s.\n");
  compute_syndrome(&s, &c0, &h0, &ctx);
  ctx.dup(&s);

  // Reset (init) the error because it is xored in the find_err functions.
  bike_memset(e, 0, sizeof(*e));

  for(uint32_t iter = 0; iter < MAX_IT; iter++) {
    const uint8_t threshold = get_threshold(&s);
    if (FIA_CURRENT_BGF_ITERATION == FIA_FAULT_ON_BGF_ITERATION) {
      // int synd_weight = r_bits_vector_weight((r_t *)s.qw);
      global_synd_weight = r_bits_vector_weight((r_t *)s.qw);
      // printf("%d,", synd_weight);
    }
    FIA_CURRENT_BGF_ITERATION++;

    DMSG("    Iteration: %d\n", iter);
    DMSG("    Weight of e: %lu\n",
         r_bits_vector_weight(&e->val[0]) + r_bits_vector_weight(&e->val[1]));
    DMSG("    Weight of syndrome: %lu\n", r_bits_vector_weight((r_t *)s.qw));

    find_err1(e, &black_e, &gray_e, &s, sk->wlist, threshold, &ctx);
    recompute_syndrome(&s, &c0, &h0, &pk, e, &ctx);



#if defined(BGF_DECODER)
    if(iter >= 1) {
      continue;
    }
#endif
    DMSG("    Weight of e: %lu\n",
         r_bits_vector_weight(&e->val[0]) + r_bits_vector_weight(&e->val[1]));
    DMSG("    Weight of syndrome: %lu\n", r_bits_vector_weight((r_t *)s.qw));

    find_err2(e, &black_e, &s, sk->wlist, ((D + 1) / 2) + 1, &ctx);
    recompute_syndrome(&s, &c0, &h0, &pk, e, &ctx);

    DMSG("    Weight of e: %lu\n",
         r_bits_vector_weight(&e->val[0]) + r_bits_vector_weight(&e->val[1]));
    DMSG("    Weight of syndrome: %lu\n", r_bits_vector_weight((r_t *)s.qw));

    find_err2(e, &gray_e, &s, sk->wlist, ((D + 1) / 2) + 1, &ctx);
    recompute_syndrome(&s, &c0, &h0, &pk, e, &ctx);
  }

  FIA_CURRENT_BGF_ITERATION = -1;
}
