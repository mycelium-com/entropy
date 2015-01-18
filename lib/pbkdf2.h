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

#ifndef LIB_PBKDF2_H
#define LIB_PBKDF2_H

#include <stdint.h>

void pbkdf2_512(uint64_t key[8],
                const char *password,
                const char *salt,
                int iterations);

#endif
