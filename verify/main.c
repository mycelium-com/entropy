/*
 * Firmware Verification Tool.
 *
 * Copyright 2015 Mycelium SA, Luxembourg.
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
#include <string.h>
#include <board.h>
#include <sysclk.h>
#include <dfll.h>
#include <udc.h>

#include "lib/fwsign.h"
#include "lib/ecdsa.h"
#include "lib/sha256.h"
#include "sys/stdio-uart.h"
#include "sys/now.h"
#include "sys/sync.h"
#include "conf_access.h"
#include "xflash.h"
#include "ui.h"

static bool msc_enabled = false;

static bool check_image(uint32_t addr, uint32_t magic);
static bool check_bip39(void);

int main(void)
{
    board_init();
    sysclk_set_source(SYSCLK_SRC_RCSYS);
    dfll_disable_closed_loop(0);
    sysclk_init();
    stdio_uart_init();
    ui_init();
    sync_init();

    if (!xflash_init()) {
        ui_error(UI_HARDWARE_FAULT);
        for (;;);
    }

    ui_busy();

    puts("\nVerification started.");

    if (!check_image(0, FW_MAGIC_BOOT))
        ui_error(UI_BOOT);
    else if (!check_image(0x4000, FW_MAGIC_ENTROPY))
        ui_error(UI_MAIN);
    else if (!check_bip39())
        ui_error(UI_BIP39);
    else
        ui_ok();

    udc_start();
    while (!ui_button_pressed()) {
        if (msc_enabled)
            udi_msc_process_trans();
        sync_diag_print();
    }

    udc_stop();
    NVIC_SystemReset();

    return 0;
}

// Mycelium signature key.
const struct Raw_public_key mycelium_public_key = {
    { 0xad, 0x78, 0x7b, 0x1f,  0x17, 0x07, 0x7f, 0x23,  0x2a, 0xf7, 0xb3, 0xb7,
      0xc0, 0xa1, 0xe5, 0xe0,  0x7c, 0x21, 0xa2, 0xa4,  0xa7, 0x6d, 0x40, 0x19,
      0x31, 0x13, 0x34, 0x61,  0xbf, 0x26, 0xb9, 0x17 },
    { 0xc3, 0x07, 0x27, 0x23,  0xbd, 0x54, 0xa2, 0x8a,  0x65, 0x1c, 0x7d, 0x0e,
      0x04, 0xee, 0x34, 0x2b,  0x2f, 0xa5, 0x32, 0x95,  0xc2, 0x39, 0xef, 0x18,
      0x8a, 0x39, 0xed, 0x40,  0x7a, 0x84, 0xfe, 0x1b }
};

static bool check_image(uint32_t addr, uint32_t magic)
{
    uint32_t t_start, t_hash, t_done;
    uint32_t hash[8];
    uint32_t offset = ((const uint32_t *) addr)[FW_SIGN_VECTOR];
    const struct Firmware_signature *signature =
        (const struct Firmware_signature *) (addr + offset);

    if ((offset & 3) != 0 || offset < 32
            || addr + offset + sizeof *signature > 256 * 1024)
        return false;

    if (signature->magic != magic)
        return false;

    t_start = now();
    offset += offsetof(struct Firmware_signature, hash);
    sha256_hash(hash, (const uint8_t *) addr, offset);
    if (memcmp(hash, signature->hash.b, sizeof hash) != 0)
        return false;
    t_hash = now();

    // check signing key
    if (memcmp(&signature->pubkey, &mycelium_public_key,
                sizeof mycelium_public_key) != 0)
        return false;

    // verify signature
    int err = ecdsa_verify_digest(mycelium_public_key.x, signature->signature.r,
                                  (const uint8_t *) hash);
    t_done = now();

    print_time_interval("Hash", t_hash - t_start);
    print_time_interval("Signature verification", t_done - t_hash);
    printf("Image check at %#lx: %d.\n", addr, err);
    return err == 0;
}

static bool check_bip39(void)
{
    // Hash of the genuine BIP-39 US English word list.
    static const uint8_t bip39_wordlist_hash[] = {
        0x19, 0xa7, 0x18, 0x88, 0x00, 0x94, 0x11, 0x2c,
        0x8a, 0x3e, 0xd5, 0x1c, 0x5c, 0x8c, 0xba, 0xb8,
        0x42, 0xdc, 0x61, 0xbb, 0x7c, 0x15, 0x25, 0x28,
        0xc6, 0xf7, 0xd7, 0x00, 0x61, 0x98, 0xf9, 0x7c
    };
    uint32_t hash[8];
    uint32_t buf[SHA256_BLOCK_SIZE / 4];
    uint32_t addr = xflash_num_blocks * 512 - XFLASH_WORDLIST_OFFSET;
    uint32_t off;
    uint32_t t_start = now();

    // compute hash of the word list in external flash
    sha256_init(hash);
    for (off = 0; off != 2048 * 5; off += sizeof buf) {
        if (!xflash_read((uint8_t *) buf, sizeof buf, addr + off)) {
            ui_error(UI_HARDWARE_FAULT);
            for (;;);
        }
        sha256_transform(hash, buf);
    }
    sha256_finish(hash, (uint8_t *) buf, 0, off);

    print_time_interval("BIP-39 hash", now() - t_start);
    printf("BIP-39 hash done.\n");
    // check if the word list is genuine
    return memcmp(hash, bip39_wordlist_hash, sizeof bip39_wordlist_hash) == 0;
}

void main_sof_action(void)
{
    sync_frame();
}

bool main_msc_enable(void)
{
    msc_enabled = true;
    return true;
}

void main_msc_disable(void)
{
    msc_enabled = false;
}

// say_yes() and say_no() are used in conf_access.h
bool say_yes(void)
{
    return true;
}

bool say_no(void)
{
    return false;
}
