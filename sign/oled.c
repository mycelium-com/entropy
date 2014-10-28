/*
 * Basic support for the OLED1 Xplained Pro extension board.
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

#include <ioport.h>
#include <ssd1306.h>
#include <font.h>
#include "oled.h"

#include "verdana.c"

#define BUTTON1 EXT2_PIN_9
#define BUTTON2 EXT2_PIN_3
#define BUTTON3 EXT2_PIN_4

static unsigned buttons = 0;

void oled_init(void)
{
    // Initialize SPI and SSD1306 controller
    ssd1306_init();
    ssd1306_clear();

    // initialise buttons
    ioport_set_pin_dir(BUTTON1, IOPORT_DIR_INPUT);
    ioport_set_pin_mode(BUTTON1, IOPORT_MODE_PULLUP);
    ioport_set_pin_dir(BUTTON2, IOPORT_DIR_INPUT);
    ioport_set_pin_mode(BUTTON2, IOPORT_MODE_PULLUP);
    ioport_set_pin_dir(BUTTON3, IOPORT_DIR_INPUT);
    ioport_set_pin_mode(BUTTON3, IOPORT_MODE_PULLUP);
}

static int text_width(int font, const char *str)
{
    int w = 0;

    while (*str) {
        int c = *str++ - ' ';
        if (c >= 0 && c < 95)
            w += font == 0 ? font_table[c][0] + 1 :
                    verdana_font_data[verdana_font[c]];
    }
    return w;
}

static inline void clear_columns(int n)
{
    for (; n; --n)
        ssd1306_write_data(0);
}

static void write_large_text(int page, const char *str)
{
    while (*str) {
        int c = *str++ - ' ';
        if (c >= 0 && c < 95) {
            const uint8_t *char_ptr = verdana_font_data + verdana_font[c];
            int i = *char_ptr++;
            char_ptr += page * i;
            for (; i; --i)
                ssd1306_write_data(*char_ptr++);
        }
    }
}

void oled_write(int font, int row, const char *text)
{
    int offset = 0;
    int w;

    if (*text < ' ') {
        w = text_width(font, ++text);
        if (w < 128)
            offset = (128 - w) / 2;
    } else {
        w = text_width(font, text);
    }

    ssd1306_set_page_address(row);
    ssd1306_set_column_address(0);
    clear_columns(offset);
    if (font == 0)
        ssd1306_write_text(text);
    else
        write_large_text(0, text);
    clear_columns(-(offset + w) & 127);

    if (font == 1) {
        ssd1306_set_page_address(row + 1);
        ssd1306_set_column_address(0);
        clear_columns(offset);
        write_large_text(1, text);
        clear_columns(-(offset + w) & 127);
    }
}

void oled_clear(void)
{
    ssd1306_clear();
}

int oled_check_buttons(void)
{
    int code = 0;
    unsigned new_buttons = 0;

    if (!ioport_get_pin_level(BUTTON1)) {
        new_buttons |= 1 << 0;
        if (!(buttons & 1 << 0))
            code = 1;
    }
    if (!ioport_get_pin_level(BUTTON2)) {
        new_buttons |= 1 << 1;
        if (!(buttons & 1 << 1))
            code = 2;
    }
    if (!ioport_get_pin_level(BUTTON3)) {
        new_buttons |= 1 << 2;
        if (!(buttons & 1 << 2))
            code = 3;
    }

    buttons = new_buttons;
    return code;
}
