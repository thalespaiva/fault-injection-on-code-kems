#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include "api.h"
#include "parameters.h"
#include "symmetric.h"

#include "gf2x.h"
#include "vector.h"


#define IN
#define OUT

// #define CHECKPOINT_STEP 500000
// #define CHECKPOINT_STEP 100000
#define CHECKPOINT_STEP 50000

#define R_SPECTRUM_SIZE (PARAM_N / 2) + 1

// The spectrum of `r`-bit vectors
typedef struct r_spectrum_s {
  uint32_t multiplicities[R_SPECTRUM_SIZE];
} r_spectrum_t;

// static void vect_print(const uint64_t *v, const uint32_t size);
void print_nonnull_positions(IN const uint64_t *in, IN const uint32_t bits_num);
int get_distance(int pos1, int pos2, int length);
void get_r_spectrum(OUT r_spectrum_t *spectrum, IN const uint64_t *in);


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

    if (argc != 2) {
        printf("Usage: %s <n_decaps>\n", argv[0]);
        exit(1);
    }

    long n_decaps = atol(argv[1]);
    // long n_errors = 0;

    unsigned char pk[PUBLIC_KEY_BYTES];
    unsigned char sk[SECRET_KEY_BYTES];
    unsigned char ct[CIPHERTEXT_BYTES];
    unsigned char ss1[SHARED_SECRET_BYTES];
    unsigned char ss2[SHARED_SECRET_BYTES];

    unsigned char seed[48] = {0};
    // syscall(SYS_getrandom, seed, 48, 0);
    for (int i = 0; i < 48; i++) {
        seed[i] = i;
    }
    prng_init(seed, NULL, 48, 0);

    crypto_kem_keypair(pk, sk);

    r_spectrum_t spectrum_x = {0};
    get_r_spectrum(&spectrum_x, global_x);

    r_spectrum_t spectrum_y = {0};
    get_r_spectrum(&spectrum_y, global_y);

    printf("noise_hw,failed\n");


    for (long i = 1; i <= n_decaps; i++) {
        crypto_kem_enc(ct, ss1, pk);

        // IMPORTANT: this should be made before decryption, otherwise `crypto_kem_dec` can
        // overwrite `global_r1,r2` with an invalid value.
        uint64_t t1[VEC_N_SIZE_64] = {0};
        uint64_t t2[VEC_N_SIZE_64] = {0};

        vect_mul(t1, global_y, global_r1);
        vect_mul(t2, global_x, global_r2);
        vect_add(t1, t1, t2, VEC_N_SIZE_64);
        vect_add(t1, t1, global_e, VEC_N_SIZE_64);

        crypto_kem_dec(ss2, ct, sk);
        // Decapsulate
        // dec_rc = crypto_kem_dec(k_dec.val, ct.val, sk.val);


        int hw = 0;
        for (size_t i = 0; i < VEC_N_SIZE_64; i++) {
            hw += __builtin_popcountll(t1[i]);
        }

        // Check test status
        int decaps_failure = 0;
        if (memcmp(ss1, ss2, SHARED_SECRET_BYTES)) {
            decaps_failure = 1;
        }
        printf("%d,%d\n", hw, decaps_failure);
        fflush(stdout);
    }

    return 0;
}

