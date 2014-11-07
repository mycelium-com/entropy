/*
 * XXTEA cipher.
 *
 * Based on public domain implementations.
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 *
 * This implementation uses a fixed block size of 512 bytes.
 * The number of cycles is set to 16 to provide better security than the
 * suggested minimum of 6.
 */

#include "xxtea.h"

void xxtea_encrypt_block(uint32_t data[XXTEA_BLOCK_SIZE / 4], const uint32_t key[4])
{
    uint32_t sum = 0, z, y, e;
    int i = XXTEA_NUM_CYCLES, r;
    const int n = XXTEA_BLOCK_SIZE / 4 - 1;

    z = data[n];                    // left neighbour for the first round
    do {
        // cycle
        sum += 0x9e3779b9;
        e = sum >> 2;
        for (r = 0; r <= n; r++) {
            // round
            y = data[(r+1) & n];    // right neighbour
            data[r] += ((z>>5 ^ y<<2) + (y>>3 ^ z<<4)) ^ ((sum^y) + (key[(r^e) & 3] ^ z));
            z = data[r];            // left neighbour for the next round
        }
    } while (--i);
}

void xxtea_decrypt_block(uint32_t data[XXTEA_BLOCK_SIZE / 4], const uint32_t key[4])
{
    uint32_t sum, z, y, e;
    int i = XXTEA_NUM_CYCLES, r;
    const int n = XXTEA_BLOCK_SIZE / 4 -1;

    sum = (uint32_t) ((uint64_t) XXTEA_NUM_CYCLES * 0x9e3779b9ul);
    y = data[0];
    do {
        // cycle
        e = sum >> 2;
        for (r = n; r >= 0; --r) {
            // round
            z = data[(r-1) & n];
            data[r] -= ((z>>5 ^ y<<2) + (y>>3 ^ z<<4)) ^ ((sum^y) + (key[(r^e) & 3] ^ z));
            y = data[r];
        }
        sum -= 0x9e3779b9;
    } while (--i);
}
