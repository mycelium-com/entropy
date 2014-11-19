/*
 * Application information based on the firmware signature structure.
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
#include <endian.h>
#include "fwsign.h"

// Build timestamp, generated during linking.
extern const char build_ts[];

void appname(const char *name)
{
    const struct Firmware_signature * signature = firmware_signature();

    printf("\n-- %s", name);
    if (signature) {
        printf(" %s-%08lx", signature->version_name,
                (unsigned long) be32_to_cpu(signature->hash.w[0]));
        if (*signature->flavour_name)
            printf(" (%s)", signature->flavour_name);
    }
    printf(" // %s --\n", build_ts);
}
