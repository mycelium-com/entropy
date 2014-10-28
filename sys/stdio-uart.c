/*
 * UART configuration for stdio.
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

#include <stdio.h>
#include <serial.h>

#include "stdio-uart.h"

/*
 *  putchar which translates LF->CRLF
 */
int putchar(int c)
{
    if (c == '\n')
        usart_putchar(CONF_UART, '\r');
    usart_putchar(CONF_UART, c);
    return c;
}

/*
 *  Initialise UART.
 */
void stdio_uart_init(void)
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

    /* Configure USART. */
    usart_serial_init(CONF_UART, (usart_serial_options_t *) &uart_options);
}
