/**
 * \file
 *
 * \brief CTRL_ACCESS interface for the AT25DFX serial flash.
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


//_____  I N C L U D E S ___________________________________________________

#include "conf_access.h"


#if AT25DFX_MEM == ENABLE

#include "conf_at25dfx.h"
#include "at25dfx.h"
#include "at25dfx_mem.h"
#include "xflash.h"

#define AT25DFX_NSECT       xflash_num_blocks


//_____ D E F I N I T I O N S ______________________________________________

/*! \name Control Interface
 */
//! @{

enum {
    FLASH_BLOCK_SIZE = 65536,
    FLASH_MAX_SIZE   = 2048 * 1024,
};

static bool b_at25dfx_unloaded = false;

static bool erased[FLASH_MAX_SIZE / FLASH_BLOCK_SIZE];

Ctrl_status at25dfx_test_unit_ready(void)
{
	if (b_at25dfx_unloaded) {
		return CTRL_NO_PRESENT;
	}
	return CTRL_GOOD;
}


Ctrl_status at25dfx_read_capacity(U32 *u32_nb_sector)
{
	*u32_nb_sector = AT25DFX_NSECT - 1;
	return CTRL_GOOD;
}


bool at25dfx_unload(bool unload)
{
	b_at25dfx_unloaded = unload;
    return false;
}


//! @}


#if ACCESS_USB == true

#include "udi_msc.h"

//! Sector buffer.
//static uint8_t sector_buf[SECTOR_SIZE];
extern uint8_t _estack[SECTOR_SIZE];
#define sector_buf _estack

/*! \name MEM <-> USB Interface
 */
//! @{


Ctrl_status at25dfx_usb_read_10(U32 addr, U16 nb_sector)
{
	if (addr + nb_sector > AT25DFX_NSECT) {
		return CTRL_FAIL;
	}
	while (nb_sector--) {
		// Read the next sector.
		at25dfx_read(sector_buf, SECTOR_SIZE, addr * SECTOR_SIZE);
		if (!udi_msc_trans_block(true, sector_buf, SECTOR_SIZE, NULL))
            return CTRL_FAIL;
        addr++;
	}
	return CTRL_GOOD;
}

Ctrl_status at25dfx_usb_write_10(U32 addr, U16 nb_sector)
{
    if (addr + nb_sector > AT25DFX_NSECT) {
        return CTRL_FAIL;
    }
    while (nb_sector--) {
        unsigned flash_addr = addr * SECTOR_SIZE;

		if (!udi_msc_trans_block(false, sector_buf, SECTOR_SIZE, NULL))
            return CTRL_FAIL;

        if (!erased[flash_addr / FLASH_BLOCK_SIZE]) {
            if (at25dfx_erase_block(flash_addr, AT25_BLOCK_ERASE_64K) != AT25_SUCCESS)
                return CTRL_FAIL;
            erased[flash_addr / FLASH_BLOCK_SIZE] = true;
        }
        if (at25dfx_write(sector_buf, SECTOR_SIZE, flash_addr) != AT25_SUCCESS)
            return CTRL_FAIL;
        addr++;
	}
    return CTRL_GOOD;
}

//! @}

#endif  // ACCESS_USB == true


#if ACCESS_MEM_TO_RAM == true

/*! \name MEM <-> RAM Interface
 */
//! @{


Ctrl_status at25dfx_df_2_ram(U32 addr, void *ram)
{
	if (addr + 1 > AT25DFX_NSECT){
		return CTRL_FAIL;
	}
	at25dfx_read(ram, SECTOR_SIZE, addr * SECTOR_SIZE);
	return CTRL_GOOD;
}


Ctrl_status at25dfx_ram_2_df(U32 addr, const void *ram)
{
	if (addr + 1 > AT25DFX_NSECT) {
		return CTRL_FAIL;
	}

	at25dfx_write(ram, SECTOR_SIZE, addr * SECTOR_SIZE);
	return CTRL_GOOD;
}


//! @}

#endif  // ACCESS_MEM_TO_RAM == true


#endif  // AT25DFX_MEM == ENABLE
