/*
 * Hierarchical Deterministic wallets with mnemonic seeds.
 * BIP-32 and BIP-39 subset implementation for Mycelium Entropy.
 *
 * Copyright 2015 Mycelium SA, Luxembourg.
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

#ifndef HD_H_INCLUDED
#define HD_H_INCLUDED

bool hd_gen_seed_with_mnemonic(int ent, uint64_t seed[8], char *mnemonic);
bool hd_make_xpub(const uint8_t *seed, int len);

#endif
