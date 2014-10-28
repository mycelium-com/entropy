/*
 * Block buffering for serial flash.
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

#ifndef BLKBUF_H_INCLUDED
#define BLKBUF_H_INCLUDED

#include <stdint.h>

// Set up an array of 4 kB block buffers on heap to conduct 512-byte R/W access
// to a serial flash with the minimum erasable sector size of 4 kB.
void blkbuf_init(void);

// Read a 512 byte sector.
const uint8_t * blkbuf_read_ptr(unsigned secno);

// Prepare a 512 byte sector for writing.
// The pointer returned points to the requested sector's content inside the
// buffer.  It can be modified there by the caller.
// The sector is marked as dirty, so it will eventually be committed to flash,
// either when the buffer is evicted or via blkbuf_flush().
uint8_t * blkbuf_write_ptr(unsigned secno);

// Write dirty buffers to flash.
void blkbuf_flush(void);

#endif
