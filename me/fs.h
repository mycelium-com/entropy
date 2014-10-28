/*
 * FAT filesystem initialisation and FatFs access LUNs.
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

#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

struct FS_init_param {
    uint8_t  lun;       // physical device LUN
    bool     fdisk;     // whether to make a partitioned volume
    uint8_t  clu_sect;  // cluster size in sectors (a power of 2)
    uint8_t  align;     // data area alignment in sectors
    char     label[11]; // volume label padded with spaces and without NUL
    // device access functions:
    const uint8_t * (*read_ptr) (unsigned secno);
    uint8_t * (*write_ptr) (unsigned secno);
};

unsigned fs_init(const struct FS_init_param *param, unsigned size, bool force);

// Our disk I/O backend for FATFS does not use the ctrl_access mechanism.
// The reason is that we need to use different memories for USB and for FATFS.
// Therefore we use our own LUNs, independent of ctrl_access.
enum {
    FS_LUN_XFLASH,  // external flash (cached when in configuration mode)
    FS_LUN_STREAM,  // virtual device for streaming JPEG
};

// Address of the JPEG stream buffer.
extern uint8_t *fs_stream_buf;

#endif
