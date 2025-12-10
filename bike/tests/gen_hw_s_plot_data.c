/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0"
 *
 * Written by Nir Drucker, Shay Gueron and Dusan Kostic,
 * AWS Cryptographic Algorithms Group.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gf2x.h"
#include "kem.h"
#include "measurements.h"
#include "utilities.h"
#include "cpu_features.h"
#include "utilities.h"

// BEGIN ATTACKER VARIABLES -- VALUES KNOWN TO THE FIA ATTACKER ===================================

pad_e_t global_e = {0};
int FIA_FAULT_ON_BGF_ITERATION = 2;
// END ATTACKER VARIABLES =========================================================================


// BEGIN SECRET VARIABLES -- NOT KNOWN TO THE FIA ATTACKER ========================================

pad_r_t global_h0 = {0};
pad_r_t global_h1 = {0};

int global_synd_weight = 0;

// END SECRET VARIABLES ===========================================================================


typedef struct magic_number_s {
  uint64_t val[4];
} magic_number_t;

#define STRUCT_WITH_MAGIC(name, size) \
  struct {                            \
    magic_number_t magic1;            \
    uint8_t        val[size];         \
    magic_number_t magic2;            \
  }(name) = {magic, {0}, magic};

#define CHECK_MAGIC(param)                                          \
  if((0 != memcmp((param).magic1.val, magic.val, sizeof(magic))) || \
     (0 != memcmp((param).magic2.val, magic.val, sizeof(magic)))) { \
    printf("Magic is incorrect for param\n");                       \
  }


#define R_SPECTRUM_SIZE (R_BITS / 2) + 1

// The spectrum of `r`-bit vectors
typedef struct r_spectrum_s {
  uint32_t multiplicities[R_SPECTRUM_SIZE];
} r_spectrum_t;


void print_nonnull_positions(IN const uint64_t *in, IN const uint32_t bits_num) {
  for (uint32_t i = 0; i < bits_num; i++) {
    if ((in[i / 64] >> (i % 64)) & 1) {
      printf("%d, ", i);
    }
  }
  printf("\n");
}

// Circular distance between `pos1` and `pos2` in a (circular) vector of size `length`.
int get_distance(int pos1, int pos2, int length) {
    int dist1, dist2;

    dist1 = abs(pos2 - pos1);
    dist2 = length - dist1;
    if (dist1 <= dist2)
        return dist1;
    return dist2;
}

// Computes the spectrum of an `r`-bit vector.
// The `bits_num` parameter should be always equals to `R_BITS` -- this parameter should be removed
// in the near future, it was here just for previous compatibility that is not needed.
void get_r_spectrum(OUT r_spectrum_t *spectrum, IN const uint64_t *in, IN const uint32_t bits_num) {
  assert (bits_num == R_BITS);
  uint32_t support[R_BITS] = {0};

  uint32_t n_support = 0;
  for (uint32_t i = 0; i < bits_num; i++) {
    if ((in[i / 64] >> (i % 64)) & 1) {
      support[n_support++] = i;
    }
  }

  bike_memset(spectrum->multiplicities, 0, R_SPECTRUM_SIZE);
  for (uint32_t i = 0; i < n_support; i++) {
    for (uint32_t j = i + 1; j < n_support; j++) {
      uint32_t distance = get_distance(support[i], support[j], R_BITS);
      spectrum->multiplicities[distance] += 1;
    }
  }
}

#define CHECKPOINT_STEP 500000

int main(int argc, char *argv[])
{

  if (argc != 3) {
    printf("Usage: %s <seed> <n_decaps>\n", argv[0]);
    exit(1);
  }

  int seed = atoi(argv[1]);
  size_t n_decaps = atoi(argv[2]);

  // Initialize the CPU features flags
  cpu_features_init();

  srand(seed);

  magic_number_t magic = {0xa1234567b1234567, 0xc1234567d1234567,
                          0xe1234567f1234567, 0x0123456711234567};

  STRUCT_WITH_MAGIC(sk, sizeof(sk_t));
  STRUCT_WITH_MAGIC(pk, sizeof(pk_t));
  STRUCT_WITH_MAGIC(ct, sizeof(ct_t));
  STRUCT_WITH_MAGIC(k_enc, sizeof(ss_t)); // shared secret after decapsulate
  STRUCT_WITH_MAGIC(k_dec, sizeof(ss_t)); // shared secret after encapsulate

  // Key generation
  assert(crypto_kem_keypair(pk.val, sk.val) == 0);

  printf("i,bgf_faulted_iteration,global_synd_weight,failed\n");

  for (FIA_FAULT_ON_BGF_ITERATION = 1; FIA_FAULT_ON_BGF_ITERATION <= 5; FIA_FAULT_ON_BGF_ITERATION++) {
    for(size_t i = 1; i <= n_decaps; ++i) {

      uint32_t dec_rc = 0;

      // Encapsulate
      assert(crypto_kem_enc(ct.val, k_enc.val, pk.val) == 0);

      // IMPORTANT: this should be made before decryption, otherwise `crypto_kem_dec` can
      // overwrite `global_e` with an invalid value (in case of decoding error), during the
      // reencryption procedure
      r_spectrum_t spectrum_e0 = {0};
      get_r_spectrum(&spectrum_e0, (const uint64_t *)PE0_RAW(&global_e), R_BITS);

      r_spectrum_t spectrum_e1 = {0};
      get_r_spectrum(&spectrum_e1, (const uint64_t *)PE1_RAW(&global_e), R_BITS);

      // Decapsulate
      dec_rc = crypto_kem_dec(k_dec.val, ct.val, sk.val);

      // Check test status
      int decaps_failure = 0;
      if(dec_rc != 0) {
        decaps_failure = 1;
      } else {
        if(secure_cmp(k_enc.val, k_dec.val, sizeof(k_dec.val) / sizeof(uint64_t))) {
          decaps_failure = 0;
        } else {
          decaps_failure = 1;
        }
      }

      printf("%ld,%d,%d,%d\n", i, FIA_FAULT_ON_BGF_ITERATION, global_synd_weight, decaps_failure);

      // Check magic numbers (memory overflow)
      CHECK_MAGIC(sk);
      CHECK_MAGIC(pk);
      CHECK_MAGIC(ct);
      CHECK_MAGIC(k_enc);
      CHECK_MAGIC(k_dec);
    }
  }


  return 0;
}
