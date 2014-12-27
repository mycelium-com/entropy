/*
 * External serial flash quick test.
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
#include <board.h>
#include <ioport.h>

#include "sys/now.h"
#include "xflash.h"
#include "main.h"

#if BOARD == USER_BOARD

#include <usart_spi.h>
#include <at25dfx.h>

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
}

static void print_elapsed_time(char **ptr, const char *what, uint32_t t)
{
    t = t * 1000 / AST_FREQ;
    *ptr += sprintf(*ptr, "%s: %lu.%03lu s.\r\n", what, t / 1000, t % 1000);
}

bool xflash_check(char **ptr, char *wbuf, int buflen)
{
    static struct usart_spi_device USART_SPI_DEVICE_EXAMPLE = {
        /* Board specific select ID. */
        .id = 1
    };
    uint8_t buf[3];
    int size;
    const char *mfr = 0;

    xflash_init();

    usart_spi_select_device(AT25DFX_SPI_MODULE, &USART_SPI_DEVICE_EXAMPLE);
    usart_spi_write_packet(AT25DFX_SPI_MODULE, (const uint8_t*)"\x9f", 1);
    usart_spi_read_packet(AT25DFX_SPI_MODULE, buf, 3);
    usart_spi_deselect_device(AT25DFX_SPI_MODULE, &USART_SPI_DEVICE_EXAMPLE);

    *ptr += sprintf(*ptr, "Serial flash: ");

    if (buf[0] == 0x00 || buf[0] == 0xFF) {
        *ptr += sprintf(*ptr, "faulty.\r\n");
        return false;
    }

    *ptr += sprintf(*ptr, "JEDEC ID %02X %02X %02X",
            buf[0], buf[1], buf[2]);

    if (buf[0] == 0x20) {
        mfr = "Micron";
        size = 64 << (buf[2] & 0xF);
    } else if (buf[0] == 0xC2) {
        mfr = "Macronix";
        size = 64 << (buf[2] & 0xF);
    }

    if (mfr)
        *ptr += sprintf(*ptr, ", %s, %d kbytes.\r\n", mfr, size);
    else {
        strcpy(*ptr, ".\r\n");
        *ptr += 3;
    }

    const char *msg;
    char rbuf[buflen];
    uint32_t tprev = now();
    at25_status_t s = at25dfx_erase_block(0, AT25_BLOCK_ERASE_64K);

    print_elapsed_time(ptr, "Flash block erase time", now() - tprev);
    if (s != AT25_SUCCESS) {
        msg = "erase";
        goto error;
    }
    if (at25dfx_write((uint8_t*) wbuf, buflen, 0) != AT25_SUCCESS) {
        msg = "write";
        goto error;
    }
    if (at25dfx_read((uint8_t*) rbuf, buflen, 0) != AT25_SUCCESS) {
        msg = "read";
        goto error;
    }
    if (memcmp(wbuf, rbuf, buflen) != 0) {
        msg = "data check";
        goto error;
    }

    *ptr += sprintf(*ptr, "Serial flash test OK.\r\n");

    uint32_t id = buf[0] << 16 | buf[1] << 8 | buf[2];
    if (id != 0xC22013 && id != 0x20BA16)
        set_fault(WRONG_FLASH);

    return true;

error:
    *ptr += sprintf(*ptr, "Serial flash %s failed.\r\n", msg);
    return false;
}

#else

void xflash_init(void)
{}

bool xflash_check(char **ptr, char *buf, int buflen)
{
    return true;
}

#endif
