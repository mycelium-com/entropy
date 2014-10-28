// SHA-256
// Public domain.

#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

enum {
    SHA256_SIZE         = 32,
    SHA256_BLOCK_SIZE   = 64,
};

// High level interface.
void sha256_hash(uint32_t hash[8], const uint8_t *data, int len);
void sha256_twice(uint32_t hash[8], const uint8_t *data, int len);

// Interface for processing the message in chunks of SHA256_BLOCK_SIZE bytes.
// Initialise internal state.
void sha256_init(uint32_t hash[8]);
// Transform one block and update state.
void sha256_transform(uint32_t hash[8], const uint32_t data[16]);
// Update state with the remaining portion of the message and finalise the
// resulting hash.
// Total length of the entire message must be provided, as it will be appended
// at the end according to the specification.
void sha256_finish(uint32_t hash[8], const uint8_t *data, int len, int total);

#endif
