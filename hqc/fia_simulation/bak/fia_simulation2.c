#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "parsing.h"
#include "hqc.h"
#include "vector.h"
#include "gf2x.h"
#include "code.h"
#include "api.h"
#include "parameters.h"
#include "code.h"

#include "sca_utils.h"

#define MAX_CHALLENGES 100000

void sca_shuffled_expand_and_sum_oracle(int32_t c_prime_block_weight_diff[PARAM_N1],
                                        double error_probability_eps,
                                        uint64_t *m,
                                        const __m256i *u_256,
                                        const uint64_t *v, const uint8_t *sk) {
    __m256i x_256[VEC_N_256_SIZE_64 >> 2] = {0};
    __m256i y_256[VEC_N_256_SIZE_64 >> 2] = {0};
    uint8_t pk[PUBLIC_KEY_BYTES] = {0};
    uint64_t tmp1[VEC_N_256_SIZE_64] = {0};
    uint64_t tmp2[VEC_N_256_SIZE_64] = {0};
    __m256i tmp3_256[VEC_N_256_SIZE_64 >> 2];

    #ifdef __STDC_LIB_EXT1__
        memset_s(y_256, 0, (VEC_N_256_SIZE_64 >> 2) * sizeof(__m256i));
    #else
        memset(y_256, 0, (VEC_N_256_SIZE_64 >> 2) * sizeof(__m256i));
    #endif

    // Retrieve x, y, pk from secret key
    hqc_secret_key_from_string(x_256, y_256, pk, sk);

    // printf("\n\ny: "); print_sparse_vector((uint64_t *) y_256);
    // printf("\nx: "); print_sparse_vector((uint64_t *) x_256);

    // Compute v - u.y
    vect_resize(tmp1, PARAM_N, v, PARAM_N1N2);
    vect_mul(tmp3_256, y_256, u_256);
    vect_add(tmp2, tmp1, (uint64_t *) tmp3_256, VEC_N_256_SIZE_64);
    // Now tmp2 = v - u.y = mG + e'

    uint64_t encoded_message_n1n2[VEC_N1N2_256_SIZE_64] = {0};
    code_encode(encoded_message_n1n2, m);

    codeword *cw_array = (codeword *) tmp2;
    codeword *em_array = (codeword *) encoded_message_n1n2;

    for (int i = 0; i < PARAM_N1; i++) {
        c_prime_block_weight_diff[i] = 0;

        // This loop represents what we can recover with side-channel information
        int weight_c_prime_block = 0;
        for (int k = 0; k < MULTIPLICITY; k++) {
            for (int j = 0; j < 4; j++) {
                uint32_t b = cw_array[i * MULTIPLICITY + k].u32[j];
                weight_c_prime_block +=  __builtin_popcount(b);
            }
        }

        // Of the `PARAM_N2` bits, `weight_c_prime_block` of them are 1s and
        // `PARAM_N2 - weight_c_prime_block` are 0s.
        // We then apply the bit error first on the 1s, converting them to 0s, and
        // then on 0s, converting them to ones (that is, decreasing or increasing the final weight).

        int n_errors_1s_to_0s = 0;
        for (int k = 0; k < weight_c_prime_block; k++) {
            n_errors_1s_to_0s += get_random_bit(error_probability_eps);
        }

        int n_errors_0s_to_1s = 0;
        for (int k = 0; k < PARAM_N2 - weight_c_prime_block; k++) {
            n_errors_0s_to_1s += get_random_bit(error_probability_eps);
        }
        weight_c_prime_block += n_errors_0s_to_1s - n_errors_1s_to_0s;

        // This loop can be computed by anyone who knows the message
        int weight_em_block = 0;
        for (int k = 0; k < MULTIPLICITY; k++) {
            for (int j = 0; j < 4; j++) {
                uint32_t a = em_array[i * MULTIPLICITY + k].u32[j];
                weight_em_block +=  __builtin_popcount(a);
            }
        }

        c_prime_block_weight_diff[i] = weight_c_prime_block - weight_em_block;
    }

}

