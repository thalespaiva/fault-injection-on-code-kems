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

#include "sampling.h"

_INLINE_ uint32_t count_ones(IN const uint8_t* a,
        IN const uint32_t len)
{
    uint32_t count = 0;

    for(uint32_t i = 0; i < len; i++)
    {
        count += __builtin_popcountll(a[i]);
    }

    return count;
}

status_t get_rand_mod_len_keccak(OUT uint32_t* rand_pos,
        IN const uint32_t len,
        IN OUT shake256_prng_state_t* prf_state,
        IN const bool uniform)
{
    status_t res = SUCCESS;

		if (uniform) {
				do {
						// Generate 32 random bits
						res = shake256_prng((uint8_t*) rand_pos, prf_state, sizeof(*rand_pos));
						// to ensure uniformity, with 0 <= rnd < 2^32,
						// reject if rnd > len * floor(2^32 / len)
						// i.e. rnd - (rnd % len) > 2^32 - len
				} while ((*rand_pos) - ((*rand_pos) % len) > -len); // -len = 2^32 - len
				*rand_pos = (*rand_pos) % len; // 0 <= *rand_pos < len

				/* alternative with Lemire's trick, needs a check */
				/* uint64_t temp; */
				/* do { */
				/* 		res = shake256_prng((uint8_t*) rand_pos, prf_state, sizeof(*rand_pos)); */
				/* 		uint64_t temp = *rand_pos; */
				/* 		temp *= len; */
				/* } while ((temp & 0xFFFFFFFF) > (1L<<32) - ((1L<<32) % len)); */
				/* *rand_pos = temp >> 32; */

		} else { // rejection not allowed
				// Generate 32 random bits
				res = shake256_prng((uint8_t*) rand_pos, prf_state, sizeof(*rand_pos));
				// Lemire's trick replaces a division with a multiplication (by len)
				// the result of the multiplication by len may exceed 32 bits
				// so we cast (*rand_pos) on a 64 bits word
				uint64_t temp = *rand_pos;
				*rand_pos = (temp * len) >> 32; // 0 <= *rand_pos < len
		}
    EXIT:
    return res;
}

void setZero(uint8_t * r, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
        r[i] = 0;
}
int CHECK_BIT(uint8_t * tmp, int position) {
    int index = position/8;
    int pos = position%8;
    return ((tmp[index] >> (pos))  & 0x01);
}
void SET_BIT(uint8_t * tmp, int position) {
    int index = position/8;
    int pos = position%8;
    tmp[index] |= 1UL << (pos);
}


status_t generate_sparse_rep_keccak(OUT uint8_t * r,
        IN  const uint32_t weight,
        IN  const uint32_t len,
        IN OUT shake256_prng_state_t *prf_state,
        IN const bool uniform)
{
    uint32_t rand_pos = 0;
    status_t res = SUCCESS;

    //Ensure r is zero.
    setZero(r, DIVIDE_AND_CEIL(len, 8ULL));

    for (int32_t i = weight - 1; i >= 0; i--)
    {
        res = get_rand_mod_len_keccak(&rand_pos, len - i, prf_state, uniform);
        CHECK_STATUS(res);

        rand_pos += i; // now i <= rand_pos < len

        if (CHECK_BIT(r, rand_pos))
        {
            // If collision, then select index i instead of rand_pos
            rand_pos = i;
        }
        SET_BIT(r, rand_pos);
    }

    EXIT:
    return res;
}
