/*
 * Base58Check encoding
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

/*
 * Base58Check encoding:
 * - append 4-byte double SHA-256 checksum
 * - encode the result in base-58
 * - prepend ones as necessary to indicate leading zero bytes, if present.
 */

#ifndef BASE58_H_INCLUDED
#define BASE58_H_INCLUDED

#include <stdbool.h>
#include "secp256k1.h"

// Base-58 mapping: "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
const char base58_map[60];

// Encode len bytes of data into b58_buf.
void base58check_encode(const unsigned char *data, int len, char *b58_buf);

// Encode bitcoin address into b58_buf.
// The buffer for storing Base58 output must be able to hold 35 bytes.
// avb is the Application/Version Byte of Base58Check.
void base58_encode_address(const curve_point *pub, unsigned char avb,
                           bool compressed, char *b58_buf);

#endif
