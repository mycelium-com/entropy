/*
 * Memory Control Access service for updating firmware over USB.
 */

/**
 * \file
 *
 * Copyright (c) 2011 - 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/*
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

#include <string.h>
#include <sam4l.h>
#include <flashcalw.h>
#include "fw-access.h"

// Declarations for filesystem structures from automatically generated files.
extern unsigned char updfs_128_img[] __attribute__ ((aligned (4)));
extern unsigned int  updfs_128_img_len;
extern unsigned int  updfs_128_nblk;
extern unsigned char updfs_256_img[] __attribute__ ((aligned (4)));
extern unsigned int  updfs_256_img_len;
extern unsigned int  updfs_256_nblk;
extern unsigned char readme[] __attribute__ ((aligned (4)));

static unsigned num_sectors;
static unsigned first_blk;
typedef uint8_t uint8_t_aligned __attribute__ ((aligned (4)));
static uint8_t_aligned *fs_img;

// Setting this to 1 will disable flashing above 128 kB in recovery mode.
// This is only useful to reduce bootloader size, in case it grows larger
// than 16k.
#define SMALL_FLASH_ONLY 0

// This function tests memory state and starts memory initialisation.
Ctrl_status fw_test_unit_ready(void)
{
#if ! SMALL_FLASH_ONLY
    switch (CHIPID->CHIPID_CIDR & 0xf00) {
    case 0x700:                 // CHIPID_CIDR_NVPSIZ_128K
#endif
        num_sectors = updfs_128_nblk;
        fs_img = updfs_128_img;
        first_blk = (updfs_128_img_len >> 9) + 1;
#if ! SMALL_FLASH_ONLY
        break;

    case 0x900:                 // CHIPID_CIDR_NVPSIZ_256K
        num_sectors = updfs_256_nblk;
        fs_img = updfs_256_img;
        first_blk = (updfs_256_img_len >> 9) + 1;
        break;

    default:
        return CTRL_NO_PRESENT;
    }
#endif

#ifdef SPECIAL
#define PROTECTED 0
    flashcalw_lock_page_region(0, false);
    fs_img[((first_blk - 2) << 9) + 0x8b] = 0x20;
#else
#define PROTECTED 32
#endif
    ((uint32_t *)fs_img)[((first_blk - 2) << 7) + 23] =
        strlen((const char *)readme);

    return CTRL_GOOD;
}


// This function returns the address of the last valid sector
// (a sector is 512 bytes).
Ctrl_status fw_read_capacity(uint32_t *nb_sector)
{
    *nb_sector = num_sectors - 1;
    return CTRL_GOOD;
}


// This function returns the write-protected mode
// (true if the memory is read-only).
bool fw_wr_protect(void)
{
    return false;
}


// This function informs about the memory type.
// Returns true if the memory is removable.
bool fw_removal(void)
{
    return false;
}


// This function unloads/loads the memory.
// Returns true if the memory is unloaded.
bool fw_unload(bool unload)
{
    return false;
}

//------------ SPECIFIC FUNCTIONS FOR TRANSFER BY USB -------------------------

#if ACCESS_USB == true

#include "udi_msc.h"

//! This function transfers data to the USB MSC interface.
//! USB works with SRAM only so we need a buffer when the address is in flash.
//!
//! @param addr         Sector address to start read
//! @param nb_sector    Number of sectors to transfer (sector=512 bytes)
//!
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
//!
Ctrl_status fw_usb_read_10(uint32_t addr, uint16_t nb_sector)
{
    uint8_t buffer[SECTOR_SIZE];

    if (addr + nb_sector > num_sectors)
        return CTRL_FAIL;

    for (; nb_sector; --nb_sector, addr++) {
        uint8_t *blkptr;

        if (addr < first_blk - 1)
            blkptr = fs_img + addr * SECTOR_SIZE;
        else if (addr == first_blk - 1)
            blkptr = readme;
        else {
            memcpy(buffer, (const void *)((addr - first_blk) * SECTOR_SIZE), sizeof buffer);
            blkptr = buffer;
        }

        if (!udi_msc_trans_block(true, blkptr, SECTOR_SIZE, 0))
            return CTRL_FAIL;   // transfer aborted
    }

    return CTRL_GOOD;
}


//! This function transfers the USB MSC data to the memory
//! USB works with SRAM only so we need a buffer when the address is in flash.
//!
//! @param addr         Sector address to start write
//! @param nb_sector    Number of sectors to transfer (sector=512 bytes)
//!
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
//!
Ctrl_status fw_usb_write_10(uint32_t addr, uint16_t nb_sector)
{
    uint8_t buffer[SECTOR_SIZE];

    if (addr + nb_sector > num_sectors)
        return CTRL_FAIL;

    for (; nb_sector; --nb_sector, addr++) {
        if (!udi_msc_trans_block(false, buffer, SECTOR_SIZE, 0))
            return CTRL_FAIL;   // transfer aborted

        // ignore writes which do not go to flash
        if (addr >= first_blk + PROTECTED) {
            flashcalw_memcpy((volatile void *)((addr - first_blk) * SECTOR_SIZE),
                    buffer, SECTOR_SIZE, true);
            if (flashcalw_is_lock_error() || flashcalw_is_programming_error())
                return CTRL_FAIL;
        }
    }

    return CTRL_GOOD;
}

#endif  // ACCESS_USB == true


//------------ SPECIFIC FUNCTIONS FOR TRANSFER BY RAM --------------------------

#if ACCESS_MEM_TO_RAM == true

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

#endif  // ACCESS_MEM_TO_RAM == true
