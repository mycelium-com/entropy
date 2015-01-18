/*
 * Tests.
 * Usage:  ./test | ./test.py
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <ff.h>
#include "lib/rs.h"
#include "lib/sha256.h"
#include "lib/ripemd.h"
#include "lib/sha512.h"
#include "lib/pbkdf2.h"
#include "lib/base58.h"
#include "lib/xxtea.h"
#include "layout.h"
#include "settings.h"
#include "data.h"
#include "keygen.h"
#include "sss.h"
#include "hd.h"

extern uint32_t _estack[1024 * 15 / 4];
extern uint32_t __ram_end__;

asm (".section .bss, \"w\"\n.globl __estack, ___ram_end__");
asm ("__estack: .space 1024 * 15\n___ram_end__: .space 4");

// for simulated fatfs
static const uint8_t *settings_txt;
static unsigned settings_bytes_left;

static int unhexlify(const char *hex, uint8_t *bin)
{
    unsigned byte = 1;
    unsigned char c;
    uint8_t *binp = bin;

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
            *binp++ = byte;
            byte = 1;
        }
    }

    assert(byte == 1);

    return binp - bin;
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

static void test_pbkdf2(void)
{
    // Test vectors from trezor-crypto, originally from
    // http://stackoverflow.com/questions/15593184/pbkdf2-hmac-sha-512-test-vectors

    static const struct {
        const char *password;
        const char *salt;
        int iterations;
        char key[129];
    } tests[] = {
        { "password", "salt", 1,
          "867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252"
          "c02d470a285a0501bad999bfe943c08f050235d7d68b1da55e63f73b60a57fce" },
        { "password", "salt", 2,
          "e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f0040713f18aefdb866d53c"
          "f76cab2868a39b9f7840edce4fef5a82be67335c77a6068e04112754f27ccf4e" },
        { "password", "salt", 4096,
          "d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f87f6902e072f457b5"
          "143f30602641b3d55cd335988cb36b84376060ecd532e039b742a239434af2d5" },
        { "passwordPASSWORDpassword", "saltSALTsaltSALTsaltSALTsaltSALTsalt", 4096,
          "8c0511f4c6e597c6ac6315d8f0362e225f3c501495ba23b868c005174dc4ee71"
          "115b59f9e60cd9532fa33e0f75aefe30225c583a186cd82bd4daea9724a3d3b8" },
    };

    uint64_t key[8];
    uint8_t  key_vec[64];
    unsigned i;

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        const char *password = tests[i].password;
        const char *salt = tests[i].salt;

        unhexlify(tests[i].key, key_vec);
        pbkdf2_512(key, password, salt, tests[i].iterations);
        if (memcmp(key, key_vec, sizeof key) != 0) {
            printf("PBKDF2 test %u FAILED.\n", i);
            abort();
        }
    }

    puts("PBKDF2 test PASSED.\n");
}

static void test_xxtea(void)
{
    static const uint32_t key[4] = {
        0x12345678, 0xabcdef99, 0x00112233, 0xffffffff
    };
    static const uint32_t msg[128] = {
        0x75712041, 0x206b6369, 0x776f7262, 0x7866206e, 0xffffffff, 0x89abcdef
    };
    static const uint32_t enc[128] = {
        0x5550980b, 0xa9e36ee9, 0xc129ef58, 0x09bc30c8, 0x5a2b7f71, 0xbd23b5fd,
        0x49fa0a5e, 0x29e0a88c, 0xabeb982d, 0x7a9df16c, 0xf6b08876, 0xd5aff559,
        0xc946f16b, 0xdf502d58, 0x6fe524ef, 0x4c9de4ae, 0x75fa29a5, 0x5058b300,
        0x5eb49410, 0xb7bbfa3f, 0xb9f97576, 0xb910258e, 0xbe6a2db0, 0xce56fcc8,
        0xbb297d7f, 0x1ece9984, 0x3e79b62b, 0xc16ede48, 0x42a1e05d, 0xce59257f,
        0xeaa58fa9, 0x6df3c32f, 0x404000a2, 0x8f9a12f6, 0xa53df492, 0xaf5d6e21,
        0x7acbfb4a, 0x22b86a8c, 0x7c2babe7, 0x6a7a8ec8, 0x505cb2dc, 0x231e6e55,
        0xc62861d0, 0x49b927cc, 0x6d3fca0b, 0x5beb763b, 0x8f9bcdda, 0xd9b9a0ea,
        0x177e42b7, 0xbac89c2b, 0x91e1d80d, 0x18032e5e, 0xb4e26ea9, 0xff637b1d,
        0x3ba58e5b, 0xde7dde8e, 0x2c75b314, 0xacdb82fc, 0x4c9b8dc8, 0x428ba935,
        0x373eab13, 0x04731b69, 0x16a7a92b, 0x5a108121, 0x25b3f698, 0x3978da78,
        0xa74d4845, 0x0fe4a04e, 0x76b5385e, 0x9740f87c, 0xb92ed67b, 0xdceb8181,
        0x16c1e413, 0x56ae342f, 0x43fa919a, 0x618aed8d, 0xcd7ddf7e, 0xcab34a94,
        0x9f7a90b8, 0x608e4603, 0xc4b8cc99, 0x2de0bd39, 0xedd52196, 0x94cf81d5,
        0x2eab4554, 0x40c6c300, 0xcfe739bd, 0x6ba39d22, 0x2f3584b6, 0xd51db720,
        0xb39dab01, 0xd35f5f9f, 0x2ead1b1b, 0x7db98411, 0x35ea7101, 0x10a93104,
        0x22b10a69, 0x0413472d, 0x2cc3259b, 0x2c6d3c31, 0x267b9598, 0xb440a61a,
        0x10d57ca7, 0xef27869e, 0x09ef2bac, 0xe430853b, 0x7ac3fa4a, 0x3507aca7,
        0x7c4bbe88, 0x6a72f9c3, 0x54c93524, 0xec8b20f8, 0x09397368, 0xdc7502f4,
        0x3c0df564, 0x7cfc6bf0, 0x26df3c96, 0x9ec589a4, 0xe9861099, 0xc6b74dfe,
        0xb0c1df91, 0x1dd975c8, 0x08489783, 0xb9bff7f4, 0xc05cd9c0, 0x4117b62e,
        0xa601237a, 0xcc1691da };
    uint32_t buf[128];

    memcpy(buf, msg, sizeof buf);
    xxtea_encrypt_block(buf, key);
    if (memcmp(buf, enc, sizeof buf) != 0) {
        puts("XXTEA encoding test 1 FAILED.");
        abort();
    }
    xxtea_decrypt_block(buf, key);
    if (memcmp(buf, msg, sizeof buf) != 0) {
        puts("XXTEA decoding test 1 FAILED.");
        abort();
    }

    int i, j;
    uint32_t rkey[4];
    uint32_t rmsg[128];

    for (i = 0; i < 256; i++) {
        for (j = 0; j < 4; j++)
            rkey[j] = random();
        for (j = 0; j < 128; j++)
            rmsg[j] = random();

        memcpy(buf, rmsg, sizeof buf);
        xxtea_encrypt_block(buf, key);
        xxtea_decrypt_block(buf, key);
        if (memcmp(buf, rmsg, sizeof buf) != 0) {
            puts("XXTEA test 2 FAILED.");
            abort();
        }
        xxtea_encrypt_block(buf, rkey);
        xxtea_decrypt_block(buf, rkey);
        if (memcmp(buf, rmsg, sizeof buf) != 0) {
            puts("XXTEA test 3 FAILED.");
            abort();
        }
    }

    puts("XXTEA tests PASSED.\n");
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
        uint16_t coin;
        bool compressed;
        uint8_t salt_type;
        uint8_t salt_len;
        uint8_t salt[32];
        bool hd;
        char *hd_path;
        const char (*keys)[2][65];
        int nkeys;
    } tests[] = {
        {
            "  # comment\r\n coin bitcoin  # test\ncompressed\r\n# another comment\n"
            "salt1 dead beef\n"
            "sign 2665 9c1cf7321c178c07437150639ff0c5b7679c7ea195253ed9abda2e\r\n"
            "081a37 00000000 \n"
            " 00000000000000000000000000000000000000000000000000000000",
            BITCOIN, true, 1, 4, { 0xde, 0xad, 0xbe, 0xef }, false, "", &public_keys[0], 1
        },
        {
            "coin   ltC \r\nsign\n   \n"
            "aeb03abeee0f0f8b4f7a5d65ce31f9570cef9f72c2dd8a19b4085a30ab03\n\n"
            "3d48#comment\n"
            "00000000000000000000000000000000000000000000000000000000000000\n"
            "01\n"
            "salt1 01234567 89abcdef aabbccdd 11223344 5432fecb 24681357 a1b2c3d4"
            " ff99ee88#max salt\n"
            "hd  m/11/22'/33\n"
            " uncompressed # really?\n"
            "sign 96e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a0160\n"
            "54201c025e2a5d96b1629b95194e806c63eb96facaedc733b1a4b70ab3b33e3a\n",
            LITECOIN, false, 1, 32, {
                0x01, 0x23, 0x45, 0x67,  0x89, 0xab, 0xcd, 0xef,
                0xaa, 0xbb, 0xcc, 0xdd,  0x11, 0x22, 0x33, 0x44,
                0x54, 0x32, 0xfe, 0xcb,  0x24, 0x68, 0x13, 0x57,
                0xa1, 0xb2, 0xc3, 0xd4,  0xff, 0x99, 0xee, 0x88,
            }, true, "m/11/22'/33", &public_keys[3], 2
        },
        {   "illegal command",      0, true, 0, 0, {}, false, "", 0, -2  },
        {   "coin unknown",         0, true, 0, 0, {}, false, "", 0, -2  },
        {   "coin\nuncompressed\n", 0, true, 0, 0, {}, false, "", 0, -2  },
        {   "coin btc coin ltc\n",  0, true, 0, 0, {}, false, "", 0, -2  },
        {   "coin 96e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a0160",
                                    0, true, 0, 0, {}, false, "", 0, -2  },
        {   "sign hmm",             0, true, 0, 0, {}, false, "", 0, -2  },
        {   "sign 01",              0, true, 0, 0, {}, false, "", 0, -2  },
        {   "salt1 abc",            0, true, 1, 1, {}, false, "", 0, -2  },
        {   "hd",                   0, true, 0, 0, {}, true,  "", 0,  0  },
        {   "hd xyz abc",           0, true, 0, 0, {}, true,  "xyz", 0, -2 },
        {   "hd abcdefghijklmnopqrstuvwxyz12345", 0, true, 0, 0, {}, true,
               "abcdefghijklmnopqrstuvwxyz12345", 0, 0 },
        {   "hd abcdefghijklmnopqrstuvwxyz123456", 0, true, 0, 0, {}, true,
               "", 0, -2 },
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
             || settings.coin.type != tests[i].coin
             || settings.compressed != tests[i].compressed
             || settings.salt_type != tests[i].salt_type
             || settings.salt_len != tests[i].salt_len) {
                printf("Parser test %u FAILED: nkeys %d, coin %#x, compressed %d.\n",
                        i, nkeys, settings.coin.type, settings.compressed);
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

// Buffer for supplying fake RNG from test vectors.
static const uint32_t *fake_rng;

static void test_bip39(void)
{
    // Mnemonics longer than 128 bytes are not currently supported.
    static const struct {
        const char *entropy;
        const char *mnemonic;
        char seed[129];
    } vectors[] = {
        {
            "00000000000000000000000000000000",
            "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
            "c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e5349553"
                "1f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04"
        },
        {
            "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
            "legal winner thank year wave sausage worth useful legal winner thank yellow",
            "2e8905819b8723fe2c1d161860e5ee1830318dbf49a83bd451cfb8440c28bd6f"
                "a457fe1296106559a3c80937a1c1069be3a3a5bd381ee6260e8d9739fce1f607"
        },
        {
            "80808080808080808080808080808080",
            "letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
            "d71de856f81a8acc65e6fc851a38d4d7ec216fd0796d0a6827a3ad6ed5511a30"
                "fa280f12eb2e47ed2ac03b5c462a0358d18d69fe4f985ec81778c1b370b652a8"
        },
        {
            "ffffffffffffffffffffffffffffffff",
            "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
            "ac27495480225222079d7be181583751e86f571027b0497b5b5d11218e0a8a13"
                "332572917f0f8e5a589620c6f15b11c61dee327651a14c34e18231052e48c069"
        },
        /* {
            "000000000000000000000000000000000000000000000000",
            "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent",
            "035895f2f481b1b0f01fcf8c289c794660b289981a78f8106447707fdd9666ca"
                "06da5a9a565181599b79f53b844d8a71dd9f439c52a3d7b3e8a79c906ac845fa"
        }, */
        {
            "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
            "legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will",
            "f2b94508732bcbacbcc020faefecfc89feafa6649a5491b8c952cede496c214a"
                "0c7b3c392d168748f2d4a612bada0753b52a1c7ac53c1e93abd5c6320b9e95dd"
        },
        {
            "808080808080808080808080808080808080808080808080",
            "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always",
            "107d7c02a5aa6f38c58083ff74f04c607c2d2c0ecc55501dadd72d025b751bc2"
                "7fe913ffb796f841c49b1d33b610cf0e91d3aa239027f5e99fe4ce9e5088cd65"
        },
        {
            "ffffffffffffffffffffffffffffffffffffffffffffffff",
            "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when",
            "0cd6e5d827bb62eb8fc1e262254223817fd068a74b5b449cc2f667c3f1f985a7"
                "6379b43348d952e2265b4cd129090758b3e3c2c49103b5051aac2eaeb890a528"
        },
        /* {
            "0000000000000000000000000000000000000000000000000000000000000000",
            "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
            "bda85446c68413707090a52022edd26a1c9462295029f2e60cd7c4f2bbd30971"
                "70af7a4d73245cafa9c3cca8d561a7c3de6f5d4a10be8ed2a5e608d68f92fcc8"
        },
        {
            "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
            "legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title",
            "bc09fca1804f7e69da93c2f2028eb238c227f2e9dda30cd63699232578480a40"
                "21b146ad717fbb7e451ce9eb835f43620bf5c514db0f8add49f5d121449d3e87"
        },
        {
            "8080808080808080808080808080808080808080808080808080808080808080",
            "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless",
            "c0c519bd0e91a2ed54357d9d1ebef6f5af218a153624cf4f2da911a0ed8f7a09"
                "e2ef61af0aca007096df430022f7a2b6fb91661a9589097069720d015e4e982f"
        }, */
        {
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote",
            "dd48c104698c30cfe2b6142103248622fb7bb0ff692eebb00089b32d22484e16"
                "13912f0a5b694407be899ffd31ed3992c456cdf60f5d4564b8ba3f05a69890ad"
        },
        {
            "77c2b00716cec7213839159e404db50d",
            "jelly better achieve collect unaware mountain thought cargo oxygen act hood bridge",
            "b5b6d0127db1a9d2226af0c3346031d77af31e918dba64287a1b44b8ebf63cdd"
                "52676f672a290aae502472cf2d602c051f3e6f18055e84e4c43897fc4e51a6ff"
        },
        {
            "b63a9c59a6e641f288ebc103017f1da9f8290b3da6bdef7b",
            "renew stay biology evidence goat welcome casual join adapt armor shuffle fault little machine walk stumble urge swap",
            "9248d83e06f4cd98debf5b6f010542760df925ce46cf38a1bdb4e4de7d21f5c3"
                "9366941c69e1bdbf2966e0f6e6dbece898a0e2f0a4c2b3e640953dfe8b7bbdc5"
        },
        /* {
            "3e141609b97933b66a060dcddc71fad1d91677db872031e85f4c015c5e7e8982",
            "dignity pass list indicate nasty swamp pool script soccer toe leaf photo multiply desk host tomato cradle drill spread actor shine dismiss champion exotic",
            "ff7f3184df8696d8bef94b6c03114dbee0ef89ff938712301d27ed8336ca89ef"
                "9635da20af07d4175f2bf5f3de130f39c9d9e8dd0472489c19b1a020a940da67"
        }, */
        {
            "0460ef47585604c5660618db2e6a7e7f",
            "afford alter spike radar gate glance object seek swamp infant panel yellow",
            "65f93a9f36b6c85cbe634ffc1f99f2b82cbb10b31edc7f087b4f6cb9e976e9fa"
                "f76ff41f8f27c99afdf38f7a303ba1136ee48a4c1e7fcd3dba7aa876113a36e4"
        },
        {
            "72f60ebac5dd8add8d2a25a797102c3ce21bc029c200076f",
            "indicate race push merry suffer human cruise dwarf pole review arch keep canvas theme poem divorce alter left",
            "3bbf9daa0dfad8229786ace5ddb4e00fa98a044ae4c4975ffd5e094dba9e0bb2"
                "89349dbe2091761f30f382d4e35c4a670ee8ab50758d2c55881be69e327117ba"
        },
        /* {
            "2c85efc7f24ee4573d2b81a6ec66cee209b2dcbd09d8eddc51e0215b0b68e416",
            "clutch control vehicle tonight unusual clog visa ice plunge glimpse recipe series open hour vintage deposit universe tip job dress radar refuse motion taste",
            "fe908f96f46668b2d5b37d82f558c77ed0d69dd0e7e043a5b0511c48c2f10646"
                "94a956f86360c93dd04052a8899497ce9e985ebe0c8c52b955e6ae86d4ff4449"
        }, */
        {
            "eaebabb2383351fd31d703840b32e9e2",
            "turtle front uncle idea crush write shrug there lottery flower risk shell",
            "bdfb76a0759f301b0b899a1e3985227e53b3f51e67e3f2a65363caedf3e32fde"
                "42a66c404f18d7b05818c95ef3ca1e5146646856c461c073169467511680876c"
        },
        {
            "7ac45cfe7722ee6c7ba84fbc2d5bd61b45cb2fe5eb65aa78",
            "kiss carry display unusual confirm curtain upgrade antique rotate hello void custom frequent obey nut hole price segment",
            "ed56ff6c833c07982eb7119a8f48fd363c4a9b1601cd2de736b01045c5eb8ab4"
                "f57b079403485d1c4924f0790dc10a971763337cb9f9c62226f64fff26397c79"
        },
        /* {
            "4fa1a8bc3e6d80ee1316050e862c1812031493212b7ec3f3bb1b08f168cabeef",
            "exile ask congress lamp submit jacket era scheme attend cousin alcohol catch course end lucky hurt sentence oven short ball bird grab wing top",
            "095ee6f817b4c2cb30a5a797360a81a40ab0f9a4e25ecd672a3f58a0b5ba0687"
                "c096a6b14d2c0deb3bdefce4f61d01ae07417d502429352e27695163f7447a8c"
        }, */
        {
            "18ab19a9f54a9274f03e5209a2ac8a91",
            "board flee heavy tunnel powder denial science ski answer betray cargo cat",
            "6eff1bb21562918509c73cb990260db07c0ce34ff0e3cc4a8cb3276129fbcb30"
                "0bddfe005831350efd633909f476c45c88253276d9fd0df6ef48609e8bb7dca8"
        },
        {
            "18a2e1d81b8ecfb2a333adcb0c17a5b9eb76cc5d05db91a4",
            "board blade invite damage undo sun mimic interest slam gaze truly inherit resist great inject rocket museum chief",
            "f84521c777a13b61564234bf8f8b62b3afce27fc4062b51bb5e62bdfecb23864"
                "ee6ecf07c1d5a97c0834307c5c852d8ceb88e7c97923c0a3b496bedd4e5f88a9"
        },
        /* {
            "15da872c95a13dd738fbf50e427583ad61f18fd99f628c417a61cf8343c90419",
            "beyond stage sleep clip because twist token leaf atom beauty genius food business side grid unable middle armed observe pair crouch tonight away coconut",
            "b15509eaa2d09d3efd3e006ef42151b30367dc6e3aa5e44caba3fe4d3e352e65"
                "101fbdb86a96776b91946ff06f8eac594dc6ee1d3e82a42dfe1b40fef6bcc3fd"
        } */
    };

    uint32_t entropy[8];
    uint64_t seed[8];
    uint8_t  seed_vec[64];
    char mnemonic[24 * 9];
    int i;

    fake_rng = entropy;

    for (i = 0; i != sizeof vectors / sizeof vectors[0]; i++) {
        int len = unhexlify(vectors[i].entropy, (uint8_t *) entropy);
        unhexlify(vectors[i].seed, seed_vec);
        if (!hd_gen_seed_with_mnemonic(len, seed, mnemonic)) {
            printf("BIP-39 test %d returned false: FAILED.\n", i);
            abort();
        }
        if (strcmp(mnemonic, vectors[i].mnemonic) != 0
                || memcmp(seed, seed_vec, sizeof seed) != 0) {
            printf("BIP-39 test %d FAILED.\n", i);
            abort();
        }
    }

    fake_rng = 0;
    puts("BIP-39 test PASSED.\n");
}

