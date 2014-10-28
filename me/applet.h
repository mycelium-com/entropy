/*
 * RAM-loadable helper applet.
 * This applet loads image from serial flash into either RAM or on-chip flash,
 * depending on the image type.
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

#ifndef APPLET_H_INCLUDED
#define APPLET_H_INCLUDED

#include <stdint.h>

struct Applet_param {
    uint32_t fat_addr;      // FAT start address in bytes
    uint32_t data_addr;     // data start address in bytes
    uint16_t csize;         // bytes per cluster
    uint16_t file_scl;      // file start cluster
    uint32_t file_size;     // file size in bytes
    uint32_t ftype;         // file type (magic)
};

extern const uintptr_t applet_addr;
extern const uintptr_t applet_param_addr;
extern const unsigned  applet_max_task;
extern const unsigned  applet_bin_len;
extern const unsigned char applet_bin[];

#endif
