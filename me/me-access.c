/*
 * Memory Control Access service for the streaming on-demand JPEG generator.
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

#include <string.h>
#include "me-access.h"


static Ctrl_status unit_status = CTRL_GOOD;


// This function tests memory state and starts memory initialisation.
Ctrl_status me_test_unit_ready(void)
{
    Ctrl_status status = unit_status;
    if (unit_status == CTRL_BUSY)
        unit_status = CTRL_GOOD;
    return status;
}

// This function returns the address of the last valid sector
// (a sector is 512 bytes).
Ctrl_status me_read_capacity(uint32_t *nb_sector)
{
    *nb_sector = cbd_num_sectors - 1;
    return me_test_unit_ready();
}

// This function unloads/loads the memory.
// Returns true if the memory is unloaded.
bool me_unload(bool unload)
{
    if (unload) {
        unit_status = CTRL_NO_PRESENT;
        return true;
    }
    // requested to load medium;
    // next status report should return CTRL_BUSY after a medium change
    if (unit_status == CTRL_NO_PRESENT)
        unit_status = CTRL_BUSY;
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

Ctrl_status me_usb_read_10(uint32_t addr, uint16_t nb_sector)
{
    if (unit_status != CTRL_GOOD)
        return unit_status;

    if (addr + nb_sector > cbd_num_sectors)
        return CTRL_FAIL;

    const struct CBD_map_entry * entry = cbd_map;

    while (addr >= entry->size)
        addr -= entry++->size;

    for (; nb_sector; --nb_sector) {
        uint8_t *blkptr = entry->get_block(addr++);
        // udi_msc_trans_block() is limited to 64KB
        if (!udi_msc_trans_block(true, blkptr, SECTOR_SIZE, 0))
            return CTRL_FAIL;   // transfer aborted
        if (addr >= entry->size)
            addr -= entry++->size;
    }
    if (entry->prefetchable)
        entry->get_block(addr); // prefetch

    return CTRL_GOOD;
}

Ctrl_status me_usb_write_10(uint32_t addr, uint16_t nb_sector)
{
    return CTRL_FAIL;
}

#endif  // ACCESS_USB


#if ACCESS_MEM_TO_RAM

// RAM <-> device access functions.
// All transfers are single sector.

#include <string.h>

Ctrl_status me_mem_2_ram(uint32_t addr, void *ram)
{
    if (addr >= cbd_num_sectors)
        return CTRL_FAIL;

    const struct CBD_map_entry * entry = cbd_map;

    while (addr >= entry->size)
        addr -= entry++->size;

    memcpy(ram, entry->get_block(addr), SECTOR_SIZE);

    return me_test_unit_ready();
}

Ctrl_status me_ram_2_mem(uint32_t addr, const void *ram)
{
    return CTRL_FAIL;
}

#endif  // ACCESS_MEM_TO_RAM
