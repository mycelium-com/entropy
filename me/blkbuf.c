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

#include <limits.h>
#include "xflash.h"
#include "blkbuf.h"

enum {
    BLKSIZE = 4096
};

// Useful derived constants
enum {
    SECT_IN_BLK = BLKSIZE / 512,
    ALL_DIRTY   = (1 << SECT_IN_BLK) - 1,   // all sectors in block are dirty
};

// Block buffer
struct Block {
    uint16_t      blkno;    // number of this block
    uint16_t      dirty;    // bitmask of dirty sectors
    int32_t       afreq;    // indicator of access frequency
    uint8_t       buf[SECT_IN_BLK][512];
};

extern struct Block _estack[];          // start of buffer memory
extern struct Block __ram_end__[];      // end of buffer memory

static struct Block *blocks;
static struct Block *end_of_blocks;

void blkbuf_init()
{
    struct Block *blk = _estack;

    blocks = blk;

    do {
        blk->blkno = (uint16_t)~0;      // non-existing block
        blk->dirty = 0;
        blk->afreq = 0;
    } while (++blk + 1 <= __ram_end__);

    end_of_blocks = blk;
}

static void evict(struct Block *blk)
{
    if (blk->dirty) {
        uint32_t address = blk->blkno * BLKSIZE;
        xflash_erase_4k(address);
        xflash_write(blk->buf[0], BLKSIZE, address);
        blk->dirty = 0;
    }
}

static struct Block * find_block(unsigned blkno)
{
    int eviction_score = INT_MIN;
    struct Block *eviction_blk = blocks;
    struct Block *blk = blocks;
    int afreq = 0;

    do {
        afreq++;
        if (blk->blkno == blkno) {
            blk->afreq += afreq;
            blocks = blk;
            return blk;
        }
        --blk->afreq;

        // eviction candidate ranking:
        //  - highest: blocks where all sectors are dirty
        //  - medium: clean blocks
        //  - lowest: partially dirty blocks
        // and in each group, lower afreq gives higher eviction score
        int this_score = ((blk->dirty == ALL_DIRTY) << 6)
                       + ((blk->dirty == 0) << 5)
                       - blk->afreq;
        if (this_score > eviction_score) {
            eviction_score = this_score;
            eviction_blk = blk;
        }

        if (++blk == end_of_blocks)
            blk = _estack;
    } while (blk != blocks);

    // no cached buffer found
    evict(eviction_blk);
    xflash_read(eviction_blk->buf[0], BLKSIZE, blkno * BLKSIZE);
    eviction_blk->blkno = blkno;
    eviction_blk->afreq = 0;
    blocks = eviction_blk;
    return eviction_blk;
}

// Make sure the requested sector is loaded into memory
// and return its address
const uint8_t * blkbuf_read_ptr(unsigned secno)
{
    struct Block *blk = find_block(secno / SECT_IN_BLK);
    return blk->buf[secno % SECT_IN_BLK];
}

// Return buffer address where the requested sector should be written to
uint8_t * blkbuf_write_ptr(unsigned secno)
{
    struct Block *blk = find_block(secno / SECT_IN_BLK);
    secno %= SECT_IN_BLK;
    blk->dirty |= 1 << secno;
    return blk->buf[secno];
}

void blkbuf_flush(void)
{
    struct Block *blk;

    for (blk = _estack; blk != end_of_blocks; blk++)
        evict(blk);
}
