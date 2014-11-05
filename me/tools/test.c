/*
 * Tests.
 * Usage:  ./test | ./test.py
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <ff.h>
#include "lib/rs.h"
#include "layout.h"
#include "lib/sha256.h"
#include "lib/ripemd.h"
#include "lib/sha512.h"
#include "lib/base58.h"
#include "settings.h"
#include "data.h"
#include "keygen.h"

extern uint32_t _estack[1024 * 15 / 4];
extern uint32_t __ram_end__;

asm (".section .bss, \"w\"\n.globl __estack, ___ram_end__");
asm ("__estack: .space 1024 * 15\n___ram_end__: .space 4");

char texts[6][SSS_STRING_SIZE];

// for simulated fatfs
static const uint8_t *settings_txt;
static unsigned settings_bytes_left;

static void unhexlify(const char *hex, uint8_t *bin)
{
    unsigned byte = 1;
    unsigned char c;

    while ((c = *hex++) != 0) {
        if (c <= ' ')
            continue;
        c |= 040;   // tolower
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
            c -= 'a' - 10;
        else {
            puts("Bad hex!");
            abort();
        }
        byte = byte << 4 | c;
        if (byte & 0x100) {
            *bin++ = byte;
            byte = 1;
        }
    }

    assert(byte == 1);
}

#if 0

int main(int argc, char *argv[])
{
    int i, x;

    rs_init(0x11d, 1);

    for (i = 0; i < sizeof _estack / sizeof _estack[0]; i++)
        _estack[i] = random();

    for (i = 2; i <= 5; i++) {
        bool test_net = i & 1;
        bool compressed = !(i & 2);

        __ram_end__ = random();
        keygen(test_net, compressed);
        printf("Key: %s\nThreshold: %d\n", texts[IDX_PRIVKEY], i);
        printf("Type: %snet %scompressed\n", test_net ? "test" : "main",
               compressed ? "" : "un");

        sss_encode(test_net, compressed, i, 5);

        for (x = 1; x <= 5; x++)
            printf("Share: %d %s\n", x, texts[IDX_SSS_PART(x)]);
        putchar('\n');
    }

    return 0;
}
#endif

static void print_hex(const char *tag, const uint8_t *data, int len)
{
    int i;

    printf("%s: %d ", tag, len);
    for (i = 0; i < len; i++)
        printf("%02x", data[i]);
    putchar('\n');
}

static void gen_hash(int size)
{
    int i, j, k;

    union {
        uint8_t  b[64];
        uint32_t w32[8];
        uint64_t w64[8];
    } hash;
    uint8_t data[300 * 13];

    for (i = 0; i < 300; i++) {
        int len = i <= 260 ? i : i * 13;
        for (j = 0; j < (i && i <= 132 ? 3 : 1); j++) {
            for (k = 0; k < len; k++)
                data[k] = j == 0 ? random() : j == 1 ? 0 : 0xFF;

            switch (size) {
            case 160:
                ripemd160_hash(hash.w32, data, len);
                break;
            case 256:
                sha256_hash(hash.w32, data, len);
                break;
            case 512:
                sha512_hash(hash.w64, data, len);
                break;
            default:
                abort();
            }

            print_hex("Message", data, len);
            print_hex("Digest", hash.b, size / 8);
            printf("%s-%d\n\n", size == 160 ? "RIPEMD" : "SHA", size);
        }
    }
}

static void gen_hmac512(void)
{
    int i, j, k;

    union {
        uint8_t  b[64];
        uint64_t w[8];
    } hash;
    uint8_t key[128];
    uint8_t msg[128];

    for (i = 0; i < 128; i++)
        for (j = 5; j <= 128; j += 128-5) {
            for (k = 0; k < i; k++)
                key[k] = random();
            for (k = 0; k < j; k++)
                msg[k] = random();
            sha512_hmac(hash.w, key, i, msg, j);

            print_hex("Key", key, i);
            print_hex("Message", msg, j);
            print_hex("Hmac", hash.b, sizeof hash.b);
            puts("HMAC/SHA-512\n");
        }
}

/*
 * Test vectors are from trezor-crypto:
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 */

