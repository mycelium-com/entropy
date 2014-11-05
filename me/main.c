/*
 * Copyright 2014 Mycelium SA, Luxembourg.
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

#include <board.h>
#include <sleepmgr.h>
#include <led.h>
#include <udc.h>
#include <ff.h>
#include <ctrl_access.h>

#include "conf_usb.h"
#include "conf_access.h"
#include "sys/conf_system.h"
#include "sys/stdio-uart.h"
#include "sys/now.h"
#include "sys/sync.h"
#include "lib/rs.h"
#include "ui.h"
#include "sss.h"
#include "jpeg.h"
#include "jpeg-data.h"
#include "xflash.h"
#include "me-access.h"
#include "keygen.h"
#include "data.h"
#include "rng.h"
#include "update.h"
#include "settings.h"

// Symbols defined by the linker.
// Uninitialised memory is between them.
extern uint8_t  _estack;
extern uint32_t __ram_end__;

unsigned num_sectors;

bool configuration_mode;

volatile bool main_b_msc_enable = false;


int main(void)
{
    uint8_t key[34];
    int len;
    bool usb_started = false;

    // Check the flag passed by the bootloader to determine if we should run in
    // firmware update and configuration mode.
    configuration_mode = (CONFIG_FW_REG & CONFIG_FW_MASK) != 0;

    sleepmgr_init();
    sysclk_init();
    board_init();
    stdio_uart_init();
    ui_init();
    sync_init();
    xflash_init();

    if (configuration_mode) {
        CONFIG_FW_REG_CLR = CONFIG_FW_MASK;     // clear config mode flag
        if (!xflash_4k_erase_capable) {
            puts("\nConfiguration mode not available with this flash chip.");
            ui_error(UI_E_UNSUPPORTED_HARDWARE);
        }
        set_active_lun(LUN_ID_XFLASH_BUF_MEM);
        ui_pulsate();
        puts("\n-- Mycelium Entropy Firmware Update --");
        __ram_end__ = 0;        // indicate that SRAM is not random
        update_run();
        ui_off();
        ui_btn_count = 0;
        do ; while (ui_btn_count == 0);
        NVIC_SystemReset();
        return 0;
    }

    set_active_lun(LUN_ID_ME_ACCESS);
    ui_keygen();
    puts("\n-- Mycelium Entropy --");

    if (!rng_init()) {
        // error, e.g. memory not random
        ui_error(UI_E_NO_ENTROPY);
    }

    // mount external flash in read-only mode for access to graphics resources
    // (layout) and settings.txt
    FATFS fs;
    f_mount(0, &fs);
    // read settings, ignoring the signing keys
    parse_settings(0, 0);

generate_new_key:
#if AT25DFX_MEM
    if (!xflash_erase()) {
        ui_error(UI_E_HARDWARE_FAULT);
    }
#endif

    // Generate key pair.
    len = keygen(key);

#if AT25DFX_MEM
    do ; while (!xflash_ready);
#endif

    bitcoin_address_ref = settings.coin == LITECOIN ? litecoin_address_fragment
        : bitcoin_address_fragment;

    int mode = ui_btn_count;
    if (mode) {
        // generate 2-of-3 Shamir's shares
        num_sectors = 928 + settings.salt_type * 80;
        rs_init(0x11d, 1);  // initialise GF(2^8) for Shamir
        sss_encode(2, 3, SSS_BASE58, key, len);
        jpeg_init(&_estack, (uint8_t *)&__ram_end__, settings.salt_type == 0 ?
                  shamir_layout : shamir_salt1_layout);
    } else {
        // generate regular private key in Wallet Import Format (aka SIPA)
        num_sectors = 290 + settings.salt_type * 140;
        jpeg_init(&_estack, (uint8_t *)&__ram_end__, settings.salt_type == 0 ?
                  main_layout : salt1_layout);
    }
    ui_off();
    LED_On(LED0);

#if AT25DFX_MEM
    xflash_write_jpeg();
#endif

    if (usb_started) {
        // indicate that a new Entropy volume is loaded
        me_unload(false);
    } else {
        // start USB stack
        udc_start();
        usb_started = true;
    }

    ui_btn_count = 0;

    // The main loop manages only the power mode
    // because the USB management is done by interrupt
    while (true) {
        if (main_b_msc_enable) {
            if (!udi_msc_process_trans()) {
                //sleepmgr_enter_sleep();
            }
        } else {
            //sleepmgr_enter_sleep();
        }

        if (global_error_flags) {
            udc_stop();
            ui_error(UI_E_HARDWARE_FAULT);
        }

        sync_diag_print();

        if (ui_btn_count) {
            // unload medium and make another key
            ui_btn_count = 0;
            me_unload(true);
            ui_keygen();
            goto generate_new_key;
        }
    }
}

void main_suspend_action(void)
{
    sync_suspend();
    ui_powerdown();
    //sleepmgr_enter_sleep();
}

void main_resume_action(void)
{
    ui_wakeup();
}

void main_sof_action(void)
{
    sync_frame();
    if (!main_b_msc_enable || configuration_mode)
        return;
    ui_process(udd_get_frame_number());
}

bool main_msc_enable(void)
{
    main_b_msc_enable = true;
    return true;
}

void main_msc_disable(void)
{
    main_b_msc_enable = false;
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
