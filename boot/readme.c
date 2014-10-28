/*
 * readme.txt
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

#include "version.h"

const char readme[512] __attribute__ ((aligned (4))) =
"                   MYCELIUM ENTROPY EMERGENCY RECOVERY MODE\r\n"
"\r\n"
"Your Mycelium Entropy device is in firmware recovery mode.\r\n"
"\r\n"
"To perform recovery:\r\n"
"1. Copy new firmware to the firmware.bin file here, using command line, e.g.:\r\n"
"     cp new.bin /Volumes/RECOVERY/firmware.bin    (OS X) or\r\n"
"     copy/b new.bin D:firmware.bin    (Windows; drive letter may vary).\r\n"
"2. Instruct your computer to eject, unmount, or safely remove RECOVERY volume.\r\n"
"3. Unplug Mycelium Entropy when safe.\r\n"
"\r\n"
"\r\n"
"[Bootloader version " VERSION "]\r\n";
