/*
 * Debug routines.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#include <stdio.h>
#include <stdint.h>
#include "debug.h"

void debug_dump_mem(unsigned addr, const void *mem, unsigned len)
{
    unsigned limit = addr + len;
    const uint8_t *p = mem;
    int i;
    while (addr < limit) {
        printf("%08x:", addr);
        for (i = 0; i < 16; ++i)
            printf("%s %02x", (i & 3) ? "" : " ", p[i]);
        printf("  ");
        for (i = 0; i < 16; ++i)
            putchar((p[i] < ' ' || p[i] >= 0x7f) ? '.' : p[i]);
        putchar('\n');
        p += 16;
        addr += 16;
    }
}
