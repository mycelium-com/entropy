/*
 * Memory Control Access interface for initial flash programming over USB.
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

#ifndef FW_ACCESS_H
#define FW_ACCESS_H

#include "conf_access.h"
#include <ctrl_access.h>

// Number of 512-byte sectors in the serial flash.
// Should be filled by the application before USB stack is started.
extern unsigned ext_flash_size;

// Address of the magic sector in the serial flash.
// Filled by USB access functions when a magic sector is written to flash.
extern uint32_t magic_sector_addr;

// State of the checksum FSM.
// USB access functions clear this on any successful write transaction.
// The state is otherwise used and controlled by the FSM in main.c.
extern uint32_t checksum_state;

// Tests the memory state and initialises the memory if required.
// This command may be used to check the media status of LUNs with removable
// media.
Ctrl_status fw_test_unit_ready(void);

// Returns the address of the last valid sector in the memory.
Ctrl_status fw_read_capacity(uint32_t *nb_sector);

// Returns the write-protected state
// (true if the memory is read-only).
bool fw_wr_protect(void);

// Tells whether the memory is removable.
bool fw_removal(void);

// Unload/load the memory.
bool fw_unload(bool unload);

#if ACCESS_USB == true

// Transfers data to USB.
//
// addr      Address of first memory sector to read.
// nb_sector Number of sectors to transfer.
Ctrl_status fw_usb_read_10(uint32_t addr, uint16_t nb_sector);

// Transfers data from USB.
//
// addr      Address of first memory sector to write.
// nb_sector Number of sectors to transfer.
Ctrl_status fw_usb_write_10(uint32_t addr, uint16_t nb_sector);

#endif

#if ACCESS_MEM_TO_RAM == true

// Copies 1 data sector to RAM.
//
// addr  Address of first memory sector to read.
// ram   Pointer to RAM buffer to write.
Ctrl_status fw_mem_2_ram(uint32_t addr, void *ram);

// Copies 1 data sector from RAM.
//
// addr  Address of first memory sector to write.
// ram   Pointer to RAM buffer to read.
Ctrl_status fw_ram_2_mem(uint32_t addr, const void *ram);

#endif

#endif
