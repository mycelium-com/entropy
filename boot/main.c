/*
 * SAM4L Bootloader with USB MSC firmware update.
 * Loosely based on Atmel's SAM-BA for SAM4L.
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

/*
USB clock is chosen in the following order:
	- a precise external clock at XIN (PA00) input (less than 100ppm error)
	- an XTAL between XIN (PA00) and XOUT (PA01)
	- built-in RC oscillator RCFAST at 12 MHz

Supported frequencies for USB:
	6 MHz +-2500ppm
	7,3728 MHz +-900ppm
	8 MHz +-2500ppm
	12 MHz +-2500ppm
	14,7456 MHz +-900ppm
	16 MHz +-2500ppm
	24 MHz +-2500ppm

Pins Usage
--------------------
The following pins are used by the program :
PA00 : input
PA01 : output
PA25 : I/O usb
PA26 : I/O usb

Memory Mapping
--------------------
Bootloader code will be located at 0x0 and executed before any applicative code.

Applications compiled to be executed along with the bootloader will start at
0x4000
Before jumping to the application, the bootloader changes the VTOR register
to use the interrupt vectors of the application @0x4000.
(Although this is not required as application code is taking care of this.)
*/

#include <freqm.h>
#include <flashcalw.h>
#include <ioport.h>
#include <osc.h>
#include <sysclk.h>
#include <udc.h>

#include "conf_board.h"
#include "conf_clock.h"
#include "conf_usb.h"
#include "conf_access.h"
#include "sys/conf_system.h"
#include "ui.h"
#include "main.h"


// Set to 1 to first try to find if OSC0 has a suitable crystal,
// like it is done in the original implementation from Atmel.
// This would also enable full chip erase in "secure" mode, which is
// not used in this project and therefore disabled by default.
#define TRY_XTAL 0

// To avoid linking with crt0 from the C library.
void __libc_init_array(void);
void __libc_init_array(void) {}


uint8_t b_usb_selected;

static inline void wait_1_ms(void)
{
	freqm_measure();
}

#if TRY_XTAL

uint32_t osc0_freq;
int8_t   i_selected_frequency;
uint8_t  u8_clock_status;
uint8_t  b_usb_clock_available;

//Items type of supported frequencies LUT
typedef struct
{
	uint32_t expected_measurement;
	uint32_t tolerance;
	uint32_t usbpll_setting;
}st_supported_freq;

//Define LUT with supported frequencies measurements as entry key
//Warning : supported frequencies shall not be too close to each other :
//a supported frequency is selected automatically after a measurement
//done with an embedded oscillator with limited accuracy : +- 4.25%
// a 5% difference between 2 contiguous frequencies is necessary to cover
// all extreme conditions (temp,voltage)
const st_supported_freq tab_supported_freq[] =
{
	// Expected freq in kHz, Tolerance in kHz,Pll reg value : MUL,DIV,DIV2,Range
	{ 6000, 250,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL(15)|SCIF_PLL_PLLDIV(1)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)},
	{ 7372, 300,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL(12)|SCIF_PLL_PLLDIV(1)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)},
	{ 8000, 300,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL(11)|SCIF_PLL_PLLDIV(1)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)},
	{12000, 550,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL( 3)|SCIF_PLL_PLLDIV(0)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)},
	{14745, 600,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL(12)|SCIF_PLL_PLLDIV(2)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)},
	{16000, 600,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL( 2)|SCIF_PLL_PLLDIV(0)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)},
	{24000,1200,SCIF_PLL_PLLCOUNT_Msk|SCIF_PLL_PLLMUL( 7)|SCIF_PLL_PLLDIV(2)|SCIF_PLL_PLLOPT(2)|SCIF_PLL_PLLOSC(0)}
};

//Clock status flag (bit field)
#define CLOCK_STATUS_EXTCLK			1
#define CLOCK_STATUS_CRYSTAL		2
#define CLOCK_STATUS_RCFAST			4

//simple abs function
static uint32_t i32abs(int32_t value)
{
  if(value>0)
    return(value);
  else
    return(-value);
}

