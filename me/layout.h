/*
 * Picture layouts for JPEG generator.
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

#ifndef LAYOUT_H
#define LAYOUT_H

// Fragment types
enum {
    FGM_LARGE_PICTURE,
    FGM_PICTURE,
    FGM_PICTURE_BY_REF,
    FGM_QR,
    FGM_TEXT,
    FGM_STOP,
};

// Text indices
enum {
    IDX_ADDRESS,
    IDX_XPUB     = IDX_ADDRESS,
    IDX_PRIVKEY,
    IDX_UNSALTED = 4,
    IDX_HD_PATH,
};
// For Shamir's Secret Sharing part n of m
#define IDX_SSS_PART(n) (n)

struct Layout {
    uint8_t  type;      // FGM_xxx
    uint8_t  vstep;     // vertical difference from previous fragment
    uint16_t x;         // horizontal position

    union {
        // picture fragment (small or large)
        const uint16_t *pic;        // direct reference
        const uint16_t **picref;    // indirect reference

        // QR code
        struct {
            uint16_t idx;       // text index IDX_xxx
            uint16_t size;      // QR_SIZE(ver): 29 or 33
        } qr;

        // text fragment
        struct {
            uint16_t idx;       // text index IDX_xxx
            uint16_t width;     // width in characters
        } text;
    };
};

extern const struct Layout main_layout[];
extern const struct Layout shamir_layout[];
extern const struct Layout salt1_layout[];
extern const struct Layout shamir_salt1_layout[];
const uint16_t *bitcoin_address_ref;

#endif
