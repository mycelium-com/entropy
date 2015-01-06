/*
 * Signing tool.
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
#include <stdbool.h>
#include <string.h>
#include <board.h>
#include <sysclk.h>
#include <delay.h>
#include <flashcalw.h>
#include <usart.h>

#include "sys/stdio-uart.h"
#include "sys/now.h"
#include "lib/fwsign.h"
#include "lib/hex.h"
#include "lib/ecdsa.h"
#include "me/rng.h"
#include "conf_uart_serial.h"
#include "oled.h"
#include "devctrl.h"


static bool readline(char *buf, int size);
static void print_public_key(const curve_point *public);
static void print_signature(const uint8_t signature[64]);

int main(void)
{
    uint32_t * private_key = (uint32_t *) (FLASH_USER_PAGE_ADDR + 0x20);
    bignum256 private;      // private key in bignum format
    curve_point public;     // public key
    bool using_production_key = false;
    uint32_t c;             // character from UART

    sysclk_init();
    board_init();
    stdio_uart_init();
    devctrl_init();
    oled_init();
    ast_init();

    puts("\n-- Signing tool --");
    oled_write(0, 0, "\tSIGNING TOOL");

    if (!rng_init()) {
        puts("Error: no entropy available.");
        oled_write(1, 1, "\tNO ENTROPY");
        return 1;
    }

    printf("Sign with development (d) or production (p) key? ");
    oled_write(1, 0, "Dev. key - 1");
    oled_write(1, 2, "Main key - 3");
    for (;;) {
        if (usart_read(CONF_UART, &c) != 0) {
            c = oled_check_buttons();
            c = c == 1 ? 'D' : c == 3 ? 'P' : 0;
        }
        c |= 040;
        if (c == 'd') {
            puts("development");
            oled_write(1, 2, "\tdevelopment key");
            break;
        }
        if (c == 'p') {
            puts("production");
            oled_write(1, 2, "\tproduction key");
            private_key += 8;
            using_production_key = true;
            break;
        }
    }
    oled_write(1, 0, "\tLoading");

    if (private_key[0] != 0xffffffff ||
            memcmp(private_key, private_key + 1, 28) != 0) {
        puts("Private key found.");
        bn_read_be((uint8_t *) private_key, &private);
    } else {
        // no private key in user page, generate a new one
        uint32_t buf[8];
        puts("Generating new private key.");
        do {
            rng_next(buf);
            bn_read_be((uint8_t *) buf, &private);
        } while (!bn_is_less(&private, &prime256k1));
        flashcalw_memcpy(private_key, buf, 32, false);
    }

    // compute public key
    uint32_t t = now();
    scalar_multiply(&private, &public);
    print_time_interval("Mutliplication time", now() - t);

    print_public_key(&public);

    for (;;) {
        char hash_hex[128];
        uint8_t hash_bin[32];
        uint8_t sig[64];
        int res;

        oled_clear();
        sprintf(hash_hex, "\tSIGN WITH %s KEY", 
                using_production_key ? "MAIN" : "DEV.");
        oled_write(0, 0, hash_hex);
        oled_write(1, 2, "\nEnter hash");

        puts("\nPlease enter hash.");
        if (!readline(hash_hex, sizeof hash_hex)) {
            puts("Error: hash string is too long.");
            continue;
        }
        if (!hash_hex[0])
            continue;

        if (unhexlify(hash_hex, hash_bin, sizeof hash_bin) != sizeof hash_bin) {
            puts("Invalid hex string.");
            continue;
        }

        strcpy(hash_hex, "Hash:");
        hexlify(hash_hex + 5, hash_bin, 8);
        oled_write(0, 0, hash_hex);
        strcpy(hash_hex, "       ");
        hexlify(hash_hex + 7, hash_bin + 8, 8);
        oled_write(0, 1, hash_hex);
        oled_write(0, 2, " ");
        sprintf(hash_hex, "%s key -- confirm with 2",
                using_production_key ? "MAIN" : "DEV.");
        oled_write(0, 3, hash_hex);

        do {
            res = oled_check_buttons();
        } while (!res);
        if (res != 2) {
            oled_clear();
            oled_write(1, 1, "\tCancelled.");
            delay_ms(3000);
            continue;
        }

        puts("Signing in progress...");
        oled_write(0, 1, " ");
        oled_write(1, 2, "\tSigning...");

        do {
            t = now();
            res = ecdsa_sign_digest((uint8_t *) private_key, hash_bin, sig);
            print_time_interval("Signing time", now() - t);
            if (res)
                printf("Error %d, retrying.\n", res);
        } while (res);

        hash_hex[0] = 'R';
        hash_hex[1] = ':';
        hexlify(hash_hex + 2, sig, 8);
        oled_write(0, 0, hash_hex);
        hash_hex[0] = 'S';
        hexlify(hash_hex + 2, sig + 32, 8);
        oled_write(0, 1, hash_hex);
        oled_write(0, 2, " ");
        oled_write(0, 3, "\tDONE");

        hexlify(hash_hex, hash_bin, 32);
        printf("Hash:%s\n", hash_hex);
        print_public_key(&public);
        print_signature(sig);

        do ; while (oled_check_buttons() == 0 && usart_read(CONF_UART, &c) != 0);
    }
}


static bool readline(char *buf, int size)
{
    uint32_t c;

    do {
        usart_getchar(CONF_UART, &c);
        if (size > 0)
            *buf++ = c;
        --size;
    } while (c != '\r' && c != '\n');

    *--buf = 0;
    return size >= 0;
}

static void print_public_key(const curve_point *public)
{
    char hex[97];
    uint8_t buf[32];

    bn_write_be(&public->x, buf);
    hexlify(hex, buf, 32);
    printf("-- Public key --\nx:%s\n", hex);
    bn_write_be(&public->y, buf);
    hexlify(hex, buf, 32);
    printf("y:%s\n", hex);
}

static void print_signature(const uint8_t signature[64])
{
    char hex[97];

    hexlify(hex, signature, 32);
    printf("-- Signature --\nr:%s\n", hex);
    hexlify(hex, signature + 32, 32);
    printf("s:%s\n", hex);
}