//Setup necessary clocks for the bootloader
//Allow USB if one of the supported input frequency is available
static void usb_clock_setup(void)
{
	uint8_t i;
	int32_t delta;
	uint32_t usbpll_setting;

	//Initialize clock status var
	u8_clock_status = 0;

	//Start OSC0 in external mode
	SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
		| SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_OSCCTRL0 - (uint32_t)SCIF);
	SCIF->SCIF_OSCCTRL0 = (SCIF_OSCCTRL0_STARTUP(0))|SCIF_OSCCTRL0_OSCEN;

	wait_1_ms();

	//Ext clock should be up and running at boot
	//Test if OSC0 clock is alive
	if((SCIF->SCIF_PCLKSR&SCIF_PCLKSR_OSC0RDY)>0)
	{
		//Ext clock is alive
		u8_clock_status = CLOCK_STATUS_EXTCLK;
	}
	else
	{//Ext clock failed, now let's try crystal mode
		//Start OSC0 in crystal mode
		SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
			| SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_OSCCTRL0 - (uint32_t)SCIF);
		SCIF->SCIF_OSCCTRL0 = SCIF_OSCCTRL0_GAIN(3)|(SCIF_OSCCTRL0_STARTUP(0))|SCIF_OSCCTRL0_OSCEN|SCIF_OSCCTRL0_MODE;

		//Wait for oscillation start (up to 100ms for low freqs xtal)
		for(i=0;i<100;i++)
		{
			wait_1_ms();
		}

		//Now the Test if OSC0 clock is alive
		if((SCIF->SCIF_PCLKSR&SCIF_PCLKSR_OSC0RDY)>0)
		{
			u8_clock_status = CLOCK_STATUS_CRYSTAL;
		}
		else
		{
			//No valid OSC0 clock found, fall back to RCFAST
		}
	}

	//No selected frequency
	i_selected_frequency = -1;

	if((u8_clock_status & (CLOCK_STATUS_CRYSTAL|CLOCK_STATUS_EXTCLK))>0)
	{
		//One of the stable clock is available, measure it
		osc0_freq=freqm_measure();

		//Search closest match in supported frequencies list
		for(i=0;i<(sizeof(tab_supported_freq)/sizeof(st_supported_freq));i++)
		{
			delta = tab_supported_freq[i].expected_measurement - osc0_freq;
			if(i32abs(delta)<tab_supported_freq[i].tolerance)
			{//Measurement is close enough, select this frequency
				i_selected_frequency = i;
				break;
			}
		}
	}

	// Forbid USB until we find a suitable clock source
	b_usb_clock_available = false;

	if(i_selected_frequency>=0)
	{
		// If a known frequency is found, setup the PLL according to the table
		usbpll_setting = tab_supported_freq[i_selected_frequency].usbpll_setting;
	}
	else
	{
		// No valid OSC0 clock with a suitable frequency, fall back to RCFAST
		u8_clock_status = CLOCK_STATUS_RCFAST;
		pll_enable_source(PLL_SRC_GCLK9);
		usbpll_setting = SCIF_PLL_PLLCOUNT_Msk | SCIF_PLL_PLLMUL(3) | SCIF_PLL_PLLDIV(0)
			| SCIF_PLL_PLLOPT(2) | SCIF_PLL_PLLOSC(PLL_SRC_GCLK9);
	}

	// Setup PLL with the selected config
	SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
		| SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_PLL[0].SCIF_PLL - (uint32_t)SCIF);
	SCIF->SCIF_PLL[0].SCIF_PLL = usbpll_setting;

	SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
		| SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_PLL[0].SCIF_PLL - (uint32_t)SCIF);
	SCIF->SCIF_PLL[0].SCIF_PLL = usbpll_setting | SCIF_PLL_PLLEN;

	//PLL Lock time should be less than 1 ms
	wait_1_ms();
	if((SCIF->SCIF_PCLKSR&SCIF_PCLKSR_PLL0LOCK)>0)
	{
		b_usb_clock_available = true;
	}
}

//Erase the entire application area if part is secured
static void erase_secured_device(void)
{
	uint32_t p;

	//Erase all application pages (monitor not erased)
	//Blank pages are not re-erase to avoid unnecessary wear of the flash
	for(p=APP_START_PAGE;p<flashcalw_get_page_count();p++)
	{
		if(flashcalw_quick_page_read(p)==false)
		{
			//Unlock if page is locked
			if(flashcalw_is_page_region_locked(p))
			{
				flashcalw_lock_page_region(p,false);
			}
			//Erase & Ensure that page is successfully erased
			while(flashcalw_erase_page(p, true)==0);
		}
	}

}

#else

// Setup necessary clocks for the bootloader.
static void usb_clock_setup(void)
{
	pll_enable_source(PLL_SRC_GCLK9);
	uint32_t usbpll_setting = SCIF_PLL_PLLCOUNT_Msk | SCIF_PLL_PLLMUL(3)
		| SCIF_PLL_PLLDIV(0) | SCIF_PLL_PLLOPT(2) | SCIF_PLL_PLLOSC(PLL_SRC_GCLK9);

	// Setup PLL with the selected config
	SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
		| SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_PLL[0].SCIF_PLL - (uint32_t)SCIF);
	SCIF->SCIF_PLL[0].SCIF_PLL = usbpll_setting;

	SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
		| SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_PLL[0].SCIF_PLL - (uint32_t)SCIF);
	SCIF->SCIF_PLL[0].SCIF_PLL = usbpll_setting | SCIF_PLL_PLLEN;

	// PLL Lock time should be less than 1 ms
	while ((SCIF->SCIF_PCLKSR & SCIF_PCLKSR_PLL0LOCK) == 0)
	{}
}

#define b_usb_clock_available true

#endif

static volatile bool main_b_msc_enable = false;

/**
 *  \brief SAM4L SAM-BA Main loop.
 *  \return Unused (ANSI-C compatibility).
 */
