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

#ifndef FW_ACCESS_H
#define FW_ACCESS_H

#include "conf_access.h"
#include <ctrl_access.h>

// Tests the memory state and initialises the memory if required.
// This command may be used to check the media status of LUNs with removable
// media.
Ctrl_status fw_test_unit_ready(void);

// Returns the address of the last valid sector in the memory.
Ctrl_status fw_read_capacity(uint32_t *nb_sector);

// Unload/load the memory.
bool fw_unload(bool unload);

#if ACCESS_USB

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

#endif
