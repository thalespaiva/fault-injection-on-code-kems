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

#include "shake_prng.h"
#include "string.h"
#include "stdio.h"
#include "utilities.h"

// Initialize KECCAK
void shake256_init(const u8 *in, u64 inLen, shake256_prng_state_t *s){
    ui r=1088; ui c=256; u8 sfx=0x1F;
    /*initialize*/ ui R=r/8; ui i,b=0; FOR(i,SHAKE256_STATE_SIZE) s->buffer[i]=0;
    /*absorb*/ while(inLen>0) { b=(inLen<R)?inLen:R; FOR(i,b) s->buffer[i]^=in[i]; in+=b; inLen-=b; if (b==R) { KeccakF1600(s->buffer); b=0; } }
    /*pad*/ s->buffer[b]^=sfx; if((sfx&0x80)&&(b==(R-1))) KeccakF1600(s->buffer); s->buffer[R-1]^=0x80; KeccakF1600(s->buffer);
    s->pos = 0;
}

// Squeeze
void shake256_squeeze(shake256_prng_state_t *s){
    KeccakF1600(s->buffer);
}

// generate random number
status_t shake256_prng(OUT uint8_t* a,
        IN shake256_prng_state_t* s,
        IN const uint32_t len)
{
    status_t res = SUCCESS;

    //When Len i smaller then whats left in the buffer
    //No need in additional AES.
    if ((len + s->pos) <= SHAKE256_BLOCK_SIZE)
    {
        memcpy(a, &(s->buffer[s->pos]), len);
        s->pos += len;

        return res;
    }

    //else copy zero bytes.
    uint32_t idx = SHAKE256_BLOCK_SIZE - s->pos;
    memcpy(a, &(s->buffer[s->pos]), idx);

    //Init s.pos;
    s->pos = 0;

    // squeeze
    shake256_squeeze(s);

    //Copy the tail.
    s->pos = len - idx;
    memcpy(&a[idx], s->buffer, s->pos);

    EXIT:
    return res;
}



/*
The following code was copied from the XKCP library
https://github.com/XKCP/XKCP
*/
int LFSR86540(u8 *R) { (*R)=((*R)<<1)^(((*R)&0x80)?0x71:0); return ((*R)&2)>>1; }
#define ROL(a,o) ((((u64)a)<<o)^(((u64)a)>>(64-o)))
static u64 load64(const u8 *x) { ui i; u64 u=0; FOR(i,8) { u<<=8; u|=x[7-i]; } return u; }
static void store64(u8 *x, u64 u) { ui i; FOR(i,8) { x[i]=u; u>>=8; } }
static void xor64(u8 *x, u64 u) { ui i; FOR(i,8) { x[i]^=u; u>>=8; } }
#define rL(x,y) load64((u8*)s+8*(x+5*y))
#define wL(x,y,l) store64((u8*)s+8*(x+5*y),l)
#define XL(x,y,l) xor64((u8*)s+8*(x+5*y),l)

void KeccakF1600(void *s)
{
    ui r,x,y,i,j,Y; u8 R=0x01; u64 C[5],D;
    for(i=0; i<24; i++) {
        /*θ*/ FOR(x,5) C[x]=rL(x,0)^rL(x,1)^rL(x,2)^rL(x,3)^rL(x,4); FOR(x,5) { D=C[(x+4)%5]^ROL(C[(x+1)%5],1); FOR(y,5) XL(x,y,D); }
        /*ρπ*/ x=1; y=r=0; D=rL(x,y); FOR(j,24) { r+=j+1; Y=(2*x+3*y)%5; x=y; y=Y; C[0]=rL(x,y); wL(x,y,ROL(D,r%64)); D=C[0]; }
        /*χ*/ FOR(y,5) { FOR(x,5) C[x]=rL(x,y); FOR(x,5) wL(x,y,C[x]^((~C[(x+1)%5])&C[(x+2)%5])); }
        /*ι*/ FOR(j,7) if (LFSR86540(&R)) XL(0,0,(u64)1<<((1<<j)-1));
    }
}