static const char public_keys[][2][65] = {
    { "26659c1cf7321c178c07437150639ff0c5b7679c7ea195253ed9abda2e081a37",
      "0000000000000000000000000000000000000000000000000000000000000000" },
    { "5b1654a0e78d28810094f6c5a96b8efb8a65668b578f170ac2b1f83bc63ba856",
      "0000000000000000000000000000000000000000000000000000000000000000" },
    { "433f246a12e6486a51ff08802228c61cf895175a9b49ed4766ea9a9294a3c7fe",
      "0000000000000000000000000000000000000000000000000000000000000001" },
    { "aeb03abeee0f0f8b4f7a5d65ce31f9570cef9f72c2dd8a19b4085a30ab033d48",
      "0000000000000000000000000000000000000000000000000000000000000001" },
    { "96e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a0160",
      "54201c025e2a5d96b1629b95194e806c63eb96facaedc733b1a4b70ab3b33e3a" },
    { "98010f8a687439ff497d3074beb4519754e72c4b6220fb669224749591dde416",
      "f3961f8ece18f8689bb32235e436874d2174048b86118a00afbd5a4f33a24f0f" },
};

static void test_base58(void)
{
    static const struct {
        bool compressed;
        uint8_t coin;
        const char address[35];
    } tests[][2] = {
        { {  true,   0, "139MaMHp3Vjo8o4x8N1ZLWEtovLGvBsg6s" },
          {  true, 111, "mhfJsQNnrXB3uuYZqvywARTDfuvyjg4RBh" } },
        { {  true,   0, "19Ywfm3witp6C1yBMy4NRYHY2347WCRBfQ" },
          {  true,  48, "LTmtvyMmoZ49SpfLY73fhZMJEFRPdyohKh" } },
        { {  true,   0, "1FWE2bn3MWhc4QidcF6AvEWpK77sSi2cAP" },
          {  true,  52, "NB5bEFH2GtoAawy8t4Qk8kfj3LWvQs3MhB" } },
        { {  true,   0, "1yrZb8dhdevoqpUEGi2tUccUEeiMKeLcs"  },
          {  true, 111, "mgVoreDcWf6BaxJ5wqgQiPpwLEFRLSr8U8" } },
        { { false,   0, "194SZbL75xCCGBbKtMsyWLE5r9s2V6mhVM" },
          { false, 111, "moaPreR5tydT3J4wbvrMLFSQi9TjPCiZc6" } },
        { { false,   0, "1A2WfBD4BJFwYHFPc5KgktqtbdJLBuVKc4" },
          { false,  48, "LUFTvPWtFxVzo5wYnDJz2uueoqfcMYiuxH" } },
    };

    unsigned i, j;

    for (i = 0; i < sizeof public_keys / sizeof public_keys[0]; i++) {
        curve_point pub;
        uint8_t buf[32];
        char str[35];

        unhexlify(public_keys[i][0], buf);
        bn_read_be(buf, &pub.x);
        unhexlify(public_keys[i][1], buf);
        bn_read_be(buf, &pub.y);

        for (j = 0; j < 2; j++) {
            base58_encode_address(&pub, tests[i][j].coin,
                    tests[i][j].compressed, str);
            if (strcmp(str, tests[i][j].address) != 0) {
                printf("Base58 test %u.%u FAILED: output '%s',\n"
                       "  expected '%s'.\n", i, j, str, tests[i][j].address);
                abort();
            }
        }
    }

    puts("Base58 test PASSED.\n");
}

