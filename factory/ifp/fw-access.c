/*
 * Memory Control Access service for initial flash programming over USB.
 *
 * Both the on-chip flash, including the bootloader, and external serial flash
 * are represented by a raw block device with no filesystem or partitions.
 * The serial flash immediately follows the on-chip flash.
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
#include <at25dfx.h>
#include "fw-access.h"

static unsigned num_sectors;        // total number of sectors in both flashes
static unsigned int_flash_size;     // number of sectors in the internal flash
unsigned ext_flash_size;            // number of sectors in the serial flash
uint32_t magic_sector_addr;         // address of the magic sector
uint32_t ext_flash_start_addr;      // start address of data in external flash
uint32_t checksum_state;            // state of the checksum FSM

// Serial flash parameters and state.
enum {
    FLASH_BLOCK_SIZE = 65536,
    FLASH_MAX_SIZE   = 512 * 1024,
};
static uint32_t last_erased = 0x80000000u;

// This function tests memory state and starts memory initialisation.
Ctrl_status fw_test_unit_ready(void)
{
    switch (CHIPID->CHIPID_CIDR & 0xf00) {
    case 0x700:                 // CHIPID_CIDR_NVPSIZ_128K
        int_flash_size = 256;   // in sectors of 512 bytes
        break;

    case 0x900:                 // CHIPID_CIDR_NVPSIZ_256K
        int_flash_size = 512;   // in sectors of 512 bytes
        break;

    default:
        return CTRL_NO_PRESENT;
    }

    num_sectors = int_flash_size + ext_flash_size;

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
    return true;
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
        if (addr < int_flash_size) {
            // read internal flash:
            // USB cannot transfer from flash directly, so we copy to RAM first
            memcpy(buffer, (const void *) (addr * SECTOR_SIZE), sizeof buffer);
        } else {
            // read external serial flash
            if (at25dfx_read(buffer, SECTOR_SIZE,
                        (addr - int_flash_size) * SECTOR_SIZE) != AT25_SUCCESS)
                return CTRL_FAIL;
        }

        if (!udi_msc_trans_block(true, buffer, SECTOR_SIZE, 0))
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
    union {
        uint8_t bytes[SECTOR_SIZE];
        const struct {
            char     magic[32];
            uint32_t hash[8];
            uint32_t start;
        };
    } buffer;

    if (addr + nb_sector > num_sectors)
        return CTRL_FAIL;

    for (; nb_sector; --nb_sector, addr++) {
        if (!udi_msc_trans_block(false, buffer.bytes, SECTOR_SIZE, 0))
            return CTRL_FAIL;   // transfer aborted

        if (addr < int_flash_size) {
            // write to internal flash
            flashcalw_memcpy((volatile void *) (addr * SECTOR_SIZE),
                    buffer.bytes, SECTOR_SIZE, true);
            if (flashcalw_is_lock_error() || flashcalw_is_programming_error())
                return CTRL_FAIL;
        } else {
            // write to external serial flash
            uint32_t flash_addr = (addr - int_flash_size) * SECTOR_SIZE;

            if (flash_addr < last_erased
             || flash_addr >= last_erased + FLASH_BLOCK_SIZE) {
                last_erased = flash_addr & ~(FLASH_BLOCK_SIZE - 1);
                if (at25dfx_erase_block(last_erased, AT25_BLOCK_ERASE_64K)
                        != AT25_SUCCESS)
                    return CTRL_FAIL;
            }
            if (at25dfx_write(buffer.bytes, SECTOR_SIZE, flash_addr)
                    != AT25_SUCCESS)
                return CTRL_FAIL;
            // check for magic block
            if (strcmp(buffer.magic, "End of data in the filesystem.") == 0) {
                magic_sector_addr = flash_addr;
                if (buffer.start < flash_addr)
                    ext_flash_start_addr = buffer.start;
            }
        }
    }

    // Any successful write restarts the checksum FSM from scratch.
    checksum_state = ext_flash_start_addr;

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
