/*
 * Resource allocation and system configuration.
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

#ifndef CONF_SYSTEM_H_INCLUDED
#define CONF_SYSTEM_H_INCLUDED

// Timer-counter instance and channel used for frequency tracking.
#define TC_SYNC         TC0
#define TC_SYNC_CHANNEL 0

// Timer-counter instance and channel for the user interface.
#define TC_UI           TC0
#define TC_UI_CHANNEL   2
#define TC_UI_Handler   TC02_Handler
#define TC_UI_IRQn      TC02_IRQn
#define TC_UI_IRQ_PRIO  2

enum {
    CONFIG_ADC_IRQ_PRIO         = 5,
    CONFIG_AST_IRQ_PRIO         = 1,
    CONFIG_BUTTON_IRQ_PRIO      = 3,
};

// Firmware configuration mode flag.
// We use GPIO Output Driving Capability Register 0 for pin PA00.
// This pin is multiplexed with XIN0 and is not used in Mycelium Entropy.
// It is connected to the OSC0 crystal on the evaluation board.
#define CONFIG_FW_REG           (GPIO->GPIO_PORT[0].GPIO_ODCR0)
#define CONFIG_FW_REG_CLR       (GPIO->GPIO_PORT[0].GPIO_ODCR0C)
#define CONFIG_FW_MASK          1

#endif
