/*
 * Memory Control Access service for reading firmware over USB.
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

#include <string.h>
#include <sam4l.h>
#include <flashcalw.h>

#include "xflash.h"
#include "fw-access.h"

// Declarations for filesystem structures from automatically generated files.
extern const unsigned char disk_img[] __attribute__ ((aligned (4)));
extern const unsigned int  disk_img_len;
extern const unsigned int  disk_nblk;

static uint32_t chip_flash_blk;
static uint32_t ext_flash_blk;

// This function tests memory state and starts memory initialisation.
Ctrl_status fw_test_unit_ready(void)
{
    chip_flash_blk = disk_img_len >> 9;
    ext_flash_blk = chip_flash_blk + 512;
    return CTRL_GOOD;
}

// This function returns the address of the last valid sector
// (a sector is 512 bytes).
Ctrl_status fw_read_capacity(uint32_t *nb_sector)
{
    *nb_sector = disk_nblk - 1;
    return CTRL_GOOD;
}

// This function unloads/loads the memory.
// Returns true if the memory is unloaded.
bool fw_unload(bool unload)
{
    return false;
}


/*
 * Read and write functions.
 *
 * Parameters:
 *   addr               - start sector number
 *   nb_sector          - number of sectors to transfer
 * (a sector is 512 bytes).
 *
 * Return values:
 *   CTRL_GOOD          - transfer successful
 *   CTRL_NO_PRESENT    - medium unloaded
 *   CTRL_BUSY          - medium has changed
 *   CTRL_FAIL          - an error occurred
 */

#if ACCESS_USB

// USB-specific access functions.

#include "udi_msc.h"

Ctrl_status fw_usb_read_10(uint32_t addr, uint16_t nb_sector)
{
    uint8_t buffer[SECTOR_SIZE];

    if (addr + nb_sector > disk_nblk)
        return CTRL_FAIL;

    for (; nb_sector; --nb_sector, addr++) {
        uint8_t *blkptr;

        if (addr < chip_flash_blk)
            blkptr = (uint8_t *) disk_img + addr * SECTOR_SIZE;
        else if (addr < ext_flash_blk) {
            uint32_t flash_addr = (addr - chip_flash_blk) * SECTOR_SIZE;
            if (flash_addr < flashcalw_get_flash_size())
                memcpy(buffer, (const void *) flash_addr, sizeof buffer);
            else
                memset(buffer, 0xff, sizeof buffer);
            blkptr = buffer;
        } else {
            uint32_t flash_addr =
                xflash_num_blocks * 512 - XFLASH_WORDLIST_OFFSET +
                (addr - ext_flash_blk) * SECTOR_SIZE;
            if (!xflash_read(buffer, sizeof buffer, flash_addr))
                memset(buffer, 0xff, sizeof buffer);
            blkptr = buffer;
        }

        if (!udi_msc_trans_block(true, blkptr, SECTOR_SIZE, 0))
            return CTRL_FAIL;   // transfer aborted
    }

    return CTRL_GOOD;
}

Ctrl_status fw_usb_write_10(uint32_t addr, uint16_t nb_sector)
{
    return CTRL_FAIL;
}

#endif


//------------ SPECIFIC FUNCTIONS FOR TRANSFER BY RAM --------------------------

#if ACCESS_MEM_TO_RAM

#include <string.h>

//! This function transfers 1 data sector from memory to RAM.
//! sector = 512 bytes
//! @param addr         Sector address to start read
//! @param ram          Address of RAM buffer
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status fw_mem_2_ram(uint32_t addr, void *ram)
{
    return CTRL_FAIL;
}


//! This function transfers 1 data sector from RAM to memory.
//! sector = 512 bytes
//! @param addr         Sector address to start write
//! @param ram          Address of RAM buffer
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status fw_ram_2_mem(uint32_t addr, const void *ram)
{
    return CTRL_FAIL;
}

#endif
