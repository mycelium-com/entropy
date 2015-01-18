/*
 * Xflash stubs for tests.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "xflash.h"

uint32_t xflash_num_blocks = 1024;

void xflash_read(uint8_t *data, uint16_t size, uint32_t address)
{
    // this is used by hd.c to read the word list
    static const char bip39_file_name[] = "../../factory/tools/en-US.b39";
    static FILE *bip39;

    if (!bip39) {
        bip39 = fopen(bip39_file_name, "rb");
        if (!bip39) {
            perror(bip39_file_name);
            abort();
        }
    }

    const uint32_t start_addr = xflash_num_blocks * 512 - XFLASH_WORDLIST_OFFSET;
    assert(address >= start_addr);
    assert(address + size <= start_addr + 2048 * 5);

    fseek(bip39, address - start_addr, SEEK_SET);
    fread(data, 1, size, bip39);
}
