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
#include "conf_access.h"

void xflash_init(void);
void xflash_read(uint8_t *data, uint16_t size, uint32_t address);
void xflash_write(const uint8_t *data, uint16_t size, uint32_t address);
// erase smallest erasable block: 4 kB (if capable) or 64 kB
void xflash_erase_4k(uint32_t address);

extern volatile bool xflash_ready;

// Flash parameters set during xflash_init()
extern uint32_t xflash_id;            // 3-byte JEDEC ID, e.g. 0x01461F
extern bool xflash_4k_erase_capable;  // whether the flash can erase 4 kB blocks
extern uint32_t xflash_num_blocks;    // size in 512 byte blocks

// To be called periodically to poll for background operations.
#if AT25DFX_JPEG
void xflash_periodic_interrupt(void);
bool xflash_erase(void);
bool xflash_check_ready(void);
bool xflash_write_jpeg(void);
#else
#define xflash_periodic_interrupt()
#endif

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
