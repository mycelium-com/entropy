/*
 * FAT filesystem initialisation.
 *
 * Volume partitioning and file system initialisation function fs_init() is
 * provided as a replacement for f_mkfs, which is not flexible enough.  This
 * works for FAT12 volumes not bigger than 65535 sectors only.
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

#include <stdio.h>
#include <string.h>
#include <ff.h>

#include "blkbuf.h"
#include "fs.h"


// DOS date and time (endian-independent bit fields)
#define DOS_DATE(d,m,y)     (d) | ((m) & 7) << 5, (m) >> 3 | ((y) - 1980) << 1
#define DOS_TIME(h,m,s)     (s) >> 1 | ((m) & 7) << 5, (m) >> 3 | (h) << 3

// Initialise a FAT filesystem on target device, if it doesn't exist or if
// the 'force' parameter is set.
// Return the number of 512-blocks occupied by partition table (if requested)
// and filesystem structures.
// Return zero if the filesystem was already present.
unsigned fs_init(const struct FS_init_param *param, unsigned size, bool force)
{
    unsigned secno = 0;

    if (!force) {
        // check boot sector signature
        const uint8_t *first_block = param->read_ptr(secno);
        if (first_block[510] == 0x55 && first_block[511] == 0xAA)
            return secno;       // already formatted
    }

    // create partition table with one partition

    if (param->fdisk) {
        struct {
            uint8_t bootcode[446];
            struct {
                uint8_t active;
                uint8_t first_head;
                uint8_t first_seccyl;
                uint8_t first_cyl;
                uint8_t ptype;
                uint8_t last_head;
                uint8_t last_seccyl;
                uint8_t last_cyl;
                uint8_t lba[4];
                uint8_t size[4];
            } part[4];
            uint8_t sig[2];
        } *mbr = (void *)param->write_ptr(secno++);

        _Static_assert (sizeof *mbr == 512, "Bad MBR structure alignment.");

        --size;                     // FAT partition size is 1 block less

        memset(mbr, 0, sizeof *mbr);

        mbr->bootcode[0] = 0xEB;    // infinite loop
        mbr->bootcode[1] = 0xFE;

        mbr->part[0].first_head = 1;
        mbr->part[0].first_seccyl = 1;
        mbr->part[0].first_cyl = 0;
        mbr->part[0].ptype = 1;     // FAT12 (if in first 32 MB of disk)
        mbr->part[0].last_head = size & 127;
        mbr->part[0].last_seccyl = 1 | (size >> 9 & 0xC0);
        mbr->part[0].last_cyl = size >> 7;
        mbr->part[0].lba[0] = 1;                // partition starts at LBA 1
        ST_DWORD(mbr->part[0].size, size);      // partition size

        mbr->sig[0] = 0x55;
        mbr->sig[1] = 0xAA;
    }

    // make FAT boot sector
    struct FAT_boot_sector {
        uint8_t jump[3];
        char    oem_name[8];
        uint8_t bytes_per_sector[2];
        uint8_t sectors_per_cluster;
        uint8_t reserved_sectors[2];    // before FAT, including boot sector
        uint8_t num_fats;
        uint8_t num_root_entries[2];
        uint8_t num_sectors[2];         // if <= 65535
        uint8_t media_descriptor;       // 0xF0 floppy, 0xF8 hard disk
        uint8_t sectors_per_fat[2];
        uint8_t sectors_per_track[2];   // part of obsolete CHS geometry
        uint8_t num_heads[2];           // part of obsolete CHS geometry
        uint8_t num_hidden_sectors[4];  // sectors on disk before this one
        uint8_t large_num_sectors[4];   // not used (volume >= 65536 sectors)
        uint8_t phys_drive_no;          // 0x00 floppy, 0x80 hard disk
        uint8_t reserved;
        uint8_t ext_boot_signature;     // 0x28 or 0x29
        uint8_t vol_serial_number[4];
        uint8_t vol_label[11];
        uint8_t fs_marker[8];           // "FAT12   "
    };
    static const struct FAT_boot_sector template = {
        .jump                   = { 0xEB, 0xFE, 0x90 },     // infinite loop
        .oem_name               = "MYCELIUM",
        .bytes_per_sector       = { 0, 2 },                 // 512
        .sectors_per_cluster    = 1,
        .reserved_sectors       = { 1, 0 },
        .num_fats               = 1,
        .num_root_entries       = { 32, 0 },
        .media_descriptor       = 0xF0,
        .sectors_per_fat        = { 1, 0 },
        .sectors_per_track      = { 1, 0 },
        .num_heads              = { 128, 0 },
        .ext_boot_signature     = 0x29,
        .vol_label              = "MYCELIUM   ",
        .fs_marker              = "FAT12   ",
    };
    struct FAT_boot_sector *fbs = (void *)param->write_ptr(secno++);

    memset(fbs, 0, 512);
    memcpy(fbs, &template, sizeof template);

    ST_WORD(fbs->num_sectors, size);
    fbs->sectors_per_cluster = param->clu_sect;
    memcpy(fbs->vol_serial_number, (const uint8_t *)0x80020C, 4);
    memcpy(fbs->vol_label, param->label, sizeof fbs->vol_label);
    ((uint8_t *)fbs)[510] = 0x55;
    ((uint8_t *)fbs)[511] = 0xAA;

    if (param->fdisk) {
        // treat this as a hard disk rather than a floppy
        fbs->media_descriptor = 0xF8;
        fbs->phys_drive_no = 0x80;
        fbs->num_hidden_sectors[0] = 1;     // this is for MBR
    }

    // compute size of a FAT in sectors
    unsigned fat_size = (size * 3 / param->clu_sect + 1) / 2 + 3;   // in bytes
    fat_size = (fat_size + 511) / 512;                  // convert to sectors
    // expand FAT if necessary for alignment
    fat_size += -(secno + fat_size + 2) & (param->align - 1);

    fbs->sectors_per_fat[0] = fat_size;     // we'll never exceed 255 sectors

    // we need the media descriptor byte to initialise the FAT, but fbs can in
    // theory get invalidated by the next call to param->..._ptr()
    uint8_t media_descriptor = fbs->media_descriptor;

    // initialise FAT and root directory
    unsigned i;
    for (i = 0; i != fat_size + 2; i++) {
        uint8_t *blk = param->write_ptr(secno++);
        memset(blk, 0, 512);

        if (i == 0) {
            // first block of the FAT: reserve clusters 0 and 1
            blk[0] = media_descriptor;
            blk[1] = 0xFF;
            blk[2] = 0xFF;
        } else if (i == fat_size) {
            // first block of the root directory: enter volume label
            static const uint8_t direntry[] = {
                // first 11 bytes are volume name, which comes from param
                0x08,       // attributes: volume label
                0,          // reserved
                0,          // creation time: tenths of second
                DOS_TIME(BUILD_HOUR, BUILD_MIN, BUILD_SEC),
                DOS_DATE(BUILD_DAY, BUILD_MONTH, BUILD_YEAR),
                DOS_DATE(BUILD_DAY, BUILD_MONTH, BUILD_YEAR),
                0, 0,       // always zero for FAT-12/16
                DOS_TIME(BUILD_HOUR, BUILD_MIN, BUILD_SEC),
                DOS_DATE(BUILD_DAY, BUILD_MONTH, BUILD_YEAR),
                // cluster number and file size are left as zeros
            };
            memcpy(blk, param->label, 11);
            memcpy(blk + 11, direntry, sizeof direntry);
        }
    }

    return secno;
}

uint8_t *fs_stream_buf;
