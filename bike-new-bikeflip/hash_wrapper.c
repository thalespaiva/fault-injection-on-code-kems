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

#include "hash_wrapper.h"
#include "utilities.h"
#include "openssl/sha.h"
#include "string.h"
#include "stdio.h"

#include <openssl/evp.h>


/*
Wrapper for SHA3-384 from openssl
*/
void sha3_384(unsigned char* output, const unsigned char* input, uint64_t size){
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    unsigned int s;
    int check;
    status_t res = SUCCESS;

    // determine type
    const EVP_MD* md = EVP_sha3_384();
    if(md == NULL) res = E_SHA384_FAIL; CHECK_STATUS(res);

    // DigistInit
    check = EVP_DigestInit_ex(ctx, md, NULL);
    if(check == 0) res = E_SHA384_FAIL; CHECK_STATUS(res);

    // digist update
    check = EVP_DigestUpdate(ctx, input, size);
    if(check == 0) res = E_SHA384_FAIL; CHECK_STATUS(res);

    // digist final
    check = EVP_DigestFinal(ctx, output, &s);
    if(check == 0) res = E_SHA384_FAIL; CHECK_STATUS(res);

    // clean up
    EVP_MD_CTX_free(ctx);

    EXIT:
    DMSG("  Exit SHA3-384.\n");
}
