/*
 * Initial factory firmware and self test for Mycelium Entropy.
 *
 * Copyright 2014, 2015 Mycelium SA, Luxembourg.
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

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

enum Faults {
    VDD_FAULT           = 1,
    FLASH_FAULT         = 2,
    USB_FAULT           = 4,
    WRONG_FLASH         = 5,
    WRONG_MCU           = 6,
    BOOTLOADER_FAULT    = 7,
};

void set_fault(enum Faults fault);

void main_suspend_action(void);
void main_resume_action(void);
void main_sof_action(void);
bool main_msc_enable(void);
void main_msc_disable(void);

#endif
