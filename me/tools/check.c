/*
 * Generate JPEG into a file to check the results.
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ff.h"
#include "lib/rs.h"
#include "data.h"
#include "jpeg.h"
#include "jpeg-data.h"
#include "keygen.h"
#include "xflash.h"
#include "rng.h"

//void jpeg_dump(void);

// Unallocated memory starts at the _estack symbol, provided by the linker to
// the embedded firmware.  sss.c temporarily uses this area for coefficients.
// Here we simulate _estack.
uint32_t _estack[16 * 256 / 4]; // max 16 shares, 256 bytes each

// Global variables expected by the embedded software.
char texts[6][SSS_STRING_SIZE];
int ui_btn_count;

// File with the contents of external flash.
static FILE *xflash_file;

enum {
    MAX_NBLK = 8000
};

// Get a random stretch of blocks to test the JPEG streaming algorithm.
static int get_random_extent(int *xend, int nblk)
{
    int start = random() % (nblk + 50) - 50;
    int n = (random() & 127) + 1;

    if (start < 0) {
        n += start;
        start = 0;
        if (n <= 0)
            return get_random_extent(xend, nblk);
    }

    if (start + n > nblk)
        n = nblk - start;

    *xend = start + n;
    printf("%d - %d\n", start, start + n - 1);
    return start;
}

static void usage(void)
{
    fputs("Usage:  check [options]\n"
          "  -t        testnet\n"
          "  -s        Shamir\n"
          "  -u        uncompressed public key\n"
          "  -r NBLK   random access test for the streaming algorithm\n"
          "            with a file size of NBLK 512-byte blocks\n"
          "  -l        Litecoin\n"
          "  -p        Peercoin\n"
          "Output is written to sample*.jpg, where * stands for "
          "option-specific suffixes.\n",
          stderr);
}

int main(int argc, char *argv[])
{
    char fname[80];
    uint8_t key[34];
    uint8_t buf[22736];
    uint8_t *bprev, *bptr = 0;
    int blk, xend;
    bool testnet = false, compressed = true, shamir = false;
    bool litecoin = false, peercoin = false;
    int nblk = 0;
    int retcode = 0;
    int i;

    static const struct {
        uint8_t coin;
        const char *suffix;
        const uint16_t *heading;
    } coins[] = {
        { BITCOIN,          "",         bitcoin_address_fragment  },
        { BITCOIN_TESTNET,  "-testnet", bitcoin_address_fragment  },
        { LITECOIN,         "-ltc",     litecoin_address_fragment },
        { PEERCOIN,         "-peer",    bitcoin_address_fragment  },    // n/a
        { DOGECOIN,         "-doge",    bitcoin_address_fragment  },    // n/a
    }, *coin;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage();
        return 1;
    }

    while ((i = getopt(argc, argv, "tsulpr:h")) != -1)
        switch (i) {
        case 't':
            testnet = true;
            break;
        case 's':
            shamir = true;
            break;
        case 'u':
            compressed = false;
            break;
        case 'l':
            litecoin = true;
            break;
        case 'p':
            peercoin = true;
            break;
        case 'r':
            nblk = strtoul(optarg, 0, 0);
            if (nblk > MAX_NBLK) {
                fprintf(stderr, "NBLK must be between 2 and %d.\n", MAX_NBLK);
                return 1;
            }
            if (nblk >= 2)
                break;
            // fall through
        default:
            usage();
            return 1;
        }

    if (litecoin && peercoin) {
        fprintf(stderr, "Cannot do different coin types at the same time.\n");
        return 1;
    }
    if (testnet && (litecoin || peercoin)) {
        fprintf(stderr, "Testnet is supported for Bitcoin only.\n");
        return 1;
    }

    coin = &coins[testnet + 2 * litecoin + 3 * peercoin];

    snprintf(fname, sizeof fname, "sample%s%s.jpg",
            coin->suffix,
            shamir ? "-sss" : "");
    FILE *f = fopen(fname, "wb");
    if (!f) {
        perror(fname);
        return 1;
    }
    xflash_file = fopen("../jpeg-data.bin", "rb");
    if (!xflash_file) {
        perror("../jpeg-data.bin");
        return 1;
    }

    srandomdev();

    // Generate key pair.
    int len = keygen(coin->coin, compressed, key);

    // Set address heading according to coin type.
    bitcoin_address_ref = coin->heading;

    if (shamir) {
        rs_init(0x11d, 1);  // initialise GF(2^8) for Shamir
        sss_encode(2, 3, SSS_BASE58, key, len);
        jpeg_init(buf, buf + sizeof buf, shamir_layout);
    } else {
        jpeg_init(buf, buf + sizeof buf, main_layout);
    }

    if (nblk) {
        // random access test of the JPEG streaming algorithm
        bool blkusage[nblk];
        uint8_t out[nblk][512];

        memset(blkusage, 0, sizeof blkusage);
        do {
            for (blk = get_random_extent(&xend, nblk); blk != xend; blk++) {
                bptr = jpeg_get_block(blk);
                if (blkusage[blk]) {
                    if (memcmp(bptr, out[blk], 512) != 0) {
                        printf("Error at block %d.\n", blk);
                        //jpeg_dump();
                        retcode = 2;
                    }
                } else {
                    memcpy(out[blk], bptr, 512);
                    blkusage[blk] = true;
                    fseek(f, blk << 9, SEEK_SET);
                    fwrite(bptr, 512, 1, f);
                }
            }
        } while (memcmp(blkusage, blkusage + 1, sizeof blkusage - 1) != 0);
        printf("%s: written.\n", fname);
    } else {
        // generate JPEG sequentially until EOF
        blk = 0;
        do {
            bprev = bptr;
            bptr = jpeg_get_block(blk++);
            fwrite(bptr, 512, 1, f);
        } while (bprev != bptr);
        printf("%s: %d blocks.\n", fname, blk);
    }

    if (fclose(f)) {
        perror(fname);
        retcode = 3;
    }
    fclose(xflash_file);

    return retcode;
}

// Simulate reading graphics data from external flash.
void xflash_read(uint8_t *data, uint16_t size, uint32_t address)
{
    fseek(xflash_file, address, SEEK_SET);
    fread(data, 1, size, xflash_file);
}

// Simulate RNG.
void rng_next(uint32_t random_number[8])
{
    int i;

    for (i = 0; i < 8; i++)
        random_number[i] = random();
}

// Fatfs stubs.
FRESULT f_mount (BYTE d, FATFS *f) { return FR_OK; }
FRESULT f_open (FIL *f, const TCHAR *n, BYTE a) { return FR_OK; }
FRESULT f_lseek (FIL *f, DWORD address)
{
    if (fseek(xflash_file, address, SEEK_SET) < 0)
        return FR_DISK_ERR;
    return FR_OK;
}
FRESULT f_read (FIL *f, void *buf, UINT len, UINT *actual)
{
    *actual = fread(buf, 1, len, xflash_file);
    return FR_OK;
}
