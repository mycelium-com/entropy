/*
 * RAM-loadable helper applet.
 * This applet loads image from serial flash into either RAM or on-chip flash,
 * depending on the image type.
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

#include <at25dfx.h>
#include <flashcalw.h>
#include <led.h>
#include "../lib/fwsign.h"
#include "applet.h"

void applet(void);

struct Applet_param applet_param __attribute__ ((section (".param")));

__attribute__ ((section (".entry")))
void applet(void)
{
    enum {
        FWIN = 16           // size of FAT window in clusters
    };
    unsigned fcl = ~0;      // first cluster loaded into FAT window
    uint8_t fat[FWIN + FWIN / 2];
    unsigned cl = applet_param.file_scl;
    uint32_t left = applet_param.file_size;
    uint32_t * const task_start = (uint32_t *) 0x20000000;
    uint8_t *mem = (uint8_t *) task_start;

    if (applet_param.ftype == FW_MAGIC_ENTROPY) {
        // erase all application area
        int page = flashcalw_get_page_count();
        do {
            --page;
            if (!flashcalw_quick_page_read(page))   // check if already erased
                flashcalw_erase_page(page, false);
        } while (page != 32);
    }

    for (;;) {
        uint32_t len = left < applet_param.csize ? left : applet_param.csize;
        at25dfx_read(mem, len,
                     applet_param.data_addr + (cl - 2) * applet_param.csize);
        if (applet_param.ftype == FW_MAGIC_ENTROPY)
            flashcalw_memcpy((void *) (applet_param.file_size - left + 0x4000),
                             mem, len, false);
        else
            mem += len;

        left -= len;
        if (!left)
            break;

        // continue blinking the LED
        if (((uintptr_t) mem & 16383) == 0)
            LED_Toggle(LED0);

        // get next cluster number
        if (cl < fcl || cl >= fcl + FWIN) {
            // shift window
            fcl = cl & ~1;
            at25dfx_read(fat, sizeof fat, applet_param.fat_addr + fcl + fcl/2);
        }
        unsigned i = cl - fcl;
        i += i / 2;
        i = fat[i] | fat[i + 1] << 8;
        cl = cl & 1 ? i >> 4 : i & 0xfff;
    }

    LED_Off(LED0);

    if (applet_param.ftype != FW_MAGIC_ENTROPY) {
        // pass control to the loaded task
        __set_MSP(task_start[0]);
        asm ("bx %0" :: "r" (task_start[1]));
    }

    for (;;);
}
