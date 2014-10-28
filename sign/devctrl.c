/*
 * Device control and UART pass-through.
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

#include <board.h>
#include <serial.h>
#include <ioport.h>
#include <gpio.h>

#include "sys/conf_system.h"
#include "devctrl.h"

#if BOARD == SAM4L_XPLAINED_PRO

// Use USART pins designated for SPI on EXT3 because of a slightly nicer pinout
// compared to the pins designated for UART.
//   19 GND
//   17 HOST RX (TARGET TX)
//   15 RESET
#define DEV_UART    EXT3_SPI_MODULE
#define RX_PIN      EXT3_PIN_SPI_MISO
#define RX_MUX      EXT3_SPI_SS_MISO
#define RST_PIN     EXT3_PIN_15
#define DEV_UART_IRQ     USART2_IRQn
#define DEV_UART_Handler USART2_Handler

static void button_interrupt(void);

void devctrl_init(void)
{
    static const usart_serial_options_t uart_options = {
        .baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
        .charlength = CONF_UART_CHAR_LENGTH,
#endif
        .paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
        .stopbits = CONF_UART_STOP_BITS,
#endif
    };

    ioport_set_pin_mode(RX_PIN, RX_MUX | IOPORT_MODE_PULLDOWN);
    ioport_disable_pin(RX_PIN);
    ioport_set_pin_level(RST_PIN, 0);

    ioport_set_pin_sense_mode(BUTTON_0_PIN, IOPORT_SENSE_BOTHEDGES);
    gpio_set_pin_callback(BUTTON_0_PIN, button_interrupt, CONFIG_BUTTON_IRQ_PRIO);
    gpio_enable_pin_interrupt(BUTTON_0_PIN);

    usart_serial_init(DEV_UART, (usart_serial_options_t *) &uart_options);
    usart_enable_interrupt(DEV_UART, US_IER_RXRDY);
    NVIC_EnableIRQ(DEV_UART_IRQ);
}

static void button_interrupt(void)
{
    if (!ioport_get_pin_level(BUTTON_0_PIN) == BUTTON_0_INACTIVE)
        ioport_set_pin_dir(RST_PIN, IOPORT_DIR_OUTPUT);
    else
        ioport_set_pin_dir(RST_PIN, IOPORT_DIR_INPUT);
}

void DEV_UART_Handler(void)
{
    uint32_t status = usart_get_status(DEV_UART);

    if (status & US_CSR_RXBRK) {
        usart_reset_status(DEV_UART);
        return;
    }

    if (status & US_CSR_RXRDY) {
        uint32_t c;

        usart_read(DEV_UART, &c);
        usart_write(CONF_UART, c);
    }
}

#else

void devctrl_init(void) {}

#endif
