/*
 * External flash support.
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

#include <string.h>
#include <at25dfx.h>
#include <ioport.h>
#include "xflash.h"


// Flash parameters set during xflash_init()
uint32_t xflash_num_blocks;     // size in 512 byte blocks

bool xflash_init(void)
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

    uint32_t xflash_id;             // 3-byte JEDEC ID, e.g. 0x01461F
    if (at25dfx_read_dev_id(&xflash_id) != AT25_SUCCESS)
        return false;

    // Analyse type of serial flash to find out its size.
    unsigned mfr = xflash_id & 0xFF;        // manufacturer ID
    if (mfr == 0 || mfr == 0xFF)
        return false;
    xflash_num_blocks = 128 << (xflash_id >> 16 & 0xF);
    return true;
}

bool xflash_erase(uint32_t address)
{
    at25_status_t s = at25dfx_erase_block(address, AT25_BLOCK_ERASE_64K);
    return s == AT25_SUCCESS;
}

bool xflash_write(const uint8_t *data, uint16_t size, uint32_t address)
{
    at25_status_t s = at25dfx_write((uint8_t *)data, size, address);
    return s == AT25_SUCCESS;
}

bool xflash_verify(const uint8_t *data, uint16_t size, uint32_t address)
{
    uint8_t buf[512];

    while (size) {
        uint16_t block_size = size < sizeof buf ? size : sizeof buf;
        at25_status_t s = at25dfx_read(buf, block_size, address);

        if (s != AT25_SUCCESS || memcmp(buf, data, block_size) != 0)
            return false;

        size -= block_size;
        data += block_size;
        address += block_size;
    }

    return true;
}
