// SHA-512 hash and HMAC implementation.
// Public domain.

#ifndef SHA512_H_INCLUDED
#define SHA512_H_INCLUDED

#include <stdint.h>

void sha512_hash(uint64_t hash[8], const uint8_t *data, int len);
void sha512_hmac(uint64_t hash[8],
                 const uint8_t *key, int key_len,
                 const uint8_t *data, int data_len);

#endif
