/*
 * Firmware signature structure.
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

#ifndef LIB_FWSIGN_H_INCLUDED
#define LIB_FWSIGN_H_INCLUDED

#include <stdint.h>

struct Raw_public_key {
    uint8_t x[32];
    uint8_t y[32];
};

struct Firmware_signature {
    uint32_t magic;
    uint16_t version_code;
    uint16_t flavour_code;
    uint8_t  hardware_family;
    uint8_t  hardware_code[3];
    uint32_t reserved1;
    char     version_name[16];
    char     flavour_name[16];
    char     hardware_name[32];
    uint8_t  reserved2[16];
    union {
        uint8_t  b[32];
        uint32_t w[8];
    } hash;
    struct Raw_public_key pubkey;
    struct {
        uint8_t r[32];
        uint8_t s[32];
    } signature;
};

enum {
    FW_MAGIC_ENTROPY    = 0x59504e45,   // 'ENPY'
    FW_MAGIC_LAYOUT     = 0x5459414c,   // 'LAYT'
    FW_MAGIC_TASK       = 0x4b534154,   // 'TASK'
    FW_MAGIC_BOOT       = 0x544f4f42,   // 'BOOT'

    FW_FLAVOUR_REGULAR  = 0,
    FW_FLAVOUR_MYCELIUM = 1,

    // hardware_family byte
    HW_FAMILY_XPRO      = 0,    // Atmel SAM4L Xtended Pro board
    HW_FAMILY_ME1       = 1,    // Mycelium Entropy platform
    // hardware_code[0]
    HW_SAM4L2           = 0,    // SAM4L chip with 128 kB flash
    HW_SAM4L4           = 1,    // SAM4L chip with 256 kB flash
    // hardware_code[1]
    HW_XSFL_NONE        = 0,    // External serial flash not required
    HW_XSFL_LSECT       = 1,    // External serial flash with 64 kB erase only
    HW_XSFL_SSECT       = 2,    // External serial flash with 4 kB erase
    // hardware_code[2]
    HW_XSFL_512K        = 3,    // External serial flash size code
    HW_XSFL_1M          = 4,
    HW_XSFL_2M          = 5,
    HW_XSFL_4M          = 6,

    FW_SIGN_VECTOR      = 8,    // signature vector in the exception table
};

// exception_table is actually an array of IntFunc, but it is not declared
// anywhere, and we only need it to access the signature vector
extern const uint32_t exception_table[];

static inline const struct Firmware_signature * firmware_signature(void)
{
    uint32_t offset = exception_table[FW_SIGN_VECTOR];

    if (!offset)
        return 0;

    return (const struct Firmware_signature *) (exception_table + offset / 4);
}

// Print application's build information.
void appname(const char *name);

#endif
