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

#include "kem.h"
#include "utilities.h"
#include "measurements.h"
#include "defs.h"

// #include "gf2x.h"
// #include "kem.h"
// #include "measurements.h"
// #include "utilities.h"
// #include "cpu_features.h"
// #include "utilities.h"

// BEGIN ATTACKER VARIABLES -- VALUES KNOWN TO THE FIA ATTACKER ===================================

// pad_e_t global_e = {0};
uint8_t global_e0[R_SIZE];
uint8_t global_e1[R_SIZE];

// END ATTACKER VARIABLES =========================================================================


// BEGIN SECRET VARIABLES -- NOT KNOWN TO THE FIA ATTACKER ========================================

// pad_r_t global_h0 = {0};
// pad_r_t global_h1 = {0};

// int global_synd_weight = 0;

// END SECRET VARIABLES ===========================================================================


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
void get_r_spectrum(OUT r_spectrum_t *spectrum, IN const uint8_t *in, IN const uint32_t bits_num) {
  assert (bits_num == R_BITS);
  uint32_t support[R_BITS] = {0};

  uint32_t n_support = 0;
  for (uint32_t i = 0; i < bits_num; i++) {
    if ((in[i / 8] >> (i % 8)) & 1) {
      support[n_support++] = i;
    }
  }

  memset(spectrum->multiplicities, 0, R_SPECTRUM_SIZE * sizeof(*spectrum->multiplicities));
  for (uint32_t i = 0; i < n_support; i++) {
    for (uint32_t j = i + 1; j < n_support; j++) {
      uint32_t distance = get_distance(support[i], support[j], R_BITS);
      spectrum->multiplicities[distance] += 1;
    }
  }
}

#define CHECKPOINT_STEP 50000

double rand01(void) {
    return (double) rand() / RAND_MAX;
}

int main(int argc, char *argv[])
{

  if (argc != 4) {
    printf("Usage: %s <seed> <n_decaps> <encaps_base_seed>\n", argv[0]);
    exit(1);
  }

  int seed = atoi(argv[1]);
  size_t n_decaps = atoi(argv[2]);

  srand(seed);

  sk_t sk    = {0}; // private-key: (h0, h1)
  pk_t pk    = {0}; // public-key:  (g0, g1)
  ct_t ct    = {0}; // ciphertext:  (c0, c1)
  ss_t k_enc = {0}; // shared secret after encapsulate
  ss_t k_dec = {0}; // shared secret after decapsulate

  // Key generation
  // assert(crypto_kem_keypair(pk.val, sk.val) == 0);
  assert(crypto_kem_keypair(pk.raw, sk.raw) == 0);

  r_spectrum_t spectrum_h0 = {0};
  // get_r_spectrum(&spectrum_h0, (const uint64_t *)global_h0.val.raw, R_BITS);
  get_r_spectrum(&spectrum_h0, (const uint8_t *)sk.val0, R_BITS);

  r_spectrum_t spectrum_h1 = {0};
  // get_r_spectrum(&spectrum_h1, (const uint64_t *)global_h1.val.raw, R_BITS);
  get_r_spectrum(&spectrum_h1, (const uint8_t *)sk.val1, R_BITS);

  // The vectors used to estimate the likelihood that a distance is in the spectrum of `h0`
  uint32_t e0_spec_fails[R_SPECTRUM_SIZE] = {0};
  uint32_t e0_spec_total[R_SPECTRUM_SIZE] = {0};

  // The vectors used to estimate the likelihood that a distance is in the spectrum of `h1`
  uint32_t e1_spec_fails[R_SPECTRUM_SIZE] = {0};
  uint32_t e1_spec_total[R_SPECTRUM_SIZE] = {0};


  int encaps_base_seed = atoi(argv[3]);
  srand(encaps_base_seed);

  // fprintf(stderr, "LEVEL = %d\n", LEVEL);
  printf("n_challenges,distance,spec_h0,spec_e0_failure,spec_e0_count,spec_e0_estimator,spec_h1,spec_e1_failure,spec_e1_count,spec_e1_estimator\n");

  for(size_t i = 1; i <= n_decaps; ++i) {

    uint32_t dec_rc = 0;

    // Encapsulate
    // assert(crypto_kem_enc(ct.val, k_enc.val, pk.val) == 0);
    assert(crypto_kem_enc(ct.raw, k_enc.raw, pk.raw) == 0);


    // IMPORTANT: this should be made before decryption, otherwise `crypto_kem_dec` can
    // overwrite `global_e` with an invalid value (in case of decoding error), during the
    // reencryption procedure
    r_spectrum_t spectrum_e0 = {0};
    get_r_spectrum(&spectrum_e0, global_e0, R_BITS);

    r_spectrum_t spectrum_e1 = {0};
    get_r_spectrum(&spectrum_e1, global_e1, R_BITS);

    // Decapsulate
    // dec_rc = crypto_kem_dec(k_dec.val, ct.val, sk.val);
    dec_rc = crypto_kem_dec(k_dec.raw, ct.raw, sk.raw);

    fflush(stdout);

    // Check test status
    int decaps_failure = 0;
    if(dec_rc != 0) {
      decaps_failure = 1;
    }
    decaps_failure |= (safe_cmp(k_enc.raw, k_dec.raw, sizeof(k_dec)/sizeof(uint64_t)) == 0);

    // Update
    for (int d = 0; d < R_SPECTRUM_SIZE; d++) {
      if (spectrum_e0.multiplicities[d]) {
        e0_spec_fails[d] += decaps_failure * spectrum_e0.multiplicities[d];
        e0_spec_total[d] += 1 * spectrum_e0.multiplicities[d];
      }
      if (spectrum_e1.multiplicities[d]) {
        e1_spec_fails[d] += decaps_failure * spectrum_e1.multiplicities[d];
        e1_spec_total[d] += 1 * spectrum_e1.multiplicities[d];
      }
    }

    if ((i % CHECKPOINT_STEP == 0) || (i == n_decaps)){
      // Print key spec and the estimators
      for (int d = 1; d < R_SPECTRUM_SIZE; d++) {
        printf("%ld,", i);
        printf("%d,", d);
        printf("%d,", spectrum_h0.multiplicities[d]);
        printf("%d,", e0_spec_fails[d]);
        printf("%d,", e0_spec_total[d]);
        printf("%.10lf,", (double) e0_spec_fails[d] / e0_spec_total[d]);
        printf("%d,", spectrum_h1.multiplicities[d]);
        printf("%d,", e1_spec_fails[d]);
        printf("%d,", e1_spec_total[d]);
        printf("%.10lf\n", (double) e1_spec_fails[d] / e1_spec_total[d]);
      }
      fflush(stdout);
    }

  }
  return 0;
}
