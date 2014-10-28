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

#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED

#include "sss.h"

extern char texts[][SSS_STRING_SIZE];

extern unsigned global_error_flags;

// error flag bit masks
enum Global_error_flags {
    FLASH_ERROR     = 1 << 0,
    NO_LAYOUT_FILE  = 1 << 1,
};

#endif
