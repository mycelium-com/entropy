/**
 * \file
 *
 * \brief User Interface
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
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
#include <led.h>
#include <ioport.h>
#include <gpio.h>
#include "ui.h"

volatile int ui_btn_count;

static void button_interrupt(void)
{
    ui_btn_count++;
    printf("BTN: %d.\n", ui_btn_count);
}

void ui_init(void)
{
    // Initialise LED.
    LED_Off(LED0);

    // Initialise button.
    ui_btn_count = 0;
    ioport_set_pin_dir(BUTTON_0_PIN, IOPORT_DIR_INPUT);
    ioport_set_pin_mode(BUTTON_0_PIN, IOPORT_MODE_PULLUP |
            IOPORT_MODE_GLITCH_FILTER);
    ioport_set_pin_sense_mode(BUTTON_0_PIN, IOPORT_SENSE_FALLING);
    if (!gpio_set_pin_callback(BUTTON_0_PIN, button_interrupt, 2))
        puts("Error setting GPIO callback.");
    gpio_enable_pin_interrupt(BUTTON_0_PIN);
}

void ui_off(void)
{
    LED_Off(LED0);
}

void ui_powerdown(void)
{
    //LED_Off(LED0);
}

void ui_wakeup(void)
{
    //LED_On(LED0);
}

void ui_start_read(void)
{
    //LED_Toggle(LED0);
}

void ui_stop_read(void)
{
    //LED_Toggle(LED0);
}

void ui_start_write(void)
{
}

void ui_stop_write(void)
{
}

void ui_process(uint16_t framenumber)
{
    if (0 == framenumber) {
        LED_On(LED0);
    }
    if (1000 == framenumber) {
        LED_Off(LED0);
    }
}
