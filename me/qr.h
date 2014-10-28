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

#ifndef QR_H
#define QR_H

#include <stdint.h>

// Maximum QR "version" (size) that we support: 3, 4 or 5.
#define QR_MAX_VERSION  4

// Size in "modules" (dots) of QR code;
// these should be surrounded by a "silent zone" four white (0) dots in width
#define QR_SIZE(ver)    (17 + (ver) * 4)

enum {
    QR3_RAW_BYTES       = 70,       // size 29
    QR4_RAW_BYTES       = 100,      // size 33
    QR5_RAW_BYTES       = 134,      // size 37

    // Reed-Solomon EC lengths in bytes
    RSLEN_3_L = 15,
    RSLEN_3_M = 26,
    RSLEN_3_Q = 36,
    RSLEN_3_H = 44,
    RSLEN_4_L = 20,
    RSLEN_4_M = 36,
    RSLEN_4_Q = 52,
    RSLEN_4_H = 64,
    RSLEN_5_L = 26,
    RSLEN_5_M = 48,
    RSLEN_5_Q = 72,
    RSLEN_5_H = 88,

    // maximum message length for each version
    QR3_MAX_LEN         = QR3_RAW_BYTES - RSLEN_3_L - 2,
    QR4_MAX_LEN         = QR4_RAW_BYTES - RSLEN_4_L - 2,
    QR5_MAX_LEN         = QR5_RAW_BYTES - RSLEN_5_L - 2,
};

#if QR_MAX_VERSION <= 3
typedef uint32_t qr_row_t;
#else
typedef uint64_t qr_row_t;
#endif

// Generate QR code from msg.
// The result is written into qr, top to bottom, least significant bit on the
// left hand side.  0 is white, 1 is black.
void qr_encode(const char *msg, qr_row_t *qr, int size);

#endif
