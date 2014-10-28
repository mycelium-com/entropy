/**
 * \file
 *
 * \brief Mycelium Entropy 1.0 board definition
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
 * Adapted for Mycelium Entropy by Mycelium SA, Luxembourg.
 * Mycelium SA have waived all copyright and related or neighbouring rights
 * to their changes to this file and placed these changes in public domain.
 */

#ifndef PLATFORMS_USER_BOARD_H_INCLUDED
#define PLATFORMS_USER_BOARD_H_INCLUDED

#include <conf_board.h>

/**
 * \ingroup group_common_boards
 * \defgroup sam4l_xplained_pro_group SAM4L Xplained Pro board
 *
 * @{
 */

/**
 * \defgroup sam4l_xplained_pro_config_group Configuration
 *
 * Symbols to use for configuring the board and its initialization.
 *
 * @{
 */
#ifdef __DOXYGEN__

//! \name Initialization
//@{

/**
 * \def CONF_BOARD_KEEP_WATCHDOG_AT_INIT
 * \brief Let watchdog remain enabled
 *
 * If this symbol is defined, the watchdog is left running with its current
 * configuration. Otherwise, it gets disabled during board initialization.
 */
# ifndef CONF_BOARD_KEEP_WATCHDOG_AT_INIT
#  define CONF_BOARD_KEEP_WATCHDOG_AT_INIT
# endif

//@}

#endif // __DOXYGEN__
/** @} */

/**
 * \defgroup sam4l_xplained_pro_features_group Features
 *
 * Symbols that describe features and capabilities of the board.
 *
 * @{
 */

//! Name string macro
#define BOARD_NAME          "Mycelium Entropy"
#define MCU_SOC_NAME        "ATSAM4LS4A"

// Hardware platform family code, used in firmware signature block
// for compatibility checking (see lib/fwsign.h)
#define BOARD_HW_CODE       HW_FAMILY_ME1

//! \name Board oscillator definitions
// No external or crystal oscillators.

//! \name LED definitions
//@{
// LED1 and LED2 are the two LEDs on the production board ME 1.0.
#define LED1                      (1 << 21)
#define LED1_PIN                  PIN_PA21
#define LED1_GPIO                 PIN_PA21
#define LED1_ACTIVE               true
#define LED1_INACTIVE             !LED1_ACTIVE
#define LED2                      (1 << 22)
#define LED2_PIN                  PIN_PA22
#define LED2_GPIO                 PIN_PA22
#define LED2_ACTIVE               true
#define LED2_INACTIVE             !LED2_ACTIVE
// LED3 corresponds to the single LED position on the prototype.
#define LED3                      (1 << 17)
#define LED3_PIN                  PIN_PA17
#define LED3_GPIO                 PIN_PA17
#define LED3_ACTIVE               true
#define LED3_INACTIVE             !LED3_ACTIVE

#define LED_PORT                  IOPORT_GPIOA
#define LED_MASK                  (LED1 | LED2 | LED3)
#define LED_ACTIVE                LED_MASK
#define LED_INACTIVE              0

// Software mostly uses LED0 for compatibility with platforms with only one LED.
// This defines LED0 as all three LEDs simultaneously.
#define LED0                      LED_MASK
#define LED0_ACTIVE               true
#define LED0_INACTIVE             !LED0_ACTIVE
//@}

//! \name SW0 definitions
//@{
#define GPIO_PUSH_BUTTON_0        PIN_PA10 /* Wrapper definition */
#define SW0_PIN                   PIN_PA10
#define SW0_ACTIVE                false
#define SW0_INACTIVE              !SW0_ACTIVE
#define SW0_EIC_PIN               PIN_PA10B_EIC_EXTINT1
#define SW0_EIC_PIN_MUX           MUX_PA10B_EIC_EXTINT1
#define SW0_EIC_LINE              1
//@}

/**
 * \name USB pin definitions
 *
 * No USB GPIO pins are in use.
 */

//! \name USART connections to GPIO for Virtual Com Port
// @{
#define COM_PORT_USART            USART0
#define COM_PORT_USART_ID         ID_USART0
#define COM_PORT_RX_PIN           PIN_PA05B_USART0_RXD
#define COM_PORT_RX_GPIO          GPIO_PA05B_USART0_RXD
#define COM_PORT_RX_MUX           MUX_PA05B_USART0_RXD
#define COM_PORT_TX_PIN           PIN_PA07B_USART0_TXD
#define COM_PORT_TX_GPIO          GPIO_PA07B_USART0_TXD
#define COM_PORT_TX_MUX           MUX_PA07B_USART0_TXD
// @}

/**
 * \name Button #0 definitions
 *
 * Wrapper macros for SW0, to ensure common naming across all Xplained Pro
 * boards.
 */
//@{
#define BUTTON_0_NAME             "SW0"
#define BUTTON_0_PIN              SW0_PIN
#define BUTTON_0_ACTIVE           SW0_ACTIVE
#define BUTTON_0_INACTIVE         SW0_INACTIVE
#define BUTTON_0_EIC_PIN          SW0_EIC_PIN
#define BUTTON_0_EIC_PIN_MUX      SW0_EIC_PIN_MUX
#define BUTTON_0_EIC_LINE         SW0_EIC_LINE

/* Definitions for SW0 when using it as EIC pin */
#if 0
#define GPIO_PUSH_BUTTON_EIC_PIN        PIN_PC24B_EIC_EXTINT1
#define GPIO_PUSH_BUTTON_EIC_PIN_MASK   GPIO_PC24B_EIC_EXTINT1
#define GPIO_PUSH_BUTTON_EIC_PIN_MUX    MUX_PC24B_EIC_EXTINT1
#define GPIO_PUSH_BUTTON_EIC_LINE       1
#define GPIO_PUSH_BUTTON_EIC_IRQ        EIC_1_IRQn
#endif
//@}

//! Number of on-board buttons
#define BUTTON_COUNT              1

//! @}

/** @} */

/** @} */

#endif /* PLATFORMS_USER_BOARD_H_INCLUDED */
