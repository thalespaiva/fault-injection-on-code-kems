#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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
void print_sparse_vector(uint64_t in[]);

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

void print_sparse_vector(uint64_t in[]) {
  uint32_t support[PARAM_N] = {0};

  int n_support = 0;
  for (int i = 0; i < PARAM_N; i++) {
    if ((in[i / 64] >> (i % 64)) & 1) {
      support[n_support++] = i;
    }
  }
  printf("\"[");
  for (int i = 0; i < n_support; i++) {
    printf("%d", support[i]);
    if (i < n_support - 1) printf(",");
  }
  printf("]\"");
}

extern int FIA_VECT_MUL_SCHOOLBOOK_TARGET_SECRET_COEFFICIENT;

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <seed>\n", argv[0]);
        exit(1);
    }

    int base_seed = atoi(argv[1]);


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
    // syscall(SYS_getrandom, seed, 48, 0);
    prng_init(seed, NULL, 48, 0);

    crypto_kem_keypair(pk, sk);

    uint64_t h[VEC_N_SIZE_64] = {0};
    uint64_t s[VEC_N_SIZE_64] = {0};
    hqc_ek_pke_from_string(h, s, pk);


    printf("x,y,h,s\n");
    print_sparse_vector(global_x);
    printf(",");
    print_sparse_vector(global_y);
    printf(",");
    print_sparse_vector(h);
    printf(",");
    print_sparse_vector(s);
    printf("\n");

    int n_guess = 0;
    int guess[PARAM_N] = {0};
    memset(guess, -1, PARAM_N*sizeof(*guess));

    int last_decaps = -1;
    printf("position,failure_rate\n");
    for (long i = 0; i < PARAM_N; i++) {
        crypto_kem_enc(ct, ss1, pk);

        FIA_VECT_MUL_SCHOOLBOOK_TARGET_SECRET_COEFFICIENT = i;
        crypto_kem_dec(ss2, ct, sk);

        // Check test status
        int decaps_failure = 0;
        if (memcmp(ss1, ss2, SHARED_SECRET_BYTES)) {
            decaps_failure = 1;
        }
        if ((last_decaps == 1) && (decaps_failure == 0)) {
            guess[n_guess++] = i;
            fprintf(stderr, "Adding %ld to guess\n", i);
        }
        last_decaps = decaps_failure;
        printf("%ld,%d\n", i, decaps_failure);
        fflush(stdout);
    }
}

