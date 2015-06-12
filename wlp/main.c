/*
 * Word List Programming Tool.
 *
 * Copyright 2015 Mycelium SA, Luxembourg.
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
#include <delay.h>
#include <led.h>

#include "xflash.h"
#include "ui.h"

extern const unsigned char en_US_b39[];
extern const int en_US_b39_len;

static int flash_wordlist(void);

int main(void)
{
    board_init();
    sysclk_init();
    ui_init();

    if (!xflash_init())
        ui_error(UI_INIT);

    // Flash and retry if failed.
    if (flash_wordlist() != 0) {
        delay_ms(1000);
        int err = flash_wordlist();
        if (err != 0)
            ui_error(err);
    }

    // Indicate success and wait to get unplugged.
    LED_Off(LED0);
    for (;;);
    return 0;
}

static int flash_wordlist(void)
{
    const uint32_t address = (xflash_num_blocks << 9) - XFLASH_WORDLIST_OFFSET;

    LED_On(LED0);
    if (!xflash_erase(address))
        return UI_ERASE;
    LED_Off(LED0);
    if (!xflash_write(en_US_b39, en_US_b39_len, address))
        return UI_WRITE;
    LED_On(LED0);
    if (!xflash_verify(en_US_b39, en_US_b39_len, address))
        return UI_VERIFY;
    LED_Off(LED0);

    return 0;
}
