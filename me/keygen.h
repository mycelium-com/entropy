/*
 * Private & public key pair generation for Mycelium Entropy.
 *
 * Copyright 2014, 2015 Mycelium SA, Luxembourg.
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

#ifndef KEYGEN_H
#define KEYGEN_H

#include <stdint.h>
#include <stdbool.h>

// Store private key in SIPA format into key_buf.
// Return key length (33 or 34), or 0 on failure.
int keygen(uint8_t key_buf[]);

// Get a random number and salt it if requested.
// Store the result into ent[8].
void keygen_get_entropy(uint32_t ent[8]);

#endif

