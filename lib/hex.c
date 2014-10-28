/*
 * Hexadeceimal <-> binary conversion.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#include <stdio.h>
#include "hex.h"

int hexlify(char *buf, const uint8_t *data, int len)
{
    int n = 0;

    for (; len; --len) {
        if ((len & 3) == 0)
            buf[n++] = ' ';
        n += sprintf(buf + n, "%02X", *data++);
    }

    buf[n] = 0;

    return n;
}

int unhexlify(const char *hex, uint8_t *bin, int size)
{
    unsigned byte = 1;
    unsigned char c;
    int n = 0;

    while ((c = *hex++) != 0) {
        if (c <= ' ')
            continue;
        c |= 040;   // tolower
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
            c -= 'a' - 10;
        else
            return -1;      // bad hex character
        if (n == size)
            return -1;      // output buffer size exceeded
        byte = byte << 4 | c;
        if (byte & 0x100) {
            *bin++ = byte;
            byte = 1;
            n++;
        }
    }

    // if we ended up with a whole number of bytes, return the number of bytes;
    // otherwise, indicate error
    return byte == 1 ? n : -1;
}