void update_likelihoods(uint32_t likelihoods[PARAM_N],
                        uint64_t r[VEC_N_256_SIZE_64],
                        uint64_t m[VEC_K_SIZE_64],
                        int32_t c_prime_block_weight_diff[PARAM_N1]) {
    size_t r_support[PARAM_OMEGA_R] = {0};
    size_t n_r_support = 0;

    for (int i = 0; i < PARAM_N; i++) {
        if ((r[i / 64] >> (i % 64)) & 1) {
            r_support[n_r_support++] = i;
        }
    }
    assert(n_r_support == PARAM_OMEGA_R);

    uint64_t encoded_message_n1n2[VEC_N1N2_256_SIZE_64] = {0};
    code_encode(encoded_message_n1n2, m);


    for (int shift = 0; shift < PARAM_N; shift++) {
        for (int i = 0; i < PARAM_OMEGA_R; i++) {
            int r_shift = (r_support[i] + shift) % PARAM_N;

            // `r_shift` iterates over `supp(r >> shift)`

            // In this case, r_shift is in the padding, so no there is no corresponding RM block
            if (r_shift >= PARAM_N1N2) {
                continue;
            }

            // Gets the difference from the block corresponding to `r_shift`
            int diff = c_prime_block_weight_diff[r_shift / PARAM_N2];

            // The bit of the encoded message corresponding to `r_shift`
            // int em_bit = (encoded_message_n1n2[r_shift / 64] >> (r_shift % 64)) & 1;

            int em_bit = (encoded_message_n1n2[r_shift / 64] >> (r_shift % 64)) & 1;

            if (diff > 0) {
                likelihoods[shift] += (em_bit == 0);
            }
            else {
                likelihoods[shift] += (em_bit != 0);
            }
        }
    }
}

size_t get_minimum_threshold_in_support(uint64_t *secret_vector,
                                     uint32_t likelihoods[PARAM_N]) {
    size_t min_thresh = -1;
    for (int i = 0; i < PARAM_N; i++) {
        if ((secret_vector[i / 64] >> (i % 64)) & 1) {
            min_thresh = likelihoods[i] < min_thresh ? likelihoods[i] : min_thresh;
        }
    }
    return min_thresh;
}

size_t get_minimum_threshold_for_both_x_and_y(uint64_t *x, uint32_t likelihoods_x[PARAM_N],
                                              uint64_t *y, uint32_t likelihoods_y[PARAM_N]) {
    size_t min_thresh_x = get_minimum_threshold_in_support(x, likelihoods_x);
    size_t min_thresh_y = get_minimum_threshold_in_support(y, likelihoods_y);
    // printf("min_thresh_x = %d\n", min_thresh_x);
    // printf("min_thresh_y = %d\n", min_thresh_y);
    size_t min_thresh = min_thresh_x < min_thresh_y ? min_thresh_x : min_thresh_y;

    return min_thresh;
}

void get_unknown2_and_zeros1_for_same_threshold(uint32_t unknown2[PARAM_N], size_t *n_unknown2,
                                                uint32_t zeros1[PARAM_N], size_t *n_zeros1,
                                                uint32_t likelihoods_x[PARAM_N],
                                                uint32_t likelihoods_y[PARAM_N],
                                                const unsigned char *sk) {
    __m256i x_256[VEC_N_256_SIZE_64 >> 2] = {0};
    __m256i y_256[VEC_N_256_SIZE_64 >> 2] = {0};
    // Retrieve x, y, pk from secret key
    unsigned char pk_discard[PUBLIC_KEY_BYTES];

    hqc_secret_key_from_string(x_256, y_256, pk_discard, sk);

    size_t min_thresh = get_minimum_threshold_for_both_x_and_y((uint64_t *) x_256,
                                                               likelihoods_x,
                                                               (uint64_t *) y_256,
                                                               likelihoods_y);

    *n_unknown2 = 0;
    *n_zeros1 = 0;
    for (int i = 0; i < PARAM_N; i++) {

        if (likelihoods_y[i] >= min_thresh) {
            unknown2[*n_unknown2] = i;
            *n_unknown2 += 1;
        }
        if (likelihoods_x[i] < min_thresh) {
            zeros1[*n_zeros1] = i;
            *n_zeros1 += 1;
        }
    }
}

