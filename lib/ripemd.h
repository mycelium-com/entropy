// RIPEMD-160
// Public domain.

#ifndef RIPEMD_H
#define RIPEMD_H

#include <stdint.h>

enum {
    RIPEMD160_SIZE  = 20,
};

void ripemd160_hash(uint32_t hash[5], const uint8_t *data, int len);

#endif
