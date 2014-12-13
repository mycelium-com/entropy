/*
 * PBKDF2, RFC 2898.
 * HMAC/SHA-512 is used as the pseudorandom function.
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
 *
 * Keeping with the tradition of releasing basic cryptography functions to
 * public domain, Mycelium SA have waived all copyright and related or
 * neighbouring rights to this file and placed this file in public domain.
 */

#include <string.h>
#include "sha512.h"
#include "pbkdf2.h"


void pbkdf2_512(uint64_t key[8],
                const uint8_t *password, int plen,
                const uint8_t *salt, int slen,
                int iterations)
{
    int j, k;
    uint64_t buf[8];
    uint8_t salti[slen + 4];

    memcpy(salti, salt, slen);
    // Derived key size equals PRF size (512 bits) in this implementation.
    // Hence, i is always 1.
    salti[slen++] = 0;
    salti[slen++] = 0;
    salti[slen++] = 0;
    salti[slen++] = 1;

    sha512_hmac(buf, password, plen, salti, slen);
    memcpy(key, buf, sizeof buf);

    for (j = 1; j < iterations; j++) {
        sha512_hmac(buf, password, plen, (uint8_t *) buf, sizeof buf);
        for (k = 0; k != sizeof buf / sizeof buf[0]; k++)
            key[k] ^= buf[k];
    }
}
