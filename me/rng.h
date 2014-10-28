/*
 * Entropy collection and extraction, and random number generation.
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

#ifndef RNG_H_INCLUDED
#define RNG_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

// Collect raw entropy and generate RNG seed.
// Return false if self-tests fail.
bool rng_init(void);
// Produce a 256-bit random number, adding more raw entropy when necessary.
void rng_next(uint32_t random_number[8]);

#endif
