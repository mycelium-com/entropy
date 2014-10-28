/*
 * SAM4L Bootloader with USB MSC firmware update.
 * Loosely based on Atmel's SAM-BA for SAM4L.
 */

/**
 * \file
 *
 * \brief Declaration of main function used by MSC example
 *
 * Copyright (c) 2009-2011 Atmel Corporation. All rights reserved.
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

#ifndef _MAIN_H_
#define _MAIN_H_

//Application start address (beggining of the intvect)
#define APP_START_ADDRESS 				0x4000
#define APP_START_PAGE	 				(APP_START_ADDRESS/FLASH_PAGE_SIZE)


//Offset in the user page to fetch the gpio to check for triggering boot entry
//The value at this address is the gpio bit number to check and active state
//The booloader doesn't PU or PD this line, so it has be stable when bootloader starts
#define USERPAGE_CONFWORD_OFFSET 		0x10

//Write this value at USERPAGE_CONFWORD_OFFSET to force monitor/boot entry
#define CONFWORD_FORCE_MONITOR_VALUE	0x4D4153
#define CONFWORD_FORCE_MONITOR_MASK		0xFFFFFF

//Bit in the booloader config word for vddin<1.8 -> PS1
#define CONFWORD_LOW_VDDIN_BIT_OFFSET	24

/*! \brief Called by MSC interface
 * Callback running when USB Host enable MSC interface
 *
 * \retval true if MSC startup is ok
 */
bool main_msc_enable(void);

/*! \brief Called by MSC interface
 * Callback running when USB Host disable MSC interface
 */
void main_msc_disable(void);

/*! \brief Called when a start of frame is received on USB line
 */
void main_sof_action(void);

/*! \brief Enters the application in low power mode
 * Callback called when USB host sets USB line in suspend state
 */
void main_suspend_action(void);

/*! \brief Called by UDD when the USB line exit of suspend state
 */
void main_resume_action(void);

uint32_t freqm_measure(void);

// Bootloader entry GPIO pin details.
// Located in uninitialised RAM to prevent them from being
// overwritten between flash_startup() and main().
extern struct {
    GpioPort *port;
    uint32_t pin;
    uint32_t pin_state;
    uint8_t sector_buffer[512]; // buffer for serial flash
} _estack;

#endif // _MAIN_H_
