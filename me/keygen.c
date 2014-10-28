/*
 * Private & public key pair generation for Mycelium Entropy.
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lib/base58.h"
#include "lib/ecdsa.h"
#include "data.h"
#include "layout.h"
#include "rng.h"
#include "keygen.h"


int keygen(uint8_t coin, bool compressed, uint8_t key_buf[])
{
    int len;

    union {
        struct {
            uint32_t unused1;
            uint32_t words[8];
        };
        struct {
            uint8_t unused2[3];
            uint8_t sipa[34];
        };
    } key;              // private key as big endian bytes
    bignum256 tkey;     // private key in bignum format
    curve_point pub;    // public key

    // generate a random 256-bit number;
    // try again if it is >= field prime
    do {
        rng_next(key.words);
        bn_read_be((uint8_t *)key.words, &tkey);
    } while (!bn_is_less(&tkey, &order256k1));

    // compute public key
    scalar_multiply(&tkey, &pub);

    // SIPA formatting
    key.sipa[0] = coin | 0x80;
    len = 33;
    if (compressed)
        key.sipa[len++] = 1;
    memcpy(key_buf, key.sipa, len);

    base58check_encode(key.sipa, len, texts[IDX_PRIVKEY]);
    base58_encode_address(&pub, coin, compressed, texts[IDX_ADDRESS]);

    printf("Address: %s.\n", texts[IDX_ADDRESS]);

    return len;
}
