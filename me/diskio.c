/*
 * Implementation of low level disk I/O module skeleton for FatFS.
 *
 * This version is alternative to the standard ASF implementation.
 * This FatFs disk I/O backend connects to different types of storage than USB
 * and therefore does not use ctrl_access.
 * It uses its own LUNs, which can address:
 *  - serial flash: directly in normal mode (RO) or through blkbuf in
 *  configuration mode (RW);
 *  - in-memory buffer for the JPEG streaming volume.
 */

/**
 * \file
 *
 * \brief Implementation of low level disk I/O module skeleton for FatFS.
 *
 * Copyright (c) 2012 - 2014 Atmel Corporation. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <compiler.h>
#include <diskio.h>
#include <ctrl_access.h>

#include "xflash.h"
#include "blkbuf.h"
#include "fs.h"
#include "main.h"

/**
 * \defgroup thirdparty_fatfs_port_group Port of low level driver for FatFS
 *
 * Low level driver for FatFS.
 *
 * @{
 */

// Sector size is always 512, as defined by SECTOR_SIZE in ctrl_access.h.

/**
 * \brief Initialize a disk.
 *
 * \param drv Physical drive number (0..).
 *
 * \return 0 or disk status in combination of DSTATUS bits
 *         (STA_NOINIT, STA_PROTECT).
 *
 * Actual initialisation is done elsewhere; we can simply return disk status.
 */
DSTATUS disk_initialize(BYTE drv) __attribute__ ((alias ("disk_status")));

/**
 * \brief  Return disk status.
 *
 * \param drv Physical drive number (0..).
 *
 * \return 0 or disk status in combination of DSTATUS bits
 *         (STA_NOINIT, STA_NODISK, STA_PROTECT).
 */
DSTATUS disk_status(BYTE drv)
{
    switch (drv) {
    case FS_LUN_XFLASH:
        return configuration_mode && xflash_4k_erase_capable ? 0 : STA_PROTECT;
    case FS_LUN_STREAM:
        return 0;
    }

    return STA_NOINIT;
}

/**
 * \brief  Read sector(s).
 *
 * \param drv Physical drive number (0..).
 * \param buff Data buffer to store read data.
 * \param sector Sector address (LBA).
 * \param count Number of sectors to read (1..255).
 *
 * \return RES_OK for success, otherwise DRESULT error code.
 */
DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, BYTE count)
{
    switch (drv) {
    case FS_LUN_XFLASH:
        if (sector + count > xflash_num_blocks)
            return RES_PARERR;
        /* Read the data */
        if (configuration_mode && xflash_4k_erase_capable) {
            unsigned i;
            for (i = 0; i < count; i++)
                memcpy(buff + i * SECTOR_SIZE, blkbuf_read_ptr(sector + i),
                        SECTOR_SIZE);
        } else {
            xflash_read(buff, count * SECTOR_SIZE, sector * SECTOR_SIZE);
        }
        break;

    case FS_LUN_STREAM:
        if (sector + count > 32) {
            printf("disk_read: LUN_STREAM: sector %lu, count %u\n",
                    sector, count);
            break;
        }
        memcpy(buff, fs_stream_buf + sector * SECTOR_SIZE,
                count * SECTOR_SIZE);
        break;

    default:
        return RES_PARERR;
    }

    return RES_OK;
}

/**
 * \brief  Write sector(s).
 *
 * The FatFs module will issue multiple sector transfer request (count > 1) to
 * the disk I/O layer. The disk function should process the multiple sector
 * transfer properly. Do not translate it into multiple sector transfers to the
 * media, or the data read/write performance may be drastically decreased.
 *
 * \param drv Physical drive number (0..).
 * \param buff Data buffer to store read data.
 * \param sector Sector address (LBA).
 * \param count Number of sectors to read (1..255).
 *
 * \return RES_OK for success, otherwise DRESULT error code.
 */
#if _READONLY == 0
DRESULT disk_write(BYTE drv, BYTE const *buff, DWORD sector, BYTE count)
{
    switch (drv) {
    case FS_LUN_XFLASH:
        if (sector + count > xflash_num_blocks)
            return RES_PARERR;
        /* Write the data */
        if (configuration_mode && xflash_4k_erase_capable) {
            unsigned i;
            for (i = 0; i < count; i++)
                memcpy(blkbuf_write_ptr(sector + i), buff + i * SECTOR_SIZE,
                        SECTOR_SIZE);
        } else {
            xflash_write(buff, count * SECTOR_SIZE, sector * SECTOR_SIZE);
        }
        break;

    case FS_LUN_STREAM:
        if (sector + count > 32) {
            printf("disk_write: LUN_STREAM: sector %lu, count %u\n",
                    sector, count);
            break;
        }
        memcpy(fs_stream_buf + sector * SECTOR_SIZE, buff,
                count * SECTOR_SIZE);
        break;

    default:
        return RES_PARERR;
    }

    return RES_OK;
}
#endif /* _READONLY */

/**
 * \brief  Miscellaneous functions, which support the following commands:
 *
 * CTRL_SYNC    Make sure that the disk drive has finished pending write
 * process. When the disk I/O module has a write back cache, flush the
 * dirty sector immediately.
 * In read-only configuration, this command is not needed.
 *
 * GET_SECTOR_COUNT    Return total sectors on the drive into the DWORD variable
 * pointed by buffer.
 * This command is used only in f_mkfs function.
 *
 * GET_BLOCK_SIZE    Return erase block size of the memory array in unit
 * of sector into the DWORD variable pointed by Buffer.
 * When the erase block size is unknown or magnetic disk device, return 1.
 * This command is used only in f_mkfs function.
 *
 * GET_SECTOR_SIZE    Return sector size of the memory array.
 *
 * \param drv Physical drive number (0..).
 * \param ctrl Control code.
 * \param buff Buffer to send/receive control data.
 *
 * \return RES_OK for success, otherwise DRESULT error code.
 */
extern unsigned num_sectors;
DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
    switch (command) {
    case GET_SECTOR_COUNT:
        switch (drive) {
        case FS_LUN_XFLASH:
            *(DWORD *)buffer = xflash_num_blocks;
        case FS_LUN_STREAM:
            *(DWORD *)buffer = num_sectors;
        default:
            return RES_NOTRDY;
        }
        break;
    case GET_BLOCK_SIZE:
        *(DWORD *)buffer = 8;
        break;
    case GET_SECTOR_SIZE:
        *(DWORD *)buffer = SECTOR_SIZE;
        break;
    case CTRL_SYNC:
        if (drive == FS_LUN_XFLASH
                && configuration_mode && xflash_4k_erase_capable)
            blkbuf_flush();
        break;
    case CTRL_ERASE_SECTOR:
        break;
    }

    return RES_OK;
}

//@}
