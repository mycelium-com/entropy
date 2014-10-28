/*
 * Asynchronous System Timer utility routines and the now() function.
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
#include <board.h>
#include <ast.h>
#include <osc.h>

static inline void ast_init(void)
{
    static struct ast_config conf = {
        .mode = AST_COUNTER_MODE,
#ifdef BOARD_OSC32_HZ
#define AST_FREQ    (BOARD_OSC32_HZ / 2)
#define AST_PER_1HZ 14
        .osc_type = AST_OSC_32KHZ,
#else
#define AST_FREQ    (OSC_RCSYS_NOMINAL_HZ / 2)
#define AST_PER_1HZ 16
        .osc_type = AST_OSC_RC,
#endif
        .psel = 0,
    };

#ifdef BOARD_OSC32_HZ
    osc_enable(OSC_ID_OSC32);
    osc_wait_ready(OSC_ID_OSC32);
#endif
    ast_enable(AST);
    ast_set_config(AST, &conf);
}

static inline uint32_t now(void)
{
    return ast_read_counter_value(AST);
}

static inline void print_time_interval(const char *what, uint32_t interval)
{
    const uint64_t mf = ((uint64_t) 1000 << 32) / AST_FREQ;
    printf("%s: %u ms.\n", what, (unsigned) ((interval * mf + (1u << 31)) >> 32));
}
