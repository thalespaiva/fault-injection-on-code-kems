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

#include "decode.h"
#include "utilities.h"

#include "kem.h"
#include "sampling.h"

#include "ring_buffer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// count number of 1's in tmp:
uint32_t getHammingWeight(const uint8_t tmp[R_BITS], const uint32_t length)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        count+=tmp[i];
    }

    return count;
}

// function (not constant time) to check if an array is zero:
uint32_t isZero(uint8_t s[R_BITS])
{
    for (uint32_t i = 0; i < R_BITS; i++)
    {
        if (s[i])
        {
            return 0;
        }
    }
    return 1;
}

void recompute_syndrome(uint8_t s[R_BITS],
        const uint32_t pos,
        const uint32_t h0_compact[DV],
        const uint32_t h1_compact[DV])
{
    if (pos < R_BITS)
    {
        for (uint32_t j = 0; j < DV; j++)
        {
            if (h0_compact[j] <= pos)
            {
                s[pos - h0_compact[j]] ^= 1;
            }
            else
            {
                s[R_BITS - h0_compact[j] +  pos] ^= 1;
            }
        }
    }
    else
    {
       for (uint32_t j = 0; j < DV; j++)
       {
          if (h1_compact[j] <= (pos - R_BITS))
               s[(pos - R_BITS) - h1_compact[j]] ^= 1;
           else
               s[R_BITS - h1_compact[j] + (pos - R_BITS)] ^= 1;
       }
    }
}

uint32_t ctr(
        uint32_t h_compact_col[DV],
        int position,
        uint8_t s[R_BITS])
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < DV; i++)
    {
        if (s[(h_compact_col[i] + position) % R_BITS])
            count++;
    }
    return count;
}

void getCol(
        uint32_t h_compact_col[DV],
        uint32_t h_compact_row[DV])
{
    if (h_compact_row[0] == 0)
    {
        h_compact_col[0] = 0;

        for (uint32_t i = 1; i < DV; i++)
        {
            // set indices in increasing order:
            h_compact_col[i] = R_BITS - h_compact_row[DV-i];
        }
    } else
    {
        for (uint32_t i = 0; i < DV; i++)
        {
            // set indices in increasing order:
            h_compact_col[i] = R_BITS - h_compact_row[DV-1-i];
        }
    }
}

// The position in e is adjusted because syndrome is transposed.
void flipAdjustedErrorPosition(uint8_t e[R_BITS*2], uint32_t position)
{
    uint32_t adjustedPosition = position;
    if (position != 0 && position != R_BITS)
    {
        adjustedPosition = (position > R_BITS) ? \
                ((N_BITS - position)+R_BITS) : (R_BITS - position);
    }
    e[adjustedPosition] ^= 1;
}

void BFIter(uint8_t e[R_BITS*2],
    uint8_t s[R_BITS],
    uint32_t T,
    uint32_t h0_compact[DV],
    uint32_t h1_compact[DV],
    uint32_t h0_compact_col[DV],
    uint32_t h1_compact_col[DV])
{

    uint8_t pos[R_BITS*2] = {0};

    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h0_compact_col, j, s);
        if (counter >= T)
        {
            //e[j] ^= 1;
            flipAdjustedErrorPosition(e, j);
            pos[j] = 1;
        }
    }
    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h1_compact_col, j, s);

        if (counter >= T)
        {
            //e[R_BITS+j] ^= 1;
            flipAdjustedErrorPosition(e, R_BITS+j);
            pos[R_BITS+j] = 1;
        }
    }

    // flip bits at the end
    for(uint32_t j=0; j < 2*R_BITS; j++){
        if(pos[j] == 1){
            recompute_syndrome(s, j, h0_compact, h1_compact);
        }
    }
}

void GetThresholdLowerBounds(double T[NbIter], uint8_t s[R_BITS]) {
		double T0 = VAR_TH_FCT(getHammingWeight(s, R_BITS));
		double x = (T0 - (DV + 1) / 2) / 3;
		T[0] = T0 + delta;
		T[1] = T[0] - x;
		T[2] = T[1] - x;
    for (int i = 3; i < NbIter; i++)
				T[i] = (DV + 1) / 2 + delta;
}
		
		

// Algorithm BIKE-Flip Decoder
int BF_decoder(uint8_t e[R_BITS*2],
    uint8_t s[R_BITS],
    uint32_t h0_compact[DV],
    uint32_t h1_compact[DV])
{
    memset(e, 0, R_BITS*2);

    // computing the first column of each parity-check block:
    uint32_t h0_compact_col[DV] = {0};
    uint32_t h1_compact_col[DV] = {0};
    getCol(h0_compact_col, h0_compact);
    getCol(h1_compact_col, h1_compact);

		double Th_lb[NbIter]; // threshold lower bound
		GetThresholdLowerBounds(Th_lb, s);
		
    for (int i = 0; i < NbIter; i++)
    {
				double x = VAR_TH_FCT(getHammingWeight(s, R_BITS));
				uint32_t T = ceil(MAX(Th_lb[i], x));

        BFIter(e, s, T, h0_compact, h1_compact, h0_compact_col, h1_compact_col);
    }
    if (getHammingWeight(s, R_BITS) == 0)
        return 0; // SUCCESS
    else
        return 1; // FAILURE
}


