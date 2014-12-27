/*
 * Initial Flash Programming Applet.
 * Can brick hardware.  Use with care.
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
#include <stdint.h>
#include <stdbool.h>
#include <board.h>
#include <sysclk.h>
#include <flashcalw.h>
#include <ioport.h>
#include <led.h>
#include <udc.h>

#include "lib/fwsign.h"
#include "lib/sha256.h"
#include "sys/stdio-uart.h"
#include "sys/sync.h"
#include "fw-access.h"
#include "ui.h"
#include "main.h"

#define EXT_FLASH   (BOARD == USER_BOARD)
#if EXT_FLASH
#include <at25dfx.h>
#endif


static volatile bool main_b_msc_enable = false;

static void check_if_finished(void);

// States of checksum_state.
enum {
    BOOT = 1,
    MAIN = 2,
    DONE,
};

int main(void)
{
    uint32_t xflash_id;     // 3-byte little endian JEDEC ID, e.g. 0x1320C2

    sysclk_init();
    board_init();
    ui_init();
    sync_init();
    stdio_uart_init();

#if EXT_FLASH
    // Initialise serial flash.
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
#endif

    appname("Initial Flash Programming Applet");

#if EXT_FLASH
    if (at25dfx_read_dev_id(&xflash_id) != AT25_SUCCESS)
        goto error;

    // Analyse type of serial flash to find out its size.
    unsigned mfr = xflash_id & 0xFF;        // manufacturer ID
    if (mfr == 0 || mfr == 0xFF) {
error:
        puts("Flash fault.");
        ui_error(2);
    }
    ext_flash_size = 128 << (xflash_id >> 16 & 0xF);
#else
    ext_flash_size = xflash_id = 0;
#endif

    // Start USB stack and attach MSB device.
    udc_start();

    ui_btn_count = 0;

    while (checksum_state != DONE) {
        if (main_b_msc_enable) {
            if (!udi_msc_process_trans()) {
                if (magic_sector_addr)
                    check_if_finished();
            }
        }

        if (ui_btn_count >= 2 && ui_btn_count < 20) {
            ui_btn_count = 20;
            // unlock bootloader
            flashcalw_lock_page_region(0, false);
            flashcalw_lock_page_region(16, false);
            puts("Bootloader unlocked.");
        }

        sync_diag_print();
    }

    // Detach device, stop USB stack, and shut down.
    puts("Done.");
    udc_stop();
    flashcalw_lock_page_region(0, true);
    flashcalw_lock_page_region(16, true);
    LED_Off(LED0);
    for (;;);
}

void main_suspend_action(void)
{
    sync_suspend();
}

void main_resume_action(void)
{
}

void main_sof_action(void)
{
    sync_frame();
}

bool main_msc_enable(void)
{
    main_b_msc_enable = true;
    return true;
}

void main_msc_disable(void)
{
    main_b_msc_enable = false;
}

static void check_if_finished(void)
{
    enum {
        NSHA = SECTOR_SIZE / SHA256_BLOCK_SIZE,
    };

    static uint32_t hash[8];
    union {
        uint8_t  b[SECTOR_SIZE];
        uint32_t w[NSHA][SHA256_BLOCK_SIZE / 4];
    } buf;

    if (checksum_state & 7) {
        // check images in the on-chip flash
        const uint32_t *base = (uint32_t *) ((checksum_state - BOOT) * 0x4000);
        uint32_t offset = base[FW_SIGN_VECTOR];

        if (offset == 0 || offset > 240 * 1024)
            return;

        const struct Firmware_signature *signature =
            (struct Firmware_signature *) (base + offset / 4);

        offset += offsetof(struct Firmware_signature, hash);
        sha256_hash(hash, (const uint8_t *) base, offset);

        if (memcmp(hash, signature->hash.b, sizeof hash) == 0) {
            printf("%s hash OK.\n",
                    checksum_state == BOOT ? "Bootloader" : "Main firmware");
            checksum_state++;   // signature OK, move on to next check
        }
        return;
    }

    // check image in the external flash

    if (checksum_state == ext_flash_start_addr)
        sha256_init(hash);

#if EXT_FLASH
    if (at25dfx_read(buf.b, sizeof buf, checksum_state) != AT25_SUCCESS) {
        printf("Flash read at %#lx failed.\n", checksum_state);
        udc_stop();
        ui_error(2);
    }
#endif

    if (checksum_state < magic_sector_addr) {
        // hash current sector
        int i;
        for (i = 0; i < NSHA; i++)
            sha256_transform(hash, buf.w[i]);
        checksum_state += sizeof buf;
    } else {
        // check hash against magic sector
        sha256_finish(hash, buf.b, 32, checksum_state - ext_flash_start_addr + 32);
        if (memcmp(hash, buf.b + 32, sizeof hash) == 0) {
            puts("Serial flash hash OK.");
            checksum_state = BOOT;  // hash OK, go to next state
        } else
            checksum_state = ext_flash_start_addr;  // bad hash, restart
    }
}
