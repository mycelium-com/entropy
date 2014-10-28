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

#include "lcd.h"

void lcd_init(void)
{
    /*
     * LCDCA Controller initialisation:
     * - Clock,
     * - Timing:  64 Hz frame rate & low power waveform, FC0, FC1, FC2
     * - Interrupt: off
     */
    static struct lcdca_config lcdca_cfg = {
        .port_mask = LCD_NB_OF_SEG,
        .x_bias = false,
        .lp_wave = true,
        .duty_type = LCD_NB_OF_COM & 3,
        .lcd_pres = false,
        .lcd_clkdiv = 3,
        .fc0 = 16,
        .fc1 = 2,
        .fc2 = 6,
        .contrast = LCD_CONTRAST_LEVEL,
    };
    static struct lcdca_blink_config blink_cfg = {
        .lcd_blink_timer = LCDCA_TIMER_FC1,
        .lcd_blink_mode = LCDCA_BLINK_SELECTED,
    };
    static struct lcdca_automated_char_config automated_char_cfg = {
        .automated_mode = LCDCA_AUTOMATED_MODE_SEQUENTIAL,
        .automated_timer = LCDCA_TIMER_FC2,
        .lcd_tdg = LCDCA_TDG_14SEG4COM,
        .stseg = LCD_FIRST_14SEG_4C,
        .dign = LCD_WIDTH_14SEG_4C,
        .steps = 0,
        .dir_reverse = LCDCA_AUTOMATED_DIR_NOT_REVERSE,
    };

    lcdca_clk_init();
    lcdca_set_config(&lcdca_cfg);
    lcdca_enable();
    lcdca_enable_timer(LCDCA_TIMER_FC0);
    lcdca_enable_timer(LCDCA_TIMER_FC1);
    lcdca_enable_timer(LCDCA_TIMER_FC2);
    lcdca_blink_set_config(&blink_cfg);
    lcdca_blink_enable();
    lcdca_automated_char_set_config(&automated_char_cfg);

    // Turn on LCD back light.
    ioport_set_pin_dir(LCD_BL_GPIO, IOPORT_DIR_OUTPUT);
    ioport_set_pin_level(LCD_BL_GPIO, LCD_BL_ACTIVE_LEVEL);
}
