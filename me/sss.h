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

#ifndef SSS_H_INCLUDED
#define SSS_H_INCLUDED

#include <stdint.h>

enum {
    // maximum total size of the share string:
    // "SSS-" prefix + base58 + NUL
    SSS_STRING_SIZE     = 62,

    // maximum secret size
    SSS_MAX_SECRET_SIZE = 34,
};

// Content type byte values as specified in the draft BIP.
enum {
    SSS_BASE58      = 19,
};

// Split secret into n shares, of which m shares are required to recover the
// secret.  Encode the resulting shares in base-58.
// 1 ≤ thr ≤ n ≤ 16.
// stype is one of SSS_... above.
// Write encoded shares into the global 'texts' array.
void sss_encode(int m, int n, uint8_t stype, const uint8_t secret[], int len);

#endif