int main(void)
{
	ioport_init();

	//Enable necessary clocks for monitor
	PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAUL)
		| PM_UNLOCK_ADDR((uint32_t)&PM->PM_PBAMASK - (uint32_t)PM);
	PM->PM_PBAMASK = PM_PBAMASK_USART0 | PM_PBAMASK_TC0;

	PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAUL)
		| PM_UNLOCK_ADDR((uint32_t)&PM->PM_PBBMASK - (uint32_t)PM);
	PM->PM_PBBMASK = PM_PBBMASK_HFLASHC|PM_PBBMASK_USBC;

	PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAUL)
		| PM_UNLOCK_ADDR((uint32_t)&PM->PM_PBCMASK - (uint32_t)PM);
	PM->PM_PBCMASK = PM_PBCMASK_PM|PM_PBCMASK_CHIPID|PM_PBCMASK_SCIF|
			PM_PBCMASK_FREQM|PM_PBCMASK_GPIO;

	PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAUL)
		| PM_UNLOCK_ADDR((uint32_t)&PM->PM_PBDMASK - (uint32_t)PM);
	PM->PM_PBDMASK = PM_PBDMASK_BPM|PM_PBDMASK_BSCIF|PM_PBDMASK_AST|
			PM_PBDMASK_WDT|PM_PBDMASK_EIC|PM_PBDMASK_PICOUART;

	PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAUL)
		| PM_UNLOCK_ADDR((uint32_t)&PM->PM_HSBMASK - (uint32_t)PM);
	PM->PM_HSBMASK = PM_HSBMASK_USBC |	PM_HSBMASK_HFLASHC| PM_HSBMASK_HTOP0|
			PM_HSBMASK_HTOP1|PM_HSBMASK_HTOP2|PM_HSBMASK_HTOP3;

	PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAUL)
		| PM_UNLOCK_ADDR((uint32_t)&PM->PM_PBADIVMASK - (uint32_t)PM);
	PM->PM_PBADIVMASK = PBA_DIVMASK_TIMER_CLOCK2
		| PBA_DIVMASK_TIMER_CLOCK3
		| PBA_DIVMASK_TIMER_CLOCK4
		| PBA_DIVMASK_TIMER_CLOCK5;

	ui_init();

#if TRY_XTAL
	//Erase the entire application area if part is secured
	if(flashcalw_is_security_bit_active())
	{
		//Tested ok
		erase_secured_device();
	}
#endif
	//We have determined we should stay in the monitor,
	//Start SAM-BA init

	//Switch to RCFAST
	osc_enable(OSC_ID_RCFAST);
	osc_wait_ready(OSC_ID_RCFAST);
	sysclk_set_source(SYSCLK_SRC_RCFAST);

    // Check if we are here because of GPIO pin
    if (_estack.pin) {
        // If the pin is held active for 5 seconds, stay in bootloader.
        // Otherwise, start the application in firmware update/configuration
        // mode.
        int i = 5000;
        int inactive_count = 0;
        do {
            if ((_estack.port->GPIO_PVR & _estack.pin) == _estack.pin_state)
				inactive_count = 0;
			else
                inactive_count++;
            if (inactive_count == 2) {
                // Pin released before 5 seconds had elapsed.
                // Set the flag and start the app.
                ui_stop();
                CONFIG_FW_REG = CONFIG_FW_MASK;             // config mode flag
                __set_MSP(*(uint32_t *) APP_START_ADDRESS); // app stack pointer
                asm ("bx %0" :: "r" (*(uint32_t *) (APP_START_ADDRESS + 4)));
            }
			wait_1_ms();
        } while (--i);
    }

#if AT25DFX_MEM
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

#if TRY_XTAL
	// Place XIN and XOUT as input before enabling SCIF.OSC0
	ioport_set_pin_dir(PIN_PA00, IOPORT_DIR_INPUT);
	ioport_set_pin_dir(PIN_PA01, IOPORT_DIR_INPUT);
#endif

	//Check available clock source for usb operation
	usb_clock_setup();
	if(b_usb_clock_available)
	{
		//Enable pins for USB
		ioport_set_pin_mode(PIN_PA25A_USBC_DM, MUX_PA25A_USBC_DM);
		ioport_disable_pin(PIN_PA25A_USBC_DM);
		ioport_set_pin_mode(PIN_PA26A_USBC_DP, MUX_PA26A_USBC_DP);
		ioport_disable_pin(PIN_PA26A_USBC_DP);
		// Start USB stack
		udc_start();
	}

	b_usb_selected = false;

    // The main loop manages only the power mode
    // because the USB management is done by interrupt
    while (true) {

        if (main_b_msc_enable) {
            if (!udi_msc_process_trans()) {
                //sleepmgr_enter_sleep();
            }
        }else{
            //sleepmgr_enter_sleep();
        }
    }
}

void main_suspend_action(void)
{
}

void main_resume_action(void)
{
}

void main_sof_action(void)
{
    if (!main_b_msc_enable)
        return;
    //ui_process(udd_get_frame_number());
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

// Settings for Atmel coding style:
// tab step is 4, indentation 1 tab.
// vim:sw=4:ts=4:noet
