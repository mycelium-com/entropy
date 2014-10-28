/*
 * Shamir's Secret Sharing.
 *
 * Copyright 2014 Mycelium SA, Luxembourg.
 *
 * This file is part of Mycelium Entropy.
 *
 * Mycelium Entropy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.  See file GPL in the source code
 * distribution or <http://www.gnu.org/licenses/>.
 *
 * Mycelium Entropy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "lib/rs.h"
#include "lib/base58.h"
#include "lib/sha256.h"
#include "data.h"
#include "layout.h"


// We can temporarily use the memory at _estack for coefficients.
extern uint32_t _estack[];
#define COEFF ((const uint8_t *)_estack)

void sss_encode(int m, int n, uint8_t stype, const uint8_t secret[], int len)
{
    assert(n > 0 && n < 16);
    assert(m > 0 && m <= n);
    assert(len <= SSS_MAX_SECRET_SIZE);

    struct Share {
        uint8_t stype;
        uint8_t id[2];
        uint8_t m_and_x;
        uint8_t y[SSS_MAX_SECRET_SIZE];
    } share;

    int x, i, j;

    sha256_twice(_estack, secret, len);
    share.id[0] = COEFF[0];
    share.id[1] = COEFF[1];
    share.stype = stype;
    share.m_and_x = (m - 1) << 4;

    // Make pseudo-random coefficients, which should be unpredictable to
    // anyone who doesn't know the secret.
    // This implementation does not follow the spec's optional algorithm
    // for computing deterministic coefficients, although it looks a bit
    // similar.  The results of hashing are treated as if they are already
    // in logarithmic form.
    // We need m - 1 coefficients of len bytes each.
    sha256_hash(_estack, secret, len + 1);
    for (i = SHA256_SIZE; i < len * (m - 1); i += SHA256_SIZE)
        sha256_hash(&_estack[i / 4], &COEFF[i - SHA256_SIZE], SHA256_SIZE);

    for (x = 1; x <= n; x++) {
        unsigned xpow = 0;          // log(x**i), starting from i = 0
        unsigned logx = rs.log[x];
        const uint8_t *cptr = COEFF;

        memcpy(share.y, secret, len);  // start with y = a0

        for (i = 1; i < m; i++) {
            xpow += logx;           // update xpow = log(x**i)
            if (xpow >= 255)
               xpow -= 255;         // modulo 255
            // multiply i-th coefficient by x**i and add to share.y in GF
            for (j = 0; j < len; j++) {
                unsigned c = *cptr++;
                if (c != INFTY) {
                    c += xpow;
                    if (c >= 255)
                        c -= 255;
                }
                share.y[j] ^= rs.exp[c];
            }
        }

        strcpy(texts[IDX_SSS_PART(x)], "SSS-");
        base58check_encode(&share.stype, len + offsetof(struct Share, y),
                           texts[IDX_SSS_PART(x)] + 4);

        share.m_and_x++;
    }
}
