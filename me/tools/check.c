/*
 * Generate JPEG into a file to check the results.
 *
 * Copyright 2014, 2015 Mycelium SA, Luxembourg.
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
#include "lib/hex.h"
#include "data.h"
#include "jpeg.h"
#include "jpeg-data.h"
#include "keygen.h"
#include "sss.h"
#include "settings.h"
#include "rng.h"

//void jpeg_dump(void);

// Unallocated memory starts at the _estack symbol, provided by the linker to
// the embedded firmware.  sss.c temporarily uses this area for coefficients,
// and hd.c for the word list.
// Here we simulate _estack.
uint32_t _estack[5 * 2048 / 4];

// Global variables expected by the embedded software.
int ui_btn_count;
struct Settings settings;

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

static bool check_layout(const struct Layout *layout, const char *name)
{
    unsigned height = 0;

    do {
        // skip fragments whose condition is false
        while (layout_conditions[layout->cond_idx] != layout->cond_val)
            layout++;
        height += layout->vstep;
    } while (layout++->type != FGM_STOP);

    if (height != JHEIGHT)
        printf("%s layout error: height %u, must be %d.\n", name, height,
                JHEIGHT);
    return height == JHEIGHT;
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
          "  -1        use type 1 salt\n"
          "  -d path   HD wallet with xpub at path (-dd for default)\n"
          "Output is written to sample*.jpg, where * stands for "
          "option-specific suffixes.\n",
          stderr);
}

int main(int argc, char *argv[])
{
    char fname[80];
    uint8_t key[34];
    uint8_t buf[24576];
    uint8_t *bprev, *bptr = 0;
    int blk, xend;
    bool testnet = false, shamir = false;
    bool litecoin = false, peercoin = false;
    int nblk = 0;
    int retcode = 0;
    int i;

    static const struct {
        uint16_t coin;
        const char *suffix;
    } coins[] = {
        { BITCOIN,          ""         },
        { BITCOIN_TESTNET,  "-testnet" },
        { LITECOIN,         "-ltc"     },
        { PEERCOIN,         "-peer"    },    // n/a
        { DOGECOIN,         "-doge"    },    // n/a
    }, *coin;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage();
        return 1;
    }

    settings.compressed = true;

    while ((i = getopt(argc, argv, "tsulp1d:r:h")) != -1)
        switch (i) {
        case 't':
            testnet = true;
            break;
        case 's':
            shamir = true;
            break;
        case 'u':
            settings.compressed = false;
            break;
        case 'l':
            litecoin = true;
            break;
        case 'p':
            peercoin = true;
            break;
        case '1':
            settings.salt_type = 1;
            break;
        case 'd':
            settings.hd = true;
            strncpy(settings.hd_path, *optarg == 'd' ? "" : optarg,
                    sizeof settings.hd_path);
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
    if (settings.hd && !settings.compressed) {
        fprintf(stderr, "HD wallets work with compressed keys only.\n");
        return 1;
    }
    if (settings.hd_path[sizeof settings.hd_path - 1] != 0) {
        fprintf(stderr, "HD path is too long.\n");
        return 1;
    }

    coin = &coins[testnet + 2 * litecoin + 3 * peercoin];
    settings.coin.type = coin->coin;

    // set up coin condition for the layout;
    // treat BITCOIN_TESTNET same as BITCOIN
    if (settings.coin.bip44 == COIN_BIP44(BITCOIN_TESTNET))
        layout_conditions[COND_COIN] = COIN_BIP44(BITCOIN);
    else
        layout_conditions[COND_COIN] = settings.coin.bip44;
    layout_conditions[COND_SALT] = settings.salt_type;

    snprintf(fname, sizeof fname, "sample%s%s%s%s.jpg",
            settings.hd ? "-hd" : "",
            coin->suffix,
            shamir ? "-sss" : "",
            settings.salt_type ? "-salt" : "");
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

    if (settings.salt_type) {
        settings.salt_len = 4;
        *(uint32_t *) settings.salt = random();
        hexlify(buf, settings.salt, settings.salt_len);
        printf("Salt:%s.\n", buf);
    }

    check_layout(main_layout, "Main");
    check_layout(shamir_layout, "Shamir's");
    check_layout(hd_layout, "HD");

    if (settings.hd) {
        uint64_t seed[8];
        hd_gen_seed_with_mnemonic(16, seed, texts[IDX_PRIVKEY]);
        if (!hd_make_xpub((const uint8_t *) seed, sizeof seed)) {
            fprintf(stderr, "Error in HD path.");
            return 1;
        }
        jpeg_init(buf, buf + sizeof buf, hd_layout);
    } else if (shamir) {
        int len;
        rs_init(0x11d, 1);  // initialise GF(2^8) for Shamir
        len = keygen(key);  // generate regular key pair
        sss_encode(2, 3, SSS_BASE58, key, len);
        jpeg_init(buf, buf + sizeof buf, shamir_layout);
    } else {
        keygen(key);        // generate regular key pair
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
