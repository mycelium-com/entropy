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

#include <stdint.h>

enum {
    RS_MAX_DEGREE = 31,
    INFTY = 255,
};

// Initialise encoder.
// The degree of the RS polynomial equals to the number of error correction
// bytes.
// Maximum allowed degree is 31, which is enough for QR codes.
void rs_init(unsigned gf_poly, unsigned degree);

// Write degree bytes of error correction information for msg into out.
// As long as gf_poly and degree don't change, rs_encode may be called without
// intermediate calls to rs_init.
void rs_encode(const uint8_t *msg, int len, uint8_t *out);

// These functions hold parameters in a global structure.
// Should we need a re-entrant version, they are very easy to modify so as to
// let the caller allocate the structure and pass a pointer to it to both
// functions.

struct RS_state {
    unsigned n;         // degree of RS generator polynomial
    uint8_t  log[256];  // logarithms base 2 (aka α)
    uint8_t  exp[256];  // powers of 2 aka α
    uint8_t  poly[32];  // enough for QR codes
};

extern struct RS_state rs;
