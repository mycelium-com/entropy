/*
 * Parser for settings.txt.
 *
 * Copyright 2014, 2105 Mycelium SA, Luxembourg.
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

#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "lib/fwsign.h"
#include "lib/endian.h"

int parse_settings(struct Raw_public_key keys[], int len);

extern struct Settings {
    union {
        struct {
            uint8_t avb;    // coin type as in Application/Version Byte
            uint8_t bip44;  // coin type according to BIP-44
        };
        uint16_t    type;   // both coin types as one word
    } coin;
    bool    compressed;
    uint8_t salt_type;
    uint8_t salt_len;
    uint8_t salt[32];
    bool    hd;
    char    hd_path[32];
} settings;

// Coin types for settings.coin.
// The first byte is avb, followed by bip44.
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COIN_TYPE(avb, bip44)   ((avb) | (bip44) << 8)
#define COIN_BIP44(type)        ((type) >> 8)
#else
#define COIN_TYPE(avb, bip44)   ((avb) << 8 | (bip44))
#define COIN_BIP44(type)        ((type) & 0xff)
#endif
enum {
    BITCOIN         = COIN_TYPE(0, 0),
    BITCOIN_TESTNET = COIN_TYPE(0x6F, 1),
    LITECOIN        = COIN_TYPE(0x30, 2),
    PEERCOIN        = COIN_TYPE(0x37, 6),
    DOGECOIN        = COIN_TYPE(0x1E, 3),
    NAMECOIN        = COIN_TYPE(0x34, 7),
};

extern const struct Raw_public_key mycelium_public_key;

#endif
