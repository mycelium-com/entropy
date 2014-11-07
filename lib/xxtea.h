/*
 * XXTEA cipher.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#ifndef LIB_XXTEA_H
#define LIB_XXTEA_H

#include <stdint.h>

enum {
    XXTEA_BLOCK_SIZE = 512, // in bytes, power of 2
    XXTEA_NUM_CYCLES = 16,  // min. is 6 + 52/n, where n is block size in words
};

void xxtea_encrypt_block(uint32_t data[XXTEA_BLOCK_SIZE / 4], const uint32_t key[4]);
void xxtea_decrypt_block(uint32_t data[XXTEA_BLOCK_SIZE / 4], const uint32_t key[4]);

#endif
