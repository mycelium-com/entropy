/*
 * Parser for settings.txt.
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

#include <string.h>
#include <ff.h>
#include "keygen.h"
#include "settings.h"

struct Settings settings;

// Mycelium signature key.
const struct Raw_public_key mycelium_public_key = {
    { 0xad, 0x78, 0x7b, 0x1f,  0x17, 0x07, 0x7f, 0x23,  0x2a, 0xf7, 0xb3, 0xb7,
      0xc0, 0xa1, 0xe5, 0xe0,  0x7c, 0x21, 0xa2, 0xa4,  0xa7, 0x6d, 0x40, 0x19,
      0x31, 0x13, 0x34, 0x61,  0xbf, 0x26, 0xb9, 0x17 },
    { 0xc3, 0x07, 0x27, 0x23,  0xbd, 0x54, 0xa2, 0x8a,  0x65, 0x1c, 0x7d, 0x0e,
      0x04, 0xee, 0x34, 0x2b,  0x2f, 0xa5, 0x32, 0x95,  0xc2, 0x39, 0xef, 0x18,
      0x8a, 0x39, 0xed, 0x40,  0x7a, 0x84, 0xfe, 0x1b }
};

int parse_settings(struct Raw_public_key keys[], int len)
{
    int key_cnt = 0;
    uint8_t buf[32];
    char token[sizeof settings.hd_path];
    int tlen = 0;           // length of accumulated token, heh heh
    uint8_t *buf_ptr = 0;
    uint8_t  buf_len = 0;
    unsigned hex_buf = 1;
    UINT i = 0;
    UINT num_chars_in_buf;
    FIL file;

    // default configuration
    settings.coin.type = BITCOIN;
    settings.compressed = true;
    settings.salt_type = 0;
    settings.salt_len = 0;
    settings.hd = false;
    settings.hd_path[0] = 0;

    if (f_open(&file, "0:settings.txt", FA_READ) != FR_OK) {
        // probably no such file, revert to default configuration
        if (len > 0)
            memcpy(&keys[0], &mycelium_public_key, sizeof mycelium_public_key);
        return 1;
    }

    struct Token_table {
        const char *token;
        union {
            const struct Token_table *args;
            struct {
                uint16_t code;
                uint16_t value;
            };
        };
    };

    enum {      // codes
        COIN,
        COMPRESS,
        SIGN,
        SALT,
        HD,
    };

    static const struct Token_table coin_args[] = {
        { .token = "bitcoin",   .code = COIN, .value = BITCOIN },
        { .token = "btc",       .code = COIN, .value = BITCOIN },
        { .token = "testnet",   .code = COIN, .value = BITCOIN_TESTNET },
        { .token = "litecoin",  .code = COIN, .value = LITECOIN },
        { .token = "ltc",       .code = COIN, .value = LITECOIN },
        { .token = "peercoin",  .code = COIN, .value = PEERCOIN },
        { .token = "ppcoin",    .code = COIN, .value = PEERCOIN },
        { .token = "ppc",       .code = COIN, .value = PEERCOIN },
        { 0 }
    };
    static const struct Token_table commands[] = {
        { .token = "coin",          .args = coin_args },
        { .token = "compressed",    .code = COMPRESS, .value = true },
        { .token = "uncompressed",  .code = COMPRESS, .value = false },
        { .token = "sign",          .code = SIGN },
        { .token = "salt1",         .code = SALT, .value = 1 },
        { .token = "hd",            .code = HD },
        { 0 }
    };
    const struct Token_table *table = commands;

    // parser state
    bool in_comment = false;
    bool in_hex = false;
    bool in_salt = false;
    int  in_hd = false;

    f_read(&file, buf, sizeof buf, &num_chars_in_buf);

    // Discard BOM if present.  This still works fine even if there are
    // less than three characters in buf.
    if ((buf[0] ^ buf[1]) == 1 && (buf[0] | 1) == 0xFF)
        i += 2;     // UTF-16 BOM
    else if (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
        i += 3;     // UTF-8 BOM

    for (;;) {
        if (num_chars_in_buf != sizeof buf)
            buf[num_chars_in_buf++] = '\n';

        for (; i < num_chars_in_buf; i++) {
            uint8_t c = buf[i];

            if (c == 0)
                continue;   // assuming UTF-16

            if (in_comment && c != '\n')
                continue;

            if (c == '#') {
                in_comment = true;
                continue;
            }
            in_comment = false;

            if (in_salt && c == '\n') {
                in_salt = false;
                in_hex = false;
                settings.salt_len = sizeof settings.salt - buf_len;
                if (hex_buf != 1)
                    return -2;
            }

            if (c <= ' ') {     // isspace
                if (in_hex)
                    continue;

                if (tlen != 0) {
                    // end of token
                    token[tlen] = 0;
                    tlen = 0;
                    if (in_hd) {
                        strcpy(settings.hd_path, token);
                        in_hd = false;
                        goto check_eol;
                    }
                    if (!table)
                        return -2;  // unexpected token
                    while (strcmp(token, table->token) != 0) {
                        table++;
                        if (!table->token)
                            return -2;  // invalid token
                    }

                    // process token
                    if (table == commands)
                        table = table->args;
                    else {
                        switch (table->code) {
                        case COIN:
                            settings.coin.type = table->value;
                            break;
                        case COMPRESS:
                            settings.compressed = table->value;
                            break;
                        case SIGN:
                            buf_ptr = key_cnt < len ? keys[key_cnt].x : 0;
                            key_cnt++;
                            buf_len = 64;
                            in_hex = true;
                            break;
                        case SALT:
                            buf_ptr = settings.salt;
                            buf_len = sizeof settings.salt;
                            settings.salt_type = table->value;
                            in_salt = true;
                            in_hex = true;
                            break;
                        case HD:
                            in_hd = true;
                            settings.hd = true;
                            break;
                        }
                        table = 0;  // no more tokens expected in this line
                    }
                }
check_eol:
                if (c == '\n') {
                    if (table && table != commands)
                        return -2;  // EOL while expecting a token
                    table = commands;
                    in_hd = false;
                }

                continue;
            }

            c |= 040;   // tolower

            if (in_hex) {
                if (c >= '0' && c <= '9')
                    c -= '0';
                else if (c >= 'a' && c <= 'f')
                    c -= 'a' - 10;
                else
                    return -2;  // bad hex
                hex_buf = hex_buf << 4 | c;
                if (hex_buf & 0x100) {
                    if (buf_ptr)
                        *buf_ptr++ = hex_buf;
                    hex_buf = 1;
                    if (--buf_len == 0)
                        in_hex = false;     // end of hex for this key or salt
                }
            } else {
                if (tlen == sizeof token - 1)
                    return -2;  // token is too long
                token[tlen++] = c;
            }
        }

        if (num_chars_in_buf != sizeof buf)
            break;

        if (f_read(&file, buf, sizeof buf, &num_chars_in_buf) != FR_OK)
            return -1;
        i = 0;
    }

    return in_hex ? -2 : key_cnt;
}
