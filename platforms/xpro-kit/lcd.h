/*
 * YMCC42412AAAFDCL segment LCD with 96 segments.
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

#ifndef PLATFORM_LCD_H_INCLUDED
#define PLATFORM_LCD_H_INCLUDED

#include <lcdca.h>
#include <ioport.h>

// YMCC42412AAAFDCL LCD glass panel definitions.
enum {
    LCD_NB_OF_COM       = 4,
    LCD_NB_OF_SEG       = 24,
    LCD_CONTRAST_LEVEL  = 30,

    // alphanumeric field configuration:
    // 14-segment with 4 commons terminals
    LCD_FIRST_14SEG_4C  = 4,    // first segment
    LCD_WIDTH_14SEG_4C  = 5,    // number of characters
    LCD_DIR_14SEG_4C    = 0,    // 0 = from left to right
};

// segments with support for automatic blinking
#define LCD_ICON_ATMEL          0, 1
#define LCD_ICON_USB            1, 1
#define LCD_ICON_COLON          3, 1
#define LCD_ICON_PLAY           2, 1
#define LCD_ICON_BAT            0, 0
#define LCD_ICON_BAT_LEVEL_1    1, 0
#define LCD_ICON_BAT_LEVEL_2    3, 0
#define LCD_ICON_BAT_LEVEL_3    2, 0

// segments without support for automatic blinking
#define LCD_ICON_WLESS_0        3, 3
#define LCD_ICON_WLESS_1        3, 2
#define LCD_ICON_WLESS_2        2, 3
#define LCD_ICON_WLESS_3        2, 2
#define LCD_ICON_VOLT           1, 2
#define LCD_ICON_MILLI_VOLT     1, 3
#define LCD_ICON_AM             0, 2
#define LCD_ICON_PM             0, 3
#define LCD_ICON_MINUS          0, 17
#define LCD_ICON_DOT_0          0, 5
#define LCD_ICON_DOT_1          3, 6
#define LCD_ICON_DOT_2          3, 10
#define LCD_ICON_DOT_3          3, 14
#define LCD_ICON_DOT_4          3, 18
#define LCD_ICON_ONE_LOWER      0, 9
#define LCD_ICON_ONE_UPPER      0, 13
#define LCD_ICON_DEGREE_C       3, 22
#define LCD_ICON_DEGREE_F       0, 21

void lcd_init(void);

// Write string to YMCC42412AAAFDCL LCD glass alphanumeric field.
// The field is five character wide.
static inline void lcd_write(const char *text)
{
    lcdca_write_packet(LCDCA_TDG_14SEG4COM, LCD_FIRST_14SEG_4C,
            (const uint8_t *)text, LCD_WIDTH_14SEG_4C, LCD_DIR_14SEG_4C);
}

// Turn LCD backlight on (on=true) or off (on=false).
static inline void lcd_backlight(bool on)
{
    ioport_set_pin_level(LCD_BL_GPIO,
            on ? LCD_BL_ACTIVE_LEVEL : LCD_BL_INACTIVE_LEVEL);
}

#endif