void sca_expand_and_sum_attack(const unsigned char *pk, const unsigned char *sk, double error_probability_eps) {

    uint32_t likelihoods_y[PARAM_N] = {0};
    uint32_t likelihoods_x[PARAM_N] = {0};

    int success = 0;
    size_t n_challenges = 0;
    size_t n_unknown2 = 0;
    size_t n_zeros1 = 0;
    for (n_challenges = 1; n_challenges < MAX_CHALLENGES; n_challenges++) {
        unsigned char ct[CIPHERTEXT_BYTES] = {0};
        uint64_t m[VEC_K_SIZE_64] = {0};
        __m256i r1_256[VEC_N_256_SIZE_64 >> 2] = {0};
        __m256i r2_256[VEC_N_256_SIZE_64 >> 2] = {0};
        sca_hqc_kem_enc(ct, r1_256, r2_256, m, pk);

        __m256i u_256[VEC_N_256_SIZE_64 >> 2] = {0};
        uint64_t v[VEC_N1N2_256_SIZE_64] = {0};
        uint8_t d[SHAKE256_512_BYTES] = {0};

        // Retrieving u, v and d from ciphertext
        hqc_ciphertext_from_string((uint64_t *) u_256, v , d, ct);

        int32_t c_prime_block_weight_diff[PARAM_N1] = {0};
        sca_shuffled_expand_and_sum_oracle(c_prime_block_weight_diff, error_probability_eps, m, u_256, v, sk);


        update_likelihoods(likelihoods_y, (uint64_t *) r1_256, m, c_prime_block_weight_diff);
        update_likelihoods(likelihoods_x, (uint64_t *) r2_256, m, c_prime_block_weight_diff);

        uint32_t unknown2[PARAM_N] = {0};
        uint32_t zeros1[PARAM_N] = {0};

        get_unknown2_and_zeros1_for_same_threshold(unknown2, &n_unknown2, zeros1, &n_zeros1,
                                                   likelihoods_x, likelihoods_y, sk);

        int is_key_equation_full_rank = solve_key_equation_matrix_if_full_rank(pk, unknown2, n_unknown2,
                                                                               zeros1, n_zeros1, sk);
        if (is_key_equation_full_rank) {
            success = 1;
            break;
        }
    }

    printf("%lf,", error_probability_eps);
    if (success) {
        printf("%lu,", n_challenges);
    }
    else {
        printf("NA,");
    }
    printf("%lu,", n_unknown2);
    printf("%lu", n_zeros1);
    printf("\n");
    fflush(stdout);

}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <n_samples> <eps_min> <eps_step> <eps_max>\n", argv[0]);
        exit(1);
    }

    printf("\n");
    printf("*********************\n");
    printf("**** HQC-%d-%d ****\n", PARAM_SECURITY, PARAM_DFR_EXP);
    printf("*********************\n");

    printf("\n");
    printf("N: %d   ", PARAM_N);
    printf("N1: %d   ", PARAM_N1);
    printf("N2: %d   ", PARAM_N2);
    printf("OMEGA: %d   ", PARAM_OMEGA);
    printf("OMEGA_R: %d   ", PARAM_OMEGA_R);
    printf("Failure rate: 2^-%d   ", PARAM_DFR_EXP);
    printf("Sec: %d bits", PARAM_SECURITY);
    printf("\n");

    unsigned char pk[PUBLIC_KEY_BYTES];
    unsigned char sk[SECRET_KEY_BYTES];
    int n_samples = atoi(argv[1]);
    double error_probability_eps_min = atof(argv[2]);
    double error_probability_eps_step = atof(argv[3]);
    double error_probability_eps_max = atof(argv[4]);

    printf("error_probability_eps,n_challenges,n_unknown2,n_zeros1\n");
    for (double eps = error_probability_eps_min; eps <= error_probability_eps_max + EPSILON; eps += error_probability_eps_step) {
        for (int i = 0; i < n_samples; i++) {
            crypto_kem_keypair(pk, sk);
            sca_expand_and_sum_attack(pk, sk, eps);
        }
    }

    return 0;
}