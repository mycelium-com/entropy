/**
 * Base58Check encoding
 *
 * Implementation based on trezor-crypto/base58.c.
 *
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
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
#include <string.h>

#include "base58.h"
#include "sha256.h"
#include "ripemd.h"

// Mapping of Base58 code points to characters.
// This is also used as font map, and we use dash in the font in addition to
// the Base58 alphabet.
const char base58_map[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz-";

// Encode len bytes of data into b58_buf using Base58Check encoding.
void base58check_encode(const unsigned char *data, int len, char *b58_buf)
{
    uint8_t num[len + 4];       // numerator
    uint32_t hash[SHA256_SIZE / 4];
    unsigned num_is_non_zero;
    int i, j;

    // compute and append checksum
    sha256_twice(hash, data, len);
    memcpy(num, data, len);
    memcpy(num + len, hash, 4);
    len += 4;

    // compute base58
    i = 0;
    do {
        unsigned rem = 0, tmp;
        num_is_non_zero = 0;

        for (j = 0; j < len; j++) {
            tmp = rem * 24 + num[j];    // 2^8 == 4*58 + 24
            num_is_non_zero |= num[j] = rem * 4 + tmp / 58;
            rem = tmp % 58;
        }
        b58_buf[i++] = base58_map[rem];
    } while (num_is_non_zero);

    // add '1' for each leading 0 in data
    while (!*data++)
        b58_buf[i++] = '1';
    b58_buf[i--] = 0;

    // reverse
    for (j = i >> 1; j >= 0; --j) {
        uint8_t tmp = b58_buf[j];
        b58_buf[j] = b58_buf[i - j];
        b58_buf[i - j] = tmp;
    }
}

// Encode bitcoin address into b58_buf.
// The buffer for storing Base58 output must be able to hold 35 bytes.
void base58_encode_address(const curve_point *pub, unsigned char avb,
                           bool compressed, char *b58_buf)
{
    union {
        struct {
            uint32_t unused1;
            uint32_t hash[5];
        };
        struct {
            uint8_t unused2[3];
            uint8_t address[1 + 20];    // bytes to encode
        };
    } to_encode;

    uint8_t keybytes[1 + 64];
    uint32_t hash_tmp[SHA256_SIZE / 4];

    // serialise public key and compute its hash
    bn_write_be(&pub->x, keybytes + 1);
    if (compressed) {
        keybytes[0] = 2 + (pub->y.val[0] & 1);
        sha256_hash(hash_tmp, keybytes, 1 + 32);
    } else {
        keybytes[0] = 4;
        bn_write_be(&pub->y, keybytes + 1 + 32);
        sha256_hash(hash_tmp, keybytes, 1 + 64);
    }
    ripemd160_hash(to_encode.hash, (uint8_t *)hash_tmp, sizeof hash_tmp);

    // add prefix
    to_encode.address[0] = avb;

    base58check_encode(to_encode.address, 21, b58_buf);
}
