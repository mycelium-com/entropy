/*
 * External flash support.
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

#ifndef XFLASH_H_INCLUDED
#define XFLASH_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

bool xflash_init(void);
bool xflash_erase(uint32_t address);
bool xflash_write(const uint8_t *data, uint16_t size, uint32_t address);
bool xflash_verify(const uint8_t *data, uint16_t size, uint32_t address);

// Flash parameters set during xflash_init()
extern uint32_t xflash_num_blocks;    // size in 512 byte blocks

enum {
    // Part of uninitialised SRAM is saved into serial flash for comparing
    // at next power-up to make sure it's sufficiently different.
    // The offset from the end of flash and size of this snapshot is set here.
    XFLASH_RAM_SNAPSHOT_OFFSET  = 1024,
    XFLASH_RAM_SNAPSHOT_SIZE    = 1024,

    // BIP-39 word list.
    XFLASH_WORDLIST_OFFSET      = 65536,

    // Number of blocks to reserve at the end of flash.
    XFLASH_RESERVED_BLOCKS      = 128,
};

#endif
