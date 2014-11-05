/*
 * Global data.
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

#include "sss.h"
#include "layout.h"
#include "data.h"

static char address[35];
static char sss[3][SSS_STRING_SIZE];
static char unsalted[73];

char * const texts[] = {
    [IDX_ADDRESS]       = address,
    [IDX_SSS_PART(1)]   = sss[0],
    [IDX_SSS_PART(2)]   = sss[1],
    [IDX_SSS_PART(3)]   = sss[2],
    [IDX_UNSALTED]      = unsalted,
};

unsigned global_error_flags;