static void test_bip32(void)
{
    static const char seed_1[] = "000102030405060708090a0b0c0d0e0f";
    static const char seed_2[] =
        "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a2"
        "9f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542";
    static const struct {
        const char *seed;
        const char *settings;
        char xpub[112];
    } vectors[] = {
        {
            seed_1, "hd m",
            "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29E"
                "SFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8"
        },
        {
            seed_1, "hd m/0'",
            "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhw"
                "BZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw"
        },
        {
            seed_1, "hd m/0'/1",
            "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkNAW"
                "bWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ"
        },
        {
            seed_1, "hd m/0'/1/2'",
            "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJPMM"
                "3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5"
        },
        {
            seed_1, "hd m/0'/1/2'/2",
            "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLj"
                "TAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV"
        },
        {
            seed_1, "hd m/0'/1/2'/2/1000000000",
            "xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcYFg"
                "JS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy"
        },
        {
            seed_2, "hd m",
            "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED"
                "9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB"
        },
        {
            seed_2, "hd m/0",
            "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6"
                "wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH"
        },
        {
            seed_2, "hd m/0/2147483647'",
            "xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySD"
                "hKiLDBrQSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a"
        },
        {
            seed_2, "hd m/0/2147483647'/1",
            "xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdM"
                "VCQcoNJxGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon"
        },
        {
            seed_2, "hd m/0/2147483647'/1/2147483646'",
            "xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkr"
                "gZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL"
        },
        {
            seed_2, "hd m/0/2147483647'/1/2147483646'/2",
            "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8"
                "p9HmGsApME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt"
        },
    };

    uint8_t seed[64];
    char settings_buf[64];
    char xpub[112];
    int i;

    for (i = 0; i != sizeof vectors / sizeof vectors[0]; i++) {
        int len = unhexlify(vectors[i].seed, seed);

        settings_txt = vectors[i].settings;
        int settings_len = strlen(settings_txt);
        settings_bytes_left = settings_len;
        if (parse_settings(0, 0) < 0)
            abort();
        texts[IDX_XPUB][0] = 0;
        if (!hd_make_xpub(seed, len)) {
            printf("BIP-39 test %d at %s FAILED: returned false.\n", i,
                    vectors[i].settings);
            abort();
        }
        if (strcmp(texts[IDX_XPUB], vectors[i].xpub) != 0) {
            printf("BIP-32 test %d FAILED at %s.\n", i, vectors[i].settings);
            printf("vector %s\ngen    %s\n", vectors[i].xpub, texts[IDX_XPUB]);
            abort();
        }

        if (settings_len >= sizeof settings.hd_path + 2)
            continue;

        strcpy(settings_buf, vectors[i].settings);
        settings_buf[settings_len++] = '/';
        settings_buf[settings_len] = 0;
        settings_bytes_left = settings_len;
        settings_txt = settings_buf;
        printf("%d %s\n", i, settings_txt);
        if (parse_settings(0, 0) < 0)
            abort();
        texts[IDX_XPUB][0] = 0;
        if (!hd_make_xpub(seed, len)) {
            printf("BIP-39 test %d at %s FAILED: returned false.\n", i,
                    vectors[i].settings);
            abort();
        }
        if (strcmp(texts[IDX_XPUB], vectors[i].xpub) != 0) {
            printf("BIP-32 test %d FAILED at %s.\n", i, settings_buf);
            abort();
        }
    }

    puts("BIP-32 test PASSED.\n");
}

int main()
{
    test_xxtea();
    test_base58();
    test_parser();
    gen_hash(160);
    gen_hash(256);
    gen_hash(512);
    gen_hmac512();
    test_pbkdf2();
    test_bip39();
    test_bip32();
    return 0;
}

// Simulate RNG.
void rng_next(uint32_t random_number[8])
{
    int i;

    for (i = 0; i < 8; i++)
        random_number[i] = fake_rng ? fake_rng[i] : random();
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
