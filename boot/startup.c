/*
 * Primary startup file for the bootloader.
 * Copies everything into SRAM and jumps there.
 */

/**
 * \file
 *
 * \brief SAM4L-SAM-BA Bootloader
 *
 * Copyright (c) 2011 - 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
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

#include <stdint.h>
#include <stdbool.h>
#include <exceptions.h>
#include "main.h"

extern uint32_t _stext_in_flash;
extern uint32_t _sfixed;
extern uint32_t _erelocate;
extern uint32_t __ram_end__;

void flash_startup(void);

__attribute__ ((section(".start.vectors")))
IntFunc flash_exception_table[] = {
    (IntFunc) (0x20000100),     // initial stack pointer
    flash_startup,              // reset vector
    0, 0, 0, 0, 0, 0,           // unused or reserved vectors
    0                           // reserved for offset to signature block
};

//Init freqm using RCSYS as reference clock
//Measure OSC0 for 1ms (115 cycles of RCSYS)
__attribute__ ((section(".start.text")))
static void init_freqm(void)
{
	FREQM->FREQM_MODE = 0;
	while((FREQM->FREQM_STATUS & FREQM_STATUS_RCLKBUSY)>0);
	FREQM->FREQM_MODE = FREQM_MODE_REFSEL(FREQM_REF_RCOSC);
	while((FREQM->FREQM_STATUS & FREQM_STATUS_RCLKBUSY)>0);
	FREQM->FREQM_MODE = FREQM_MODE_REFSEL(FREQM_REF_RCOSC)|FREQM_MODE_REFCEN;
	while((FREQM->FREQM_STATUS & FREQM_STATUS_RCLKBUSY)>0);
	FREQM->FREQM_MODE = FREQM_MODE_REFSEL(FREQM_REF_RCOSC)|FREQM_MODE_REFCEN|
			FREQM_MODE_REFNUM(115)|FREQM_MODE_CLKSEL(FREQM_OSC0);
}

//Measure OSC0 for 1ms (115 cycles of RCSYS)
//Frequency is outputted in kHz
__attribute__ ((section(".start.text")))
uint32_t freqm_measure(void)
{
	FREQM->FREQM_CTRL = FREQM_CTRL_START;
	while((FREQM->FREQM_STATUS & FREQM_STATUS_BUSY)>0);
	return(FREQM->FREQM_VALUE & FREQM_VALUE_VALUE_Msk);
}

__attribute__ ((section(".start.text")))
static inline void wait_1_ms(void)
{
	freqm_measure();
}

__attribute__ ((section(".start.text")))
static void check_start_application(void)
{
	uint32_t * ptr_reset_vector;
	uint32_t * ptr_msp;
	uint32_t * ptr_configword;
	uint32_t bootloader_config_word;
	uint32_t app_start_add;
	uint8_t i;
	uint8_t pin_forced_count;

    _estack.pin = 0;

	//Config word of the bootloader
	ptr_configword = (uint32_t *) (FLASH_USER_PAGE_ADDR+USERPAGE_CONFWORD_OFFSET);
	bootloader_config_word = (*ptr_configword);

	if((bootloader_config_word&CONFWORD_FORCE_MONITOR_MASK)==CONFWORD_FORCE_MONITOR_MASK) {
		// Extremely dangerous!  Writing application firmware without means to
		// enter bootloader will almost certainly brick the device.
		// TODO: error indication.
		for (;;);
	}

	//Get reset vector from intvect table of application
	ptr_reset_vector = (uint32_t *) (APP_START_ADDRESS+4);

	//Test reset vector of application @APP_START_ADDRESS+4
	// Stay in SAM-BA if *(APP_START+0x4) == 0xFFFFFFFF
	//Application erased condition
	if(*ptr_reset_vector==0xFFFFFFFF)
	{
		//Stay in bootloader
		return;
	}

	//Check magic value to force monitor mode
	if((bootloader_config_word&CONFWORD_FORCE_MONITOR_MASK)==CONFWORD_FORCE_MONITOR_VALUE)
		return;
	//Expected format : 0xXX XX Xx NN where BIT8 is the active state (bootloader triggered)
	// NN is the GPIO number (ex : PIN_PC31=0x5F (95d))
	_estack.pin = bootloader_config_word&0xFF;
	_estack.pin_state = (bootloader_config_word>>8)&1;
	// Set the selected pin in input mode
	_estack.port = &GPIO->GPIO_PORT[_estack.pin >> 5];
    _estack.pin_state <<= _estack.pin & 31;
    _estack.pin = 1 << (_estack.pin & 31);
    // Always enable the Schmitt trigger for input pins.
    _estack.port->GPIO_STERS = _estack.pin;
	// Pull the selected pin in the opposite direction
    if (_estack.pin_state)
        _estack.port->GPIO_PDERS = _estack.pin;
    else
        _estack.port->GPIO_PUERS = _estack.pin;
	// Wait for debounce time: 10ms
	for(i=0;i<10;i++)
		wait_1_ms();
	// Test pin value on 3 samples
	pin_forced_count = 0;
	for(i=0;i<3;i++)
	{
        if ((_estack.port->GPIO_PVR & _estack.pin) == _estack.pin_state)
			pin_forced_count++;
		wait_1_ms();
	}
    // Majority vote
	if (pin_forced_count > 1) {
        // Stay in the bootloader
        return;
	}
	// Disable pull-up/down
    _estack.port->GPIO_PDERC = _estack.pin;
    _estack.port->GPIO_PUERC = _estack.pin;

	// Start the application

	//Put reset vector in function ptr
	//ptrfunc.__ptr = (void*)(*ptr_reset_vector);
	app_start_add = (*ptr_reset_vector);
	//On reset, the processor loads the MSP with the value from address 0x00000000
	//So before jumping in application, bootloader loads the MSP
	//with the value from address 0x00004000
	//Execute / jump in application
	ptr_msp = (uint32_t *) (APP_START_ADDRESS);
	__set_MSP(*(ptr_msp));
	asm("bx %0"::"r"(app_start_add));
}


__attribute__ ((section(".start.text")))
void flash_startup(void)
{
	uint32_t *src = &_stext_in_flash;
    uint32_t *dst = &_sfixed;
    IntFunc main_bootloader_entry_point = (IntFunc)src[1];

	//Start FREQM,
	init_freqm();

	//Jump in application if condition is satisfied
	check_start_application();

    do
        *dst++ = *src++;
    while (dst < &_erelocate);

    __ram_end__ = 0;
	__set_MSP((uintptr_t)&_estack);
    main_bootloader_entry_point();
}
