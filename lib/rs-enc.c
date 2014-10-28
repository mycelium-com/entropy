/*
 * 8-bit Reed-Solomon encoder.
 * Includes support for Galois Field GF(2^8) maths.
 *
 * Copyright 2013-2014 Mycelium SA, Luxembourg.
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

#include <string.h>
#include <assert.h>
#include "rs.h"

struct RS_state rs;

// logarithmic multiplication
// assumes b != INFTY, as is the case here
static unsigned logmul(unsigned a, unsigned b)
{
    if (a == INFTY)
        return INFTY;
    a += b;
    return a >= 255 ? a - 255 : a;      // modulo 255
}

// logarithmic addition
static inline unsigned logadd(unsigned a, unsigned b)
{
    return rs.log[rs.exp[a] ^ rs.exp[b]];
}

void rs_init(unsigned gf_poly, unsigned degree)
{
    unsigned i, b;

    assert(degree < sizeof (rs.poly));

    rs.n = degree;

    // generate Galois field exp and log lookup tables
    b = 1;
    for (i = 0; i < 255; i++) {
        // b = 2**i in GF
        rs.log[b] = i;
        rs.exp[i] = b;
        b <<= 1;
        if (b & 0x100)
            b ^= gf_poly;
    }
    // special case: let log(0) be 255, representing -∞
    rs.log[0] = INFTY;
    rs.exp[INFTY] = 0;

    // check if this polynomial really generates a GF
    assert(b == 1);

    // Calcluate RS code generator polynomial:
    // its roots are powers of 2 from 0 to the specified degree,
    // so it's a product of (x ^ 2**i), 0 ≤ i ≤ degree-1.
    // The elements of poly are logarithms of coefficients.
    rs.poly[0] = 0;
    for (i = 0; i < degree; i++) {
        unsigned j;

        rs.poly[i+1] = 0;

        // multiply poly by (x ^ 2**i), aka (x + 2**i) or (x - 2**i)
        for (j = i; j > 0; --j)
            rs.poly[j] = logadd(rs.poly[j-1], logmul(rs.poly[j], i));
        rs.poly[0] = logmul(rs.poly[0], i);
    }
}

void rs_encode(const uint8_t *msg, int len, uint8_t *out)
{
    int i;
    unsigned j;
    uint8_t buf[len + rs.n];

    assert(len > 0);

    memcpy(buf, msg, len);
    memset(buf + len, 0, rs.n);

    for (i = 0; i < len; i++) {
        unsigned coef = buf[i];
        if (coef != 0) {
            coef = rs.log[coef];
            for (j = 1; j <= rs.n; j++)
                buf[i+j] ^= rs.exp[logmul(rs.poly[rs.n - j], coef)];
        }
    }

    memcpy(out, buf + len, rs.n);
}