static void test_parser(void)
{
    static const struct {
        const char *text;
        uint8_t coin;
        bool compressed;
        uint8_t salt_type;
        uint8_t salt_len;
        uint8_t salt[32];
        const char (*keys)[2][65];
        int nkeys;
    } tests[] = {
        {
            "  # comment\r\n coin bitcoin  # test\ncompressed\r\n# another comment\n"
            "salt1 dead beef\n"
            "sign 2665 9c1cf7321c178c07437150639ff0c5b7679c7ea195253ed9abda2e\r\n"
            "081a37 00000000 \n"
            " 00000000000000000000000000000000000000000000000000000000",
            BITCOIN, true, 1, 4, { 0xde, 0xad, 0xbe, 0xef }, &public_keys[0], 1
        },
        {
            "coin   ltC \r\nsign\n   \n"
            "aeb03abeee0f0f8b4f7a5d65ce31f9570cef9f72c2dd8a19b4085a30ab03\n\n"
            "3d48#comment\n"
            "00000000000000000000000000000000000000000000000000000000000000\n"
            "01\n"
            "salt1 01234567 89abcdef aabbccdd 11223344 5432fecb 24681357 a1b2c3d4"
            " ff99ee88#max salt\n"
            " uncompressed # really?\n"
            "sign 96e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a0160\n"
            "54201c025e2a5d96b1629b95194e806c63eb96facaedc733b1a4b70ab3b33e3a\n",
            LITECOIN, false, 1, 32, {
                0x01, 0x23, 0x45, 0x67,  0x89, 0xab, 0xcd, 0xef,
                0xaa, 0xbb, 0xcc, 0xdd,  0x11, 0x22, 0x33, 0x44,
                0x54, 0x32, 0xfe, 0xcb,  0x24, 0x68, 0x13, 0x57,
                0xa1, 0xb2, 0xc3, 0xd4,  0xff, 0x99, 0xee, 0x88,
            }, &public_keys[3], 2
        },
        {   "illegal command",      0, true, 0, 0, {}, 0, -2  },
        {   "coin unknown",         0, true, 0, 0, {}, 0, -2  },
        {   "coin\nuncompressed\n", 0, true, 0, 0, {}, 0, -2  },
        {   "coin btc coin ltc\n",  0, true, 0, 0, {}, 0, -2  },
        {   "coin 96e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a0160",
                                    0, true, 0, 0, {}, 0, -2  },
        {   "sign hmm",             0, true, 0, 0, {}, 0, -2  },
        {   "sign 01",              0, true, 0, 0, {}, 0, -2  },
        {   "salt1 abc",            0, true, 1, 1, {}, 0, -2  },
    };

    unsigned i;

    for (i = 0; i != sizeof tests / sizeof tests[0]; i++) {
        struct Raw_public_key pub[5];
        const int last_pub = sizeof pub / sizeof pub[0] - 1;
        int j, nkeys;
        int npasses = tests[i].nkeys < 0 ? 1 : tests[i].nkeys + 2;

        for (j = 0; j != npasses; j++) {
            memset(pub, 0xB5, sizeof pub);

            settings_txt = tests[i].text;
            settings_bytes_left = strlen(settings_txt);
            nkeys = parse_settings(pub, j);

            if (nkeys != tests[i].nkeys
             || settings.coin != tests[i].coin
             || settings.compressed != tests[i].compressed
             || settings.salt_type != tests[i].salt_type
             || settings.salt_len != tests[i].salt_len) {
                printf("Parser test %u FAILED: nkeys %d, coin %#x, compressed %d.\n",
                        i, nkeys, settings.coin, settings.compressed);
                abort();
            }

            if (nkeys < 0)
                continue;       // correct error detection

            if (memcmp(settings.salt, tests[i].salt, settings.salt_len) != 0) {
                printf("Parser test %u FAILED: wrong salt.\n", i);
                abort();
            }

            if (nkeys > j)
                nkeys = j;
            int k;
            for (k = 0; k != nkeys; k++) {
                struct Raw_public_key expected;
                unhexlify(tests[i].keys[k][0], expected.x);
                unhexlify(tests[i].keys[k][1], expected.y);

                if (memcmp(&pub[k], &expected, sizeof expected) != 0) {
                    printf("Parser test %u FAILED: wrong key %d.\n", i, k);
                    abort();
                }
            }
            do {
                if (memcmp(&pub[k], &pub[last_pub], sizeof pub[0]) != 0) {
                    printf("Parser test %u FAILED: key %d overwritten.\n", i, k);
                    abort();
                }
            } while (++k < last_pub);
        }
    }

    puts("Parser test PASSED.\n");
}

int main()
{
    test_base58();
    test_parser();
    gen_hash(160);
    gen_hash(256);
    gen_hash(512);
    gen_hmac512();
    return 0;
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
FRESULT f_read (FIL *f, void *buf, UINT len, UINT *actual)
{
    if (len > settings_bytes_left)
        len = settings_bytes_left;
    memcpy(buf, settings_txt, len);
    settings_bytes_left -= len;
    settings_txt += len;
    *actual = len;
    return FR_OK;
}
