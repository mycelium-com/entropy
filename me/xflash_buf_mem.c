/*
 * Memory Control Access (CTRL_ACCESS) interface for the external flash
 * in read-write mode using block buffers and cache.
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

#include "xflash_buf_mem.h"

#if XFLASH_BUF_MEM == ENABLE

#include <string.h>
#include "xflash.h"
#include "blkbuf.h"
#include "fs.h"
#include "main.h"


Ctrl_status xfb_test_unit_ready(void)
{
    // R/W access to serial flash through this interface is for configuration
    // (firmware update) mode only.
    if (!configuration_mode || !xflash_4k_erase_capable) {
        return CTRL_NO_PRESENT;
    }
    return CTRL_GOOD;
}

Ctrl_status xfb_read_capacity(uint32_t *nb_sector)
{
    *nb_sector = xflash_num_blocks - 1;
    return CTRL_GOOD;
}


#if ACCESS_USB

#include "udi_msc.h"

// MEM <-> USB Interface

Ctrl_status xfb_usb_read_10(uint32_t addr, uint16_t nb_sector)
{
    if (addr + nb_sector > xflash_num_blocks)
        return CTRL_FAIL;

    for (; nb_sector; --nb_sector) {
        const uint8_t *blkptr = blkbuf_read_ptr(addr++);
        if (!udi_msc_trans_block(true, (uint8_t *)blkptr, SECTOR_SIZE, 0))
            return CTRL_FAIL;
    }
    return CTRL_GOOD;
}

Ctrl_status xfb_usb_write_10(uint32_t addr, uint16_t nb_sector)
{
    if (addr + nb_sector > xflash_num_blocks - XFLASH_RESERVED_BLOCKS)
        return CTRL_FAIL;

    for (; nb_sector; --nb_sector) {
        uint8_t *blkptr = blkbuf_write_ptr(addr++);
        if (!udi_msc_trans_block(false, blkptr, SECTOR_SIZE, 0))
            return CTRL_FAIL;   // transfer aborted
    }

    return CTRL_GOOD;
}

#endif  // ACCESS_USB


#if ACCESS_MEM_TO_RAM

// MEM <-> RAM Interface

Ctrl_status xfb_mem_2_ram(uint32_t addr, void *ram)
{
    if (addr >= xflash_num_blocks)
        return CTRL_FAIL;
    memcpy(ram, blkbuf_read_ptr(addr), SECTOR_SIZE);
    return CTRL_GOOD;
}

Ctrl_status xfb_ram_2_mem(uint32_t addr, const void *ram)
{
    if (addr >= xflash_num_blocks)
        return CTRL_FAIL;

    memcpy(blkbuf_write_ptr(addr), ram, SECTOR_SIZE);
    return CTRL_GOOD;
}

#endif  // ACCESS_MEM_TO_RAM

#endif
