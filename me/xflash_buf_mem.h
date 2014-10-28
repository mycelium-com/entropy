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

#ifndef XFLASH_BUF_MEM_H
#define XFLASH_BUF_MEM_H

#include <ctrl_access.h>

// Tests the memory state and initialises the memory if required.
// This command may be used to check the media status of LUNs with removable
// media.
Ctrl_status xfb_test_unit_ready(void);

// Returns the address of the last valid sector in the memory.
Ctrl_status xfb_read_capacity(uint32_t *nb_sector);

#if ACCESS_USB

// Transfers data to USB.
//
// addr      Address of first memory sector to read.
// nb_sector Number of sectors to transfer.
Ctrl_status xfb_usb_read_10(uint32_t addr, uint16_t nb_sector);

// Transfers data from USB.
//
// addr      Address of first memory sector to write.
// nb_sector Number of sectors to transfer.
Ctrl_status xfb_usb_write_10(uint32_t addr, uint16_t nb_sector);

#endif

#if ACCESS_MEM_TO_RAM

// Copies 1 data sector to RAM.
//
// addr  Address of first memory sector to read.
// ram   Pointer to RAM buffer to write.
Ctrl_status xfb_mem_2_ram(uint32_t addr, void *ram);

// Copies 1 data sector from RAM.
//
// addr  Address of first memory sector to write.
// ram   Pointer to RAM buffer to read.
Ctrl_status xfb_ram_2_mem(uint32_t addr, const void *ram);

#endif

#endif
