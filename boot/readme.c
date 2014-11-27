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

const char readme[1024] __attribute__ ((aligned (4))) =
"                          MYCELIUM ENTROPY BOOTLOADER\r\n"
"\r\n"
"Your Mycelium Entropy device is in firmware recovery mode.\r\n"
"\r\n"
"This mode is UNSAFE due to lack of signature checks.  It is intended solely for\r\n"
"unbricking, when the main firmware is damaged and normal firmware update and\r\n"
"configuration mode is not working.\r\n"
"\r\n"
"To perform recovery:\r\n"
"1. Copy new firmware to the firmware.bin file here using command line, e.g.:\r\n"
"     cp new.bin /Volumes/BOOTLOADER/firmware.bin           (OS X);\r\n"
"     cp new.bin /media/username/BOOTLOADER/firmware.bin    (Ubuntu); or\r\n"
"     copy/b new.bin X:firmware.bin                         (Windows;\r\n"
"                                          substitute this drive's letter for X).\r\n"
"2. Instruct your computer to eject, unmount, or safely remove BOOTLOADER volume.\r\n"
"3. Unplug Mycelium Entropy when safe.\r\n"
"\r\n"
"\r\n"
"[Bootloader version " VERSION "]\r\n";
