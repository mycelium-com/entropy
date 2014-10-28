/**
 * \file
 *
 * \brief  Configuration File for SAM4L Xplained Pro Board.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
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

#ifndef CONF_BOARD_H_INCLUDED
#define CONF_BOARD_H_INCLUDED

// Enable Com Port.
#define CONF_BOARD_COM_PORT

// Enable USB interface.
#define CONF_BOARD_USB_PORT

// Enable LCD backlight.
#define CONF_BOARD_BL

#define LCD_BL                          PC05
#define LCD_BL_GPIO                     PIN_PC05
#define LCD_BL_GPIO_MASK                GPIO_PC05
#define LCD_BL_ACTIVE_LEVEL             IOPORT_PIN_LEVEL_HIGH
#define LCD_BL_INACTIVE_LEVEL           IOPORT_PIN_LEVEL_LOW

// Enable SD MMC interface pins through SPI.
#define CONF_BOARD_SD_MMC_SPI

// Enable SPI on EXT2.
#define CONF_BOARD_SPI
#define CONF_BOARD_SPI_NPCS2

// Enable DATA/CMD and RESET pins for OLED.
#define CONF_BOARD_OLED_UG_2832HSWEG04

// Enable TWIM0.
#define CONF_BOARD_TWIMS0

// Temperature sensor AT30TSE75x on TWIM0.
#define TEMP_TWIM       TWIM0
#define TEMP_TWIM_SPEED TWI_STD_MODE_SPEED
#define TEMP_TWIM_IRQ   TWIM0_IRQn

// LED1 on IO1 extension board.
#define LED1                      PIN_PC00
#define LED1_PIN                  PIN_PC00 /* Wrapper definition */
#define LED1_GPIO                 PIN_PC00 /* Wrapper definition */
#define LED1_ACTIVE               false
#define LED1_INACTIVE             !LED1_ACTIVE

#endif  /* CONF_BOARD_H_INCLUDED */
