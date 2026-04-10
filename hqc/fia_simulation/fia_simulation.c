#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include "api.h"
#include "hqc.h"
#include "symmetric.h"

#if HQC_ARCH_X86_64_AVX256 == 1

#include "x86_64/common/hqc-1/parameters.h"

#define VEC_N_256_NUM_WORDS (VEC_N_256_SIZE_64 >> 2)

// BEGIN SECRET VARIABLES -- NOT KNOWN TO THE FIA ATTACKER ========================================
uint64_t global_x[VEC_N_256_NUM_WORDS << 2] = {0};
uint64_t global_y[VEC_N_256_NUM_WORDS << 2] = {0};
uint64_t global_noise_hw[VEC_N_256_NUM_WORDS << 2] = {0};
// END SECRET VARIABLES ===========================================================================


// BEGIN ATTACKER VARIABLES -- VALUES KNOWN TO THE FIA ATTACKER ===================================
uint64_t global_r1[VEC_N_256_NUM_WORDS << 2] = {0};
uint64_t global_r2[VEC_N_256_NUM_WORDS << 2] = {0};
uint64_t global_e[VEC_N_256_NUM_WORDS << 2] = {0};
// END ATTACKER VARIABLES =========================================================================

#else

#include "parameters.h"

// BEGIN SECRET VARIABLES -- NOT KNOWN TO THE FIA ATTACKER ========================================
uint64_t global_x[VEC_N_SIZE_64] = {0};
uint64_t global_y[VEC_N_SIZE_64] = {0};
uint64_t global_noise_hw[VEC_N_SIZE_64] = {0};
// END SECRET VARIABLES ===========================================================================


// BEGIN ATTACKER VARIABLES -- VALUES KNOWN TO THE FIA ATTACKER ===================================
uint64_t global_r1[VEC_N_SIZE_64] = {0};
uint64_t global_r2[VEC_N_SIZE_64] = {0};
uint64_t global_e[VEC_N_SIZE_64] = {0};
// END ATTACKER VARIABLES =========================================================================

#endif

#define IN
#define OUT

#define CHECKPOINT_STEP 1000000

#define R_SPECTRUM_SIZE (PARAM_N / 2) + 1

// The spectrum of `r`-bit vectors
typedef struct r_spectrum_s {
  uint32_t multiplicities[R_SPECTRUM_SIZE];
} r_spectrum_t;

// static void vect_print(const uint64_t *v, const uint32_t size);
void print_nonnull_positions(IN const uint64_t *in, IN const uint32_t bits_num);
int get_distance(int pos1, int pos2, int length);
void get_r_spectrum(OUT r_spectrum_t *spectrum, IN const uint64_t *in);


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
// The `bits_num` parameter should be always equals to `PARAM_N` -- this parameter should be removed
// in the near future, it was here just for previous compatibility that is not needed.
void get_r_spectrum(OUT r_spectrum_t *spectrum, IN const uint64_t *in) {
  uint32_t support[PARAM_N] = {0};

  uint32_t n_support = 0;
  for (uint32_t i = 0; i < PARAM_N; i++) {
    if ((in[i / 64] >> (i % 64)) & 1) {
      support[n_support++] = i;
    }
  }

  memset(spectrum->multiplicities, 0, R_SPECTRUM_SIZE * sizeof(*spectrum->multiplicities));
  for (uint32_t i = 0; i < n_support; i++) {
    for (uint32_t j = i + 1; j < n_support; j++) {
      uint32_t distance = get_distance(support[i], support[j], PARAM_N);
      spectrum->multiplicities[distance] += 1;
    }
  }
}



int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <seed> <n_decaps>\n", argv[0]);
        exit(1);
    }

    int base_seed = atoi(argv[1]);
    long n_decaps = atol(argv[2]);

    // long n_errors = 0;

    unsigned char pk[PUBLIC_KEY_BYTES];
    unsigned char sk[SECRET_KEY_BYTES];
    unsigned char ct[CIPHERTEXT_BYTES];
    unsigned char ss1[SHARED_SECRET_BYTES];
    unsigned char ss2[SHARED_SECRET_BYTES];

    srand(base_seed);

    unsigned char seed[48] = {0};
    for (int i = 0; i < 48; i++) {
        seed[i] = rand();
    }
    prng_init(seed, NULL, 48, 0);

    crypto_kem_keypair(pk, sk);

    r_spectrum_t spectrum_x = {0};
    get_r_spectrum(&spectrum_x, global_x);

    r_spectrum_t spectrum_y = {0};
    get_r_spectrum(&spectrum_y, global_y);

    // The vectors used to estimate the likelihood that a distance is in the spectrum of `x`
    uint32_t r2_spec_fails[R_SPECTRUM_SIZE] = {0};
    uint32_t r2_spec_total[R_SPECTRUM_SIZE] = {0};

    // The vectors used to estimate the likelihood that a distance is in the spectrum of `y`
    uint32_t r1_spec_fails[R_SPECTRUM_SIZE] = {0};
    uint32_t r1_spec_total[R_SPECTRUM_SIZE] = {0};


    printf("n_challenges,distance,spec_x,spec_r2_estimator,spec_y,spec_r1_estimator\n");


    for (long i = 1; i <= n_decaps; i++) {
        crypto_kem_enc(ct, ss1, pk);

        // IMPORTANT: this should be made before decryption, otherwise `crypto_kem_dec` can
        // overwrite `global_r1,r2` with an invalid value.
        r_spectrum_t spectrum_r2 = {0};
        get_r_spectrum(&spectrum_r2, global_r2);

        r_spectrum_t spectrum_r1 = {0};
        get_r_spectrum(&spectrum_r1, global_r1);

        crypto_kem_dec(ss2, ct, sk);
        // Decapsulate
        // dec_rc = crypto_kem_dec(k_dec.val, ct.val, sk.val);

        // Check test status
        int decaps_failure = 0;
        if (memcmp(ss1, ss2, SHARED_SECRET_BYTES)) {
            decaps_failure = 1;
        }
        // printf("decaps_failure = %d\n", decaps_failure);
        for (int d = 0; d < R_SPECTRUM_SIZE; d++) {
            if (spectrum_r2.multiplicities[d]) {
                r2_spec_fails[d] += decaps_failure * spectrum_r2.multiplicities[d];
                r2_spec_total[d] += spectrum_r2.multiplicities[d];
            }
            if (spectrum_r1.multiplicities[d]) {
                r1_spec_fails[d] += decaps_failure * spectrum_r1.multiplicities[d];
                r1_spec_total[d] += spectrum_r1.multiplicities[d];
            }
        }

        if (i % CHECKPOINT_STEP == 0 || i == n_decaps) {
            // Print key spec and the estimators
            for (int d = 1; d < R_SPECTRUM_SIZE; d++) {
                printf("%ld,", i);
                printf("%d,", d);
                printf("%d,", spectrum_x.multiplicities[d]);
                printf("%.4lf,", (double) r2_spec_fails[d] / r2_spec_total[d]);
                printf("%d,", spectrum_y.multiplicities[d]);
                printf("%.4lf\n", (double) r1_spec_fails[d] / r1_spec_total[d]);
            }
            fflush(stdout);
        }
    }

    return 0;
}

