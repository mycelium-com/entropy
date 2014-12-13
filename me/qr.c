/*
 * QR code encoder.
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

#include "lib/rs.h"
#include "qr.h"

enum {
    // format parameters
    ECL_L   = 1,
    ECL_M   = 0,
    ECL_Q   = 3,
    ECL_H   = 2,

    // mask: diagonal lines (fixed)
    MASK    = 3,

    // data encoding mode (we only use byte mode)
    BYTE_MODE = 4,
};

static const unsigned raw_capacity[] = {
    QR3_RAW_BYTES, QR4_RAW_BYTES, QR5_RAW_BYTES, QR6_RAW_BYTES,
};
static const uint8_t ecc_len[] = {
    // ECC lengths for H, Q, M, L levels
    RSLEN_3_H, RSLEN_3_Q, RSLEN_3_M, RSLEN_3_L,
    RSLEN_4_H, RSLEN_4_Q, RSLEN_4_M, RSLEN_4_L,
    RSLEN_5_H, RSLEN_5_Q, RSLEN_5_M, RSLEN_5_L,
    RSLEN_6_H, RSLEN_6_Q, RSLEN_6_M, RSLEN_6_L,
};

// Fill all "modules" (pixels) belonging to fixed patterns with 1,
// and all data modules with 0.
static void fill_fixed_pattern_mask(qr_row_t *qr, int size)
{
    qr_row_t p;
    int i;

    // top finders and format info
    p = 0x1ff | (qr_row_t)0x1fe << (size - 9);
    for (i = 0; i < 9; i++)
        qr[i] = p;

    // horizontal timing
    qr[6] = ((qr_row_t)1 << size) - 1;

    // vertical timing
    for (; i < size - 9; i++)
        qr[i] = 1 << 6;

    // alignment and bottom finder
    p = 0x40 | (qr_row_t)0x1f << (size - 9);
    for (; i < size - 4; i++) {
        qr[i] = p;
        p |= 0x1ff;
    }
    for (; i < size; i++)
        qr[i] = 0x1ff;
}

// Draw fixed patterns by selectively converting 1s in the mask to 0s
// where the corresponding module is white.
static void apply_fixed_patterns(qr_row_t *qr, int size)
{
    qr_row_t p;
    int i;

    // top finders
    qr[0] ^= 0x180 | (qr_row_t)0x02 << (size - 9);
    p = 0x1be | (qr_row_t)0xfa << (size - 9);
    qr[1] ^= p;
    qr[5] ^= p;
    p = 0x1a2 | (qr_row_t)0x8a << (size - 9);
    qr[2] ^= p;
    qr[3] ^= p;
    qr[4] ^= p;

    // horizontal timing
    qr[6] ^= 0x2aaa80 | (qr_row_t)0x2aaa80 << (size - 29);

    // format info
    p = 0x1bf | (qr_row_t)0x1fe << (size - 9);
    qr[7] ^= p | 0x40;
    qr[8] ^= p;

    // vertical timing
    for (i = 9; i < size - 9; i++)
        qr[i] ^= (i & 1) << 6;

    // alignment and bottom finder
    qr[++i] ^= 0xff | (qr_row_t)0xe << (size - 9);
    qr[++i] ^= 0x180 | (qr_row_t)0xa << (size - 9);
    qr[++i] ^= 0x1be | (qr_row_t)0xe << (size - 9);
    qr[++i] ^= 0x1a2;
    qr[++i] ^= 0x1a2;
    qr[++i] ^= 0x1a2;
    qr[++i] ^= 0x1be;
    qr[++i] ^= 0x180;
}

// bitstream feeder state and interleave parameters
static uint8_t *msg_ptr;
static int      msg_bit;
static uint16_t msg_idx;
static uint16_t msg_pitch;
static uint8_t  msg_nparts;
static uint8_t  msg_part;
static uint8_t  msg_data_len;
static uint8_t  msg_data_skip;

static unsigned next_bit(void)
{
    static uint8_t byte;

    if (--msg_bit < 0) {
        msg_bit = 7;
        byte = msg_ptr[msg_idx + msg_part * msg_pitch];
        msg_part++;
        while (msg_part == msg_nparts) {
            msg_part = 0;
            msg_idx++;
            if (msg_idx == msg_data_len)
                msg_part = msg_data_skip;
            else
                break;
        }
    }
    return byte >> msg_bit & 1;
}

// Serialise data from msg_ptr as a bitstream and fit it over 0-bits in qr,
// thus avoiding fixed patterns.
static void fill_bitstream(qr_row_t *qr, int size)
{
    int x = size - 1;
    int y = size - 1;
    int dir = -1;
    qr_row_t rbit = (qr_row_t)1 << x;
    qr_row_t lbit = rbit >> 1;

    // start in the bottom right corner, going up
    do {
        // Using just one of the eight possible inversion masks,
        // 011: (x+y) % 3 == 0, compute (x+y) % 3 without division;
        // actually, do it for the left position (x-1, y).
        int mask = x + y - 1;
        mask = (mask>>4) + (mask&15);
        mask = (mask>>2) + (mask&3);
        mask = (mask>>2) + (mask&3);
        // This gives a result between 1 and 4 for all points except (0,0),
        // but that one belongs to a fixed pattern and is not used for the
        // bit stream.
        // For the left position the masking condition is equivalent to
        // mask == 3, and the right position is +1, so mask == 2.

        if ((qr[y] & rbit) == 0)
            qr[y] |= (qr_row_t)((mask == 2) ^ next_bit()) << x;
        if ((qr[y] & lbit) == 0)
            qr[y] |= (qr_row_t)((mask == 3) ^ next_bit()) << (x-1);

        y += dir;

        if (y < 0)
            y++;
        else if (y == size)
            --y;
        else
            continue;

        // reached top or bottom edge: change vertical direction and
        // shift left by 2, or by 3 if we are at the vertical timing
        dir = -dir;
        x -= 2;
        if (x == 6)
            --x;
        rbit = (qr_row_t)1 << x;
        lbit = rbit >> 1;
    } while (x >= 0);
}

// Encode the format word with error correction.
static unsigned encode_format(unsigned fmt, unsigned poly, int degree)
{
    unsigned h = 1 << degree;
    int i;

    fmt <<= degree;

    for (i = 31 - degree; i >= 0; --i)
        if (fmt & h << i)
            fmt ^= poly << i;

    return fmt;
}

// Insert format word fmt into the appropriate places in qr.
static void insert_format(unsigned fmt, qr_row_t *qr, int size)
{
    // horizontal positions in line 8 for each bit of format
    static const int8_t hpos[15] = {
        -1, -2, -3, -4, -5, -6, -7, -8,
        7, 5, 4, 3, 2, 1, 0
    };
    // vertical positions in column 8 for each bit of format
    static const int8_t vpos[15] = {
        0, 1, 2, 3, 4, 5, 7, 8,
        -7, -6, -5, -4, -3, -2, -1
    };

    int i;
    int offset = size;

    for (i = 0; i < 15; i++) {
        if (i == 8)
            offset = 0;
        if (fmt & 1 << i) {
            qr[8] |= (qr_row_t)1 << (hpos[i] + offset);
            qr[vpos[i] + size - offset] |= 1 << 8;
        }
    }
}

// Generate QR code from msg.
void qr_encode(const char *msg, qr_row_t *qr, int size)
{
    int i;
    int c = strlen(msg);
    int len;                            // length of data without EC
    unsigned ecl;                       // EC level
    unsigned f;
    uint8_t buf[QR5_RAW_BYTES + 5];     // 5 for gaps and remaining bits
    uint8_t *bufp = buf;

    // find highest EC level for the requested QR size with sufficient
    // data capacity to hold msg + two bytes for mode and length;
    // usable capacty = raw capacity - RS length - 2
    f = raw_capacity[(size - 29) / 4];  // raw capacity in bytes
    i = size - 29;
    while (len = f - ecc_len[i], c > len - 2) {
        i++;
        assert(i != size - 25);         // don't go beyond the lowest ECL
    }
    ecl = (i & 3) ^ 2;                  // ECL encoding for the format byte

    // compute the partitioning of data and ecc bytes
    f -= len;           // ecc size
    int n = 1;          // number of parts
    while (f > 31) {
        f >>= 1;
        n <<= 1;
    }

    rs_init(0x11d, f);

    // tell the bit stream feeder about interleaving
    msg_nparts = n;
    msg_data_len = len / n;
    msg_data_skip = n - len % n;
    msg_pitch = msg_data_len + 1 + f;

    // fill QR data buffer with message as follows:
    // encoding mode (4 bits), length (8 bits for BYTE_MODE), message, 0000,
    // then fill up the remaining space with zeros;
    // do it one part at a time, appending error correction bytes to each part
    len %= n;           // number of data parts that are 1 byte longer
    c |= BYTE_MODE << 8;
    do {                // for each part
        for (i = 0; i < msg_data_len + (n <= len); i++) {
            bufp[i] = c >> 4;
            c = c << 8 | *msg;
            if (*msg)
                msg++;
        }
        rs_encode(bufp, i, bufp + msg_data_len + 1);
        bufp += msg_pitch;
    } while (--n);

    *bufp = 0;          // overflow bits

    // initialise bit stream feeder and fill QR code
    msg_ptr = buf;
    msg_idx = 0;
    msg_part = 0;
    msg_bit = 0;
    fill_fixed_pattern_mask(qr, size);
    fill_bitstream(qr, size);

    // overlay fixed patterns
    apply_fixed_patterns(qr, size);

    // generate format information
    f = ecl << 3 | MASK;                        // five bits
    f = f << 10 | encode_format(f, 0x537, 10);  // add 10 check bits
    f ^= 0x5412;                                // apply format mask

    insert_format(f, qr, size);
}
