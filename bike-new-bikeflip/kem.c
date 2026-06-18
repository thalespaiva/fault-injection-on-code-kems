/******************************************************************************
 * BIKE -- Bit Flipping Key Encapsulation
 *
 * Copyright (c) 2021 Nir Drucker, Shay Gueron, Rafael Misoczki, Tobias Oder,
 * Tim Gueneysu, Jan Richter-Brockmann.
 * Contact: drucker.nir@gmail.com, shay.gueron@gmail.com,
 * rafaelmisoczki@google.com, tobias.oder@rub.de, tim.gueneysu@rub.de,
 * jan.richter-brockmann@rub.de.
 *
 * Permission to use this code for BIKE is granted.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * * The names of the contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ""AS IS"" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS CORPORATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "hash_wrapper.h"
#include "openssl_utils.h"
#include "ntl.h"
#include "decode.h"
#include "sampling.h"
#include "kem.h"
#include "conversions.h"
#include "shake_prng.h"

// Function H. It uses the extract-then-expand paradigm based on SHA384 and
// AES256-CTR PRNG to produce e from m and mu.
_INLINE_ status_t functionH(
        OUT uint8_t * e,
        IN const uint8_t * m,
        IN const uint8_t * mu)
{
    status_t res = SUCCESS;

    // format seed as a 32-bytes input:
//    seed_t seed_for_hash;
//    memcpy(seed_for_hash.raw, m, ELL_SIZE);

    // format seed as a 64-bytes input:
    uint8_t seed_for_hash[2 * ELL_SIZE];
    memcpy(seed_for_hash, m, ELL_SIZE);
    memcpy(seed_for_hash + ELL_SIZE, mu, ELL_SIZE);

    // use the seed to generate sparse error vector e:
    DMSG("    Generating random error.\n");
    shake256_prng_state_t prng_state = {0};
//    shake256_init(seed_for_hash.raw, ELL_SIZE, &prng_state);
    shake256_init(seed_for_hash, 2 * ELL_SIZE, &prng_state);
    res = generate_sparse_rep_keccak(e, T1, N_BITS, &prng_state, false); CHECK_STATUS(res);

    EXIT:
    DMSG("  Exit functionH.\n");
    return res;
}

// Function L. Computes L(e0 || e1)
_INLINE_ status_t functionL(
        OUT uint8_t * output,
        IN const uint8_t * e)
{
    status_t res = SUCCESS;
    uint8_t hash_value[SHA384_HASH_SIZE] = {0};
    uint8_t e_split[2 * R_SIZE] = {0};

    ntl_split_polynomial(e_split, &e_split[R_SIZE], e);

    // select hash function
    sha3_384(hash_value, e_split, 2*R_SIZE);

    memcpy(output, hash_value, ELL_SIZE);

    DMSG("  Exit functionL.\n");
    return res;
}

// Function K. Computes K(m || c0 || c1).
_INLINE_ status_t functionK(
        OUT uint8_t * output,
        IN const uint8_t * m,
        IN const uint8_t * c0,
        IN const uint8_t * c1)
{
    status_t res = SUCCESS;
    sha384_hash_t large_hash = {0};

    // preparing buffer with: [m || c0 || c1]
    uint8_t tmp1[ELL_SIZE + 2*R_SIZE] = {0};
    memcpy(tmp1, m, ELL_SIZE);
    memcpy(tmp1 + ELL_SIZE, c0, R_SIZE);
    memcpy(tmp1 + ELL_SIZE + R_SIZE, c1, ELL_SIZE);

    // shared secret =  K(m || c0 || c1)
    // select hash function
    sha3_384(large_hash.raw, tmp1, 2*ELL_SIZE + R_SIZE);
    memcpy(output, large_hash.raw, ELL_SIZE);
  
    DMSG("  Exit functionK.\n");
    return res;
}

_INLINE_ status_t compute_syndrome(OUT syndrome_t* syndrome,
        IN const ct_t* ct,
        IN const sk_t* sk)
{
    status_t res = SUCCESS;
    uint8_t s_tmp_bytes[R_BITS] = {0};
    uint8_t s0[R_SIZE] = {0};

    // syndrome: s = c0*h0
    ntl_mod_mul(s0, sk->val0, ct->val0);

    // store the syndrome in a bit array
    convertByteToBinary(s_tmp_bytes, s0, R_BITS);
    transpose(syndrome->raw, s_tmp_bytes);

    DMSG("  Exit compute_syndrome.\n");

    return res;
}

////////////////////////////////////////////////////////////////
//The three APIs below (keypair, enc, dec) are defined by NIST:
//In addition there are two KAT versions of this API as defined.
////////////////////////////////////////////////////////////////
int crypto_kem_keypair(OUT unsigned char *pk, OUT unsigned char *sk)
{
    //Convert to these implementation types
    sk_t* l_sk = (sk_t*)sk;
    pk_t* l_pk = (pk_t*)pk;

    // return code
    status_t res = SUCCESS;

    //For NIST DRBG_CTR
    double_seed_t seeds = {0};
    shake256_prng_state_t h_prng_state = {0};

    //Get the entropy seeds
    get_seeds(&seeds, KEYGEN_SEEDS);

    // sk = (h0, h1, sigma)
    uint8_t * h0 = l_sk->val0;
    uint8_t * h1 = l_sk->val1;
    uint8_t * mu = l_sk->mu; // part of the public key (first ELL_SIZE bytes)
    uint8_t * sigma = l_sk->sigma;

    uint8_t inv_h0[R_SIZE] = {0};

    DMSG("  Enter crypto_kem_keypair.\n");
    DMSG("    Calculating the secret key.\n");

    shake256_init(seeds.s1.raw, ELL_SIZE, &h_prng_state);
    res = generate_sparse_rep_keccak(h0, DV, R_BITS, &h_prng_state, true); CHECK_STATUS(res);
    res = generate_sparse_rep_keccak(h1, DV, R_BITS, &h_prng_state, true); CHECK_STATUS(res);

    // use the second seed as sigma
    memcpy(sigma, seeds.s2.raw, ELL_SIZE);

    DMSG("    Calculating the public key.\n");

    // pk = (1, h1*h0^(-1)), the first pk component (1) is implicitly assumed
    ntl_mod_inv(inv_h0, h0);
    ntl_mod_mul(l_pk->val, h1, inv_h0);

    // mu gets the first ELL_SIZE bytes of l_pk->val
    memcpy(mu, l_pk->val, ELL_SIZE);

    EDMSG("h0: "); print((uint64_t*)l_sk->val0, R_BITS);
    EDMSG("h1: "); print((uint64_t*)l_sk->val1, R_BITS);
    EDMSG("h: "); print((uint64_t*)l_pk->val, R_BITS);
    EDMSG("mu: "); print((uint64_t*)l_sk->mu, ELL_BITS);
    EDMSG("sigma: "); print((uint64_t*)l_sk->sigma, ELL_BITS);

    EXIT:
    DMSG("  Exit crypto_kem_keypair.\n");
    return res;
}

//Encapsulate - pk is the public key,
//              ct is a key encapsulation message (ciphertext),
//              ss is the shared secret.
int crypto_kem_enc(OUT unsigned char *ct,
        OUT unsigned char *ss,
        IN  const unsigned char *pk)
{
    DMSG("  Enter crypto_kem_enc.\n");

    status_t res = SUCCESS;

    //Convert to these implementation types
    const pk_t* l_pk = (pk_t*)pk;
    ct_t* l_ct = (ct_t*)ct;
    ss_t* l_ss = (ss_t*)ss;

    //For NIST DRBG_CTR.
    double_seed_t seeds = {0};

    //Get the entropy seeds.
    get_seeds(&seeds, ENCAPS_SEEDS);

    // quantity m:
    uint8_t m[ELL_SIZE] = {0};

    // error vector:
    uint8_t e[N_SIZE] = {0};
    uint8_t e0[R_SIZE] = {0};
    uint8_t e1[R_SIZE] = {0};

    // temporary buffer:
    uint8_t tmp[ELL_SIZE] = {0};

    // first bytes of the public key
    uint8_t mu[ELL_SIZE] = {0};
    memcpy(mu, l_pk->val, ELL_SIZE);

    //random data generator; Using seed s1
    memcpy(m, seeds.s1.raw, ELL_SIZE);

    // (e0, e1) = H(m, mu)
    functionH(e, m, mu);
    ntl_split_polynomial(e0, e1, e);

    // ct = (c0, c1) = (e0 + e1*h, L(e0, e1) \XOR m)
    ntl_mod_mul(l_ct->val0, e1, l_pk->val);
    ntl_add(l_ct->val0, l_ct->val0, e0);
    functionL(tmp, e);
    for (uint32_t i = 0; i < ELL_SIZE; i++)
        l_ct->val1[i] = tmp[i] ^ m[i];

    // Function K:
    //shared secret =  K(m || c0 || c1)
    functionK(l_ss->raw, m, l_ct->val0, l_ct->val1);

    EDMSG("ss: "); print((uint64_t*)l_ss->raw, sizeof(*l_ss)*8);

    EXIT:

    DMSG("  Exit crypto_kem_enc.\n");
    return res;
}

//Decapsulate - ct is a key encapsulation message (ciphertext),
//              sk is the private key,
//              ss is the shared secret
int crypto_kem_dec(OUT unsigned char *ss,
        IN const unsigned char *ct,
        IN const unsigned char *sk)
{
    DMSG("  Enter crypto_kem_dec.\n");
    status_t res = SUCCESS;

    // convert to this implementation types
    const sk_t* l_sk = (sk_t*)sk;
    const ct_t* l_ct = (ct_t*)ct;
    ss_t* l_ss = (ss_t*)ss;

    int failed = 0;

    // for NIST DRBG_CTR
    double_seed_t seeds = {0};
  
    uint8_t e_recomputed[N_SIZE] = {0};

    uint8_t Le0e1[ELL_SIZE + 2*R_SIZE] = {0};
    uint8_t m_prime[ELL_SIZE] = {0};

    uint32_t h0_compact[DV] = {0};
    uint32_t h1_compact[DV] = {0};

    uint8_t e_prime[N_SIZE] = {0};
    uint8_t e_twoprime[R_BITS*2] = {0};
   
    uint8_t e_tmp1[R_BITS*2] = {0};
    uint8_t e_tmp2[N_SIZE] = {0};

    uint8_t e0rand[R_SIZE] = {0};
    uint8_t e1rand[R_SIZE] = {0};

    int rc;

    DMSG("  Converting to compact rep.\n");
    convert2compact(h0_compact, l_sk->val0);
    convert2compact(h1_compact, l_sk->val1);

    DMSG("  Computing s.\n");
    syndrome_t syndrome;

       // Step 1. computing syndrome:
    res = compute_syndrome(&syndrome, l_ct, l_sk); CHECK_STATUS(res);

    // Step 2. decoding:
    DMSG("  Decoding.\n");
    rc = BF_decoder(e_tmp1, syndrome.raw, h0_compact, h1_compact);

    convertBinaryToByte(e_prime, e_tmp1, 2*R_BITS);

    // Step 3. compute L(e0 || e1)
    functionL(Le0e1, e_prime);

    // Step 4. retrieve m' = c1 \xor L(e0 || e1)
    for(uint32_t i = 0; i < ELL_SIZE; i++)
    {
        m_prime[i] = l_ct->val1[i] ^ Le0e1[i];
    }

    // Step 5. (e0, e1) = H(m, mu)
    functionH(e_recomputed, m_prime, l_sk->mu);

    if (!safe_cmp(e_recomputed, e_prime, N_SIZE))
    {
        DMSG("recomputed error vector does not match decoded error vector\n");
        failed = 1;
    }

    // Step 6. compute shared secret k = K()
    if (failed) {
        // shared secret = K(sigma || c0 || c1)
        functionK(l_ss->raw, l_sk->sigma, l_ct->val0, l_ct->val1);
    }
    else
    {
       // shared secret = K(m' || c0 || c1)
        functionK(l_ss->raw, m_prime, l_ct->val0, l_ct->val1);
    }

    EXIT:

    DMSG("  Exit crypto_kem_dec.\n");
    return res;
}

