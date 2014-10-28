// Hexadeceimal <-> binary conversion.

#ifndef LIB_HEX_H_INCLUDED
#define LIB_HEX_H_INCLUDED

#include <stdint.h>

// Convert data to hex with spaces every four bytes.
// Return resulting string length.
int hexlify(char *buf, const uint8_t *data, int len);

// Convert hex to bin, not more than size bytes.
// Return number of bytes converted, or -1 on error.
int unhexlify(const char *hex, uint8_t *bin, int size);

#endif
