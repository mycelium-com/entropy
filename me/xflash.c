/*
 * External flash support.
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

#include "conf_access.h"

#include <stdio.h>
#include <at25dfx.h>
#include <ioport.h>

#include "sys/now.h"
#include "jpeg.h"
#include "xflash.h"
#include "data.h"

// Flash parameters set during xflash_init()
uint32_t xflash_id;             // 3-byte JEDEC ID, e.g. 0x01461F
bool xflash_4k_erase_capable;   // whether the flash can erase 4 kB blocks
uint32_t xflash_num_blocks;     // size in 512 byte blocks

// JEDEC manufacturer IDs
enum {
    ST_MICRON   = 0x20,
    MACRONIX    = 0xC2,
};

void xflash_init(void)
{
    ioport_set_pin_mode(PIN_PA13A_USART1_RTS, MUX_PA13A_USART1_RTS);
    ioport_disable_pin(PIN_PA13A_USART1_RTS);
    ioport_set_pin_mode(PIN_PA14A_USART1_CLK, MUX_PA14A_USART1_CLK);
    ioport_disable_pin(PIN_PA14A_USART1_CLK);
    ioport_set_pin_mode(PIN_PA15A_USART1_RXD, MUX_PA15A_USART1_RXD);
    ioport_disable_pin(PIN_PA15A_USART1_RXD);
    ioport_set_pin_mode(PIN_PA16A_USART1_TXD, MUX_PA16A_USART1_TXD);
    ioport_disable_pin(PIN_PA16A_USART1_TXD);

    at25dfx_initialize();
    at25dfx_set_mem_active(AT25DFX_MEM_ID);

    if (at25dfx_read_dev_id(&xflash_id) != AT25_SUCCESS) {
        global_error_flags |= FLASH_ERROR;
        return;
    }

    // Analyse type of serial flash to find out its size
    // and whether it is 4 kB subsector erase capable.
    unsigned mfr = xflash_id & 0xFF;        // manufacturer ID
    if (mfr == 0 || mfr == 0xFF) {
        puts("Serial flash is faulty.");
        global_error_flags |= FLASH_ERROR;
        return;
    }
    xflash_num_blocks = 128 << (xflash_id >> 16 & 0xF);
    xflash_4k_erase_capable = mfr == MACRONIX || (xflash_id & 0xFFFF) == 0xBA20;
}

void xflash_read(uint8_t *data, uint16_t size, uint32_t address)
{
    if (at25dfx_read(data, size, address) != AT25_SUCCESS)
        global_error_flags |= FLASH_ERROR;
}

void xflash_write(const uint8_t *data, uint16_t size, uint32_t address)
{
    if (at25dfx_write((uint8_t *)data, size, address) != AT25_SUCCESS)
        global_error_flags |= FLASH_ERROR;
}

void xflash_erase_4k(uint32_t address)
{
    uint8_t erase_cmd = xflash_4k_erase_capable ?
            AT25_BLOCK_ERASE_4K : AT25_BLOCK_ERASE_64K;

    if (at25dfx_erase_block(address, erase_cmd) != AT25_SUCCESS)
        global_error_flags |= FLASH_ERROR;
}

#if AT25DFX_MEM == ENABLE
// We are using flash as a buffer which can hold an entire JPEG file
// and serve it through USB in random order.

volatile bool xflash_ready;
static volatile bool xflash_ok;
static uint32_t erase_addr, erase_total;

bool xflash_erase(void)
{
    erase_addr = 0;
    erase_total = 512 * 1024;
    xflash_ok = false;
    xflash_ready = false;

    if (at25dfx_start_erasing_block(erase_addr, AT25_BLOCK_ERASE_64K)
            != AT25_SUCCESS) {
        puts("Error erasing serial flash.");
        return false;
    }
    xflash_ok = true;
    return true;
}

bool xflash_check_ready(void)
{
    uint8_t status;

    // Read status register and check busy bit.
    if (at25dfx_read_status(&status) != AT25_SUCCESS) {
        puts("Error erasing serial flash.");
        return false;   // flash I/O error
        // TODO: handle error
    }

    if ((status & AT25_STATUS_RDYBSY) != AT25_STATUS_RDYBSY_READY)
        return false;   // not ready yet

    erase_addr += 64 * 1024;

    if (erase_addr >= erase_total)
        return true;    // finished

    if (at25dfx_start_erasing_block(erase_addr, AT25_BLOCK_ERASE_64K)
            != AT25_SUCCESS) {
        puts("Error erasing serial flash.");
        // TODO: handle error
    }

    return false;       // either not ready or I/O error
}

// To be called periodically to poll for background operations.
void xflash_periodic_interrupt(void)
{
    if (xflash_ok && !xflash_ready)
        xflash_ready = xflash_check_ready();
}

bool xflash_write_jpeg(void)
{
    uint8_t *bprev, *bptr = 0;
    unsigned blk = 0;

    uint32_t t, tprev = now(), t_erase, tmax_jpeg = 0, tmax_write = 0;

    t = now();
    t_erase = t - tprev;
    tprev = t;

    do {
        bprev = bptr;
        bptr = jpeg_get_block(blk);
            t = now();
            if (t - tprev > tmax_jpeg) tmax_jpeg = t - tprev;
            tprev = t;
        if (at25dfx_write(bptr, 512, blk << 9) != AT25_SUCCESS) {
            printf("Error writing block %u.\n", blk);
            return false;
        }
            t = now();
            if (t - tprev > tmax_write) tmax_write = t - tprev;
            tprev = t;
        blk++;
    } while (bptr != bprev);

    printf("xflash: erase remaining %lu, max jpeg gen %lu, max flash write %lu.\n",
           t_erase, tmax_jpeg, tmax_write);

    return true;
}

#endif
