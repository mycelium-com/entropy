/*
 * Parser for settings.txt.
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

#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "lib/fwsign.h"

int parse_settings(struct Raw_public_key keys[], int len);

extern struct Settings {
    uint8_t coin;
    bool    compressed;
    uint8_t salt_type;
    uint8_t salt_len;
    uint8_t salt[32];
} settings;

extern const struct Raw_public_key mycelium_public_key;

#endif
