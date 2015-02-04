/*
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

#include <board.h>
#include <sleepmgr.h>
#include <usart.h>
#include <led.h>
#include <udc.h>
#include <ff.h>
#include <ctrl_access.h>

#include "conf_usb.h"
#include "conf_access.h"
#include "conf_uart_serial.h"
#include "sys/conf_system.h"
#include "sys/stdio-uart.h"
#include "sys/now.h"
#include "sys/sync.h"
#include "lib/rs.h"
#include "lib/fwsign.h"
#include "ui.h"
#include "sss.h"
#include "jpeg.h"
#include "jpeg-data.h"
#include "xflash.h"
#include "fs.h"
#include "me-access.h"
#include "keygen.h"
#include "hd.h"
#include "data.h"
#include "rng.h"
#include "update.h"
#include "settings.h"
#include "readme.h"

// Symbols defined by the linker.
// Uninitialised memory is between them.
extern struct {
    uint8_t sector_buf[512];
    uint8_t stream_buf[];
} _estack;
extern uint32_t __ram_end__;

// Composite Block Device parameters.  They are used by me-access.c.
unsigned cbd_num_sectors;   // total number of sectors on the device
enum {      // indices of the CBD map entries for different content
    CBD_ENTRY_FS,           // filesystem metadata and readme.txt
    CBD_ENTRY_JPEG,         // JPEG file with keys and addresses
    CBD_ENTRY_ESRC,         // raw entropy sources
};
static void make_fs(void);
static uint8_t * fs_get_block(unsigned blk);
static uint8_t * esrc_get_block(unsigned blk);
struct CBD_map_entry cbd_map[] = {
    [CBD_ENTRY_FS]      = { .get_block = fs_get_block },
    [CBD_ENTRY_JPEG]    = { .get_block = jpeg_get_block, .prefetchable = true },
    [CBD_ENTRY_ESRC]    = { .get_block = esrc_get_block },
};
// File name prefix.
static const char *prefix;

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
        appname("Mycelium Entropy Firmware Update");
        __ram_end__ = 0;        // indicate that SRAM is not random
        update_run();
        ui_off();
        ui_btn_count = 0;
        sysclk_set_source(SYSCLK_SRC_RCSYS);
        do
            sleepmgr_enter_sleep();
        while (ui_btn_count == 0);
        NVIC_SystemReset();
        return 0;
    }

    set_active_lun(LUN_ID_ME_ACCESS);
    ui_keygen();
    appname("Mycelium Entropy");

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
    if (settings.hd) {
        uint64_t seed[8];
        hd_gen_seed_with_mnemonic(16, seed, texts[IDX_PRIVKEY]);
        hd_make_xpub((const uint8_t *) seed, sizeof seed);
        len = 0;    // suppress uninitialised variable warning
    } else {
        len = keygen(key);
    }

#if AT25DFX_MEM
    do ; while (!xflash_ready);
#endif

    // set up coin condition for the layout;
    // treat BITCOIN_TESTNET same as BITCOIN
    if (settings.coin.bip44 == COIN_BIP44(BITCOIN_TESTNET))
        layout_conditions[COND_COIN] = COIN_BIP44(BITCOIN);
    else
        layout_conditions[COND_COIN] = settings.coin.bip44;

    int mode = ui_btn_count;
    if (settings.hd) {
        // HD mode does not support Shamir's secret sharing or salt yet
        cbd_num_sectors = 432;
        jpeg_init(_estack.stream_buf, (uint8_t *) &__ram_end__,
                hd_layout);
        prefix = "HD ";
    } else if (mode) {
        // generate 2-of-3 Shamir's shares
        cbd_num_sectors = 928 + settings.salt_type * 80;
        rs_init(0x11d, 1);  // initialise GF(2^8) for Shamir
        sss_encode(2, 3, SSS_BASE58, key, len);
        jpeg_init(_estack.stream_buf, (uint8_t *) &__ram_end__,
                settings.salt_type == 0 ? shamir_layout : shamir_salt1_layout);
        prefix = "2-of-3 ";
    } else {
        // generate regular private key in Wallet Import Format (aka SIPA)
        cbd_num_sectors = 290 + settings.salt_type * 140;
        jpeg_init(_estack.stream_buf, (uint8_t *) &__ram_end__,
                settings.salt_type == 0 ? main_layout : salt1_layout);
        prefix = "";
    }
    cbd_buf_owner = CBD_NONE;
    make_fs();
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
                sleepmgr_enter_sleep();
            }
        } else {
            sleepmgr_enter_sleep();
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

        main_check_suspend_wakeup(false);
    }
}

static volatile enum {
    NONE,
    SUSPEND,
    WAKEUP
} usb_request;

void main_check_suspend_wakeup(bool postpone_suspend)
{
	irqflags_t flags;
    static uint32_t pba_mask;

    switch (usb_request) {
    case SUSPEND:
        if (postpone_suspend)
            break;
        do ; while (!usart_is_tx_empty(CONF_UART));
        flags = cpu_irq_save();
        pba_mask = PM->PM_PBAMASK;
        PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAu) |
            PM_UNLOCK_ADDR((uintptr_t) &PM->PM_PBAMASK - (uintptr_t) PM);
        PM->PM_PBAMASK = 0;
        usb_request &= ~SUSPEND;
        cpu_irq_restore(flags);
        ui_suspend();
        sysclk_set_prescalers(3, 3, 3, 3, 3);
        break;

    case WAKEUP:
        sysclk_set_prescalers(0, 0, 0, 0, 0);
        ui_wakeup();
        flags = cpu_irq_save();
        if (pba_mask) {
            PM->PM_UNLOCK = PM_UNLOCK_KEY(0xAAu) |
                PM_UNLOCK_ADDR((uintptr_t) &PM->PM_PBAMASK - (uintptr_t) PM);
            PM->PM_PBAMASK = pba_mask;
        }
        usb_request &= ~WAKEUP;
        cpu_irq_restore(flags);
        break;

    default:
        break;
    }
}

void main_suspend_action(void)
{
    sync_suspend();
    usb_request = SUSPEND;
}

void main_resume_action(void)
{
    usb_request = WAKEUP;
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

// Composite block device filesystem.

static const uint8_t * read_ptr(unsigned secno)
{
    return _estack.stream_buf + secno * 512;
}

static uint8_t * write_ptr(unsigned secno)
{
    return _estack.stream_buf + secno * 512;
}

static void make_fs(void)
{
    enum {
        CLU_SECT    = 2,                // cluster size in sectors
    };
    static const struct FS_init_param fs_param = {
        .lun        = FS_LUN_STREAM,    // TODO: unused by fs_init()
        .fdisk      = false,            // make a non-partitioned volume
        .clu_sect   = CLU_SECT,         // cluster size in sectors
        .align      = 1,                // no alignment
        .label      = "MYCELIUM   ",    // volume label
        .read_ptr   = read_ptr,         // device read function
        .write_ptr  = write_ptr,        // device write function
    };
    FATFS fs;
    FIL file;
    UINT bytes_written;

    // Initialise and mount filesystem.
    fs_stream_buf = _estack.stream_buf;
    unsigned nblk = fs_init(&fs_param, cbd_num_sectors, true); // TODO: FS size
    f_mount(1, &fs);

    // Add readme.txt.
    f_open(&file, "1:readme.txt", FA_WRITE | FA_CREATE_ALWAYS);
    f_write(&file, readme, sizeof readme - 1, &bytes_written);
    f_close(&file);
    nblk += (sizeof readme - 2 + CLU_SECT * 512) / 512 & -CLU_SECT;

    // Set sizes in the USB device block map.
    cbd_map[CBD_ENTRY_FS].size = nblk;
    cbd_map[CBD_ENTRY_JPEG].size = cbd_num_sectors - nblk;

    // Add JPEG file.
    char *buf = (char *) _estack.stream_buf + nblk * 512;
    sprintf((char *)buf, "1:%s%.35s.jpg", prefix, texts[IDX_ADDRESS]);
    f_open(&file, (char *)buf, FA_WRITE | FA_CREATE_ALWAYS);
    FRESULT res = f_lseek(&file, (cbd_num_sectors - nblk) * 512);
    if (res != FR_OK)
        printf("f_lseek: %d\n", res);
    res = f_close(&file);
    if (res != FR_OK)
        printf("f_close: %d\n", res);

    // Unmount filesystem and register ownership.
    f_mount(1, 0);
    cbd_buf_owner = CBD_FS;
}

static uint8_t * fs_get_block(unsigned blk)
{
    if (cbd_buf_owner != CBD_FS)
        make_fs();
    return write_ptr(blk);
}

static uint8_t * esrc_get_block(unsigned blk)
{
    // TODO: decrypt raw entropy from serial flash
    return _estack.sector_buf;
}
