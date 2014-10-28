/*
 * Bootloader Replacement Tool.
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

#include <string.h>
#include <stdbool.h>
#include <board.h>
#include <sysclk.h>
#include <flashcalw.h>
#include <delay.h>
#include <led.h>

#include "boot/main.h"
#include "sys/conf_system.h"
#include "ui.h"

extern const unsigned char boot_bin[];
extern const int boot_bin_len;

static bool flash_bootloader(void);

int main(void)
{
    board_init();
    sysclk_init();
    ui_init();

    // If we start in configuration mode, it's possible that we've just
    // invoked ourselves after flashing the bootloader.
    // In that case, do nothing.
    if (CONFIG_FW_REG & CONFIG_FW_MASK) {
        LED_Off(LED0);
        for (;;);
    }

    // Request confirmation.
    ui_warn_bl_flash();
    ui_wait_for_click();
    ui_stop();
    LED_On(LED0);

    // Flash and retry if failed.
    if (!flash_bootloader()) {
        delay_ms(1000);
        if (!flash_bootloader())
            ui_error(6);
    }

    // Restart main firmware in configuration mode.
    LED_Off(LED0);
    CONFIG_FW_REG = CONFIG_FW_MASK;             // config mode flag
    __set_MSP(*(uint32_t *) APP_START_ADDRESS); // app stack pointer
    asm ("bx %0" :: "r" (*(uint32_t *) (APP_START_ADDRESS + 4)));
}

static bool flash_bootloader(void)
{
    unsigned pages_per_region = flashcalw_get_page_count_per_region();
    bool use_region_1 = APP_START_PAGE > pages_per_region;

    // Unlock bootloader region(s).
    flashcalw_lock_page_region(0, false);
    if (use_region_1)
        flashcalw_lock_page_region(pages_per_region, false);

    // Flash bootloader in boot_bin[] from address 0.
    flashcalw_memcpy((volatile void *) 0, boot_bin, boot_bin_len, true);

    // Lock bootloader region(s).
    flashcalw_lock_page_region(0, true);
    if (use_region_1)
        flashcalw_lock_page_region(pages_per_region, true);

    // Verify.
    // memcmp() requires addresses to be non-null, so we verify the value
    // at address 0 separately.
    return *(unsigned char *)0 == boot_bin[0]
        && memcmp((void *) 1, boot_bin + 1, boot_bin_len - 1) == 0;
}
