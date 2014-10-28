/*
 * Flash wrapper for the Bootloader Replacement Tool.
 * Normally the BRT is linked to run directly from SRAM.
 * This wrapper allows BRT to be run from flash:
 * it copies everything into SRAM and jumps there.
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

#include <stdint.h>
#include <exceptions.h>

extern uint32_t _stext_in_flash;
extern uint32_t _sfixed;
extern uint32_t _erelocate;
extern uint32_t _estack;
extern uint32_t __ram_end__;

void flash_startup(void);

__attribute__ ((section(".start.vectors")))
IntFunc flash_exception_table[] = {
    (IntFunc) (0x20000100),     // initial stack pointer
    flash_startup,              // reset vector (entry point)
    0, 0, 0, 0, 0, 0,           // unused or reserved vectors
    0                           // reserved for offset to signature block
};

__attribute__ ((section(".start.text")))
void flash_startup(void)
{
    uint32_t *src = &_stext_in_flash;
    uint32_t *dst = &_sfixed;
    IntFunc ram_entry_point = (IntFunc)src[1];

    do
        *dst++ = *src++;
    while (dst < &_erelocate);

    __ram_end__ = 0;
    __set_MSP((uintptr_t)&_estack);
    ram_entry_point();
}
