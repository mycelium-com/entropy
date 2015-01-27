/*
 * Firmware update and configuration.
 *
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

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sleepmgr.h>
#include <udc.h>
#include <ctrl_access.h>
#include <ff.h>
#include <board.h>
#include <led.h>

#include "lib/fwsign.h"
#include "lib/hex.h"
#include "lib/ecdsa.h"
#include "lib/sha256.h"
#include "sys/now.h"
#include "xflash.h"
#include "blkbuf.h"
#include "fs.h"
#include "ui.h"
#include "main.h"
#include "data.h"
#include "settings.h"
#include "applet.h"
#include "update.h"

PARTITION VolToPart[] = {
    { FS_LUN_XFLASH, 1 },   // volume 0: serial flash, partition 1
    { FS_LUN_STREAM, 0 },   // volume 1: JPEG stream, not partitioned
};

static void detect_hardware(void);
static bool make_files(void);
static void process_config(const struct Firmware_signature *signature);
static bool find_image(char fname[13], struct Firmware_signature *signature);
static void hash_image(const char *fname, uint32_t hash[8]);
static void run_applet(const char *fname, uint32_t ftype, FATFS *fs);

static DWORD fattime = (BUILD_YEAR - 1980) << 25
                     | BUILD_MONTH << 21
                     | BUILD_DAY << 16
                     | BUILD_HOUR << 11
                     | BUILD_MIN << 5
                     | BUILD_SEC >> 1;

enum {
    RESERVED_BLOCKS = 128,  // number of blocks to reserve at the end of flash
};

// Check filesystem parameters.
// Our update applet only works with FAT12 with clusters not larger than
// 16 kB.  If the user has re-formatted the drive, FatFs might still
// accept it, but we need to check if it's suitable for the applet.
// This must be called after accessing the filesystem, when *fs is valid;
// f_mount() alone is not enough.
static bool fs_param_ok(const FATFS *fs)
{
    return fs->fs_type == FS_FAT12 && fs->csize <= 32;
}

void update_run(void)
{
    static const struct FS_init_param fs_param = {
        .lun        = FS_LUN_XFLASH,    // TODO: unused by fs_init()
        .fdisk      = true,             // make a partitioned volume
        .clu_sect   = 8,                // cluster is the flash erase unit
        .align      = 8,                // align data to flash erase units
        .label      = "MYCELIUM FW",    // volume label
        .read_ptr   = blkbuf_read_ptr,  // device read function
        .write_ptr  = blkbuf_write_ptr, // device write function
    };
    FATFS fs;

    blkbuf_init();
    fs_init(&fs_param, xflash_num_blocks - RESERVED_BLOCKS, false);
    f_mount(0, &fs);
    detect_hardware();
    if (!make_files() || !fs_param_ok(&fs)) {
        // errors here probably mean the file system is messed up;
        // let's destroy it and make a fresh one
        f_mount(0, 0);
        fs_init(&fs_param, xflash_num_blocks - RESERVED_BLOCKS, true);
        f_mount(0, &fs);
        make_files();
    }
    f_mount(0, 0);
    udc_start();
    blkbuf_flush();
    ui_btn_count = 0;

    bool may_be_dirty = false;
    uint32_t last_usb_trans = 0;

    // Run as a mass storage device until the button is clicked.
    do {
        if (main_b_msc_enable) {
            if (udi_msc_process_trans()) {
                may_be_dirty = true;
                last_usb_trans = now();
            } else {
                sleepmgr_enter_sleep();
            }
        } else {
            sleepmgr_enter_sleep();
        }

        if (may_be_dirty && now() - last_usb_trans > AST_FREQ / 2) {
            blkbuf_flush();
            may_be_dirty = false;
        }

        main_check_suspend_wakeup(may_be_dirty);

        if (global_error_flags) {
            udc_stop();
            ui_error(UI_E_HARDWARE_FAULT);
        }
    } while (ui_btn_count == 0);

    // enter rapid blinking mode as in key generation to indicate we are
    // busy checking stuff
    ui_keygen();
    udc_stop();
    blkbuf_flush();
    f_mount(0, &fs);

    // find image to run or to flash and read its signature block
    char fname[13];
    struct Firmware_signature signature;
    bool image_found = find_image(fname, &signature);

    // process settings.txt and check if the signature is authorised
    process_config(image_found ? &signature : 0);

    if (!image_found)
        return;

    // compare hash
    uint32_t hash[8];
    hash_image(fname, hash);
    if (memcmp(hash, signature.hash.b, sizeof hash) != 0)
        ui_error(UI_E_INVALID_SIGNATURE);

    // verify signature
    int err = ecdsa_verify_digest(signature.pubkey.x, signature.signature.r,
                                  (const uint8_t *) hash);
    if (err)
        ui_error(UI_E_INVALID_SIGNATURE);

    // Run applet that will take care of flashing or running the image.
    ui_off();
    ui_btn_off();
    LED_On(LED0);
    run_applet(fname, signature.magic, &fs);
}

static const char readme_txt[] =
"                          MYCELIUM ENTROPY\r\n"
"               Firmware Update and Configuration Mode\r\n"
"\r\n"
"1. To run an update: copy new firmware file(s) (.bin/.lay/.tsk) here;\r\n"
"   and/or\r\n"
"   to change settings: edit settings.txt with a plain text editor.\r\n"
"2. Instruct your computer to eject, unmount, or to safely remove the\r\n"
"   'MYCELIUM FW' volume.\r\n"
"3. Click the button to commence the update and to check configuration.\r\n"
"4. Rapid flashing of the LED indicates that an operation is in progress.\r\n"
"   Do not disturb the device.\r\n"
"5. When the indicator has turned off completely, unplug the device.\r\n"
"\r\n"
"If you see a repeating blink pattern, it indicates a warning or an error.\r\n"
"Please see the User Manual at http://mycelium.com/entropy for further\r\n"
"information.  It is safe to unplug the device and try again later.\r\n"
"\r\n"
"----\r\n";

static const char * const readme_info[] = {
    "Version:",
    "Hardware:",
    "Version-Code:",
    "Hardware-Code:",
    "Hash:",
    "Signed-By:",
    "No signature block.",
};

static const char settings_txt[] =
"# Mycelium Entropy configuration\r\n"
"#\r\n"
"# If this file is deleted, it will be generated automatically next time\r\n"
"# with default settings.\r\n"
"#\r\n"
"# Please remember to EJECT this volume and CLICK the button on the device\r\n"
"# when you are done editing this file.\r\n"
"#\r\n"
"# Setting names and values are not case sensitive.\r\n"
"# The '#' character starts a comment.\r\n"
"\r\n"
"# Type of cryptocurrency / network / coin:\r\n"
"coin Bitcoin\r\n"
"#coin Litecoin\r\n"
"#coin testnet\r\n"
"\r\n"
"# Type of public key:\r\n"
"compressed\r\n"
"#uncompressed\r\n"
"\r\n"
"# Advanced feature: up to 32 bytes of your own salt in hex, e.g.:\r\n"
"#salt1 dead beef\r\n"
"\r\n"
"# Permitted public keys for signing firmware:\r\n"
"sign";

static struct {
    uint8_t family;
    uint8_t code[3];
} hardware_descr;

static void detect_hardware(void)
{
#if BOARD == USER_BOARD
    hardware_descr.family = BOARD_HW_CODE;
    hardware_descr.code[2] = xflash_id >> 16 & 0xF;
#else
    hardware_descr.family = HW_FAMILY_XPRO;
    hardware_descr.code[2] = 15;
#endif
    hardware_descr.code[0] = (CHIPID->CHIPID_CIDR >> 9 & 7) - 3;
    hardware_descr.code[1] = xflash_4k_erase_capable ? HW_XSFL_SSECT
                                                     : HW_XSFL_LSECT;
}

// Create or update readme.txt and settings.txt files.
// Remove leftovers that might confuse us.
static bool make_files(void)
{
    FRESULT res;
    FIL file;
    DIR dir;
    UINT written;
    unsigned i;
    int n;
    char buf[160];

    // remove tasks to prevent them from being run again by accident
    if (f_opendir(&dir, "0:") != FR_OK)
        return false;

    for (;;) {
        FILINFO fileinfo;
        const char *ext;

        fileinfo.lfname = buf;
        fileinfo.lfsize = sizeof buf;
        if (f_readdir(&dir, &fileinfo) != FR_OK)
            return false;

        if (!*fileinfo.fname)
            break;      // no more entries

        // take the most recent file modification timestamp as a reference
        DWORD datetime = (DWORD) fileinfo.fdate << 16 | fileinfo.ftime;
        if (datetime > fattime)
            fattime = datetime;

        if ((fileinfo.fattrib & (AM_DIR | AM_HID | AM_SYS)) == 0
                && (ext = index(fileinfo.fname, '.')) != 0
                && strcasecmp(ext + 1, "tsk") == 0) {
            if (fileinfo.fattrib & AM_RDO)
                f_chmod(fileinfo.fname, 0, AM_RDO);
            if (f_unlink(fileinfo.fname) != FR_OK)
                return false;
        }
    }

    // readme.txt

    f_chmod("0:readme.txt", 0, AM_RDO | AM_HID | AM_SYS);
    res = f_open(&file, "0:readme.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
        return false;

    res = f_write(&file, readme_txt, sizeof readme_txt - 1, &written);
    if (res != FR_OK || written != sizeof readme_txt - 1)
        return false;

    // obtain pointer to this image's signature block
    const struct Firmware_signature *signature = firmware_signature();

    for (i = 0; i != sizeof readme_info / sizeof readme_info[0]; i++) {
        n = sprintf(buf, readme_info[i]);

        switch (i) {
        case 0:
            if (!signature)
                continue;
            n += sprintf(buf + n, " %s", signature->version_name);
            if (*signature->flavour_name)
                n += sprintf(buf + n, " (%s)", signature->flavour_name);
            break;

        case 1:
            n += sprintf(buf + n, " %s", BOARD_NAME);
#if BOARD == USER_BOARD
            if (hardware_descr.code[0] != HW_SAM4L4
             || xflash_id != 0x16BA20)
                n += sprintf(buf + n, " prototype");
#endif
            break;

        case 2:
            if (!signature)
                continue;
            n += sprintf(buf + n, " %u %u", signature->version_code,
                          signature->flavour_code);
            break;

        case 3:
            n += hexlify(buf + n, &hardware_descr.family, 4);
            break;

        case 4:
            if (!signature)
                continue;
            n += hexlify(buf + n, signature->hash.b, 4);
            break;

        case 5:
            if (!signature)
                continue;
            n += hexlify(buf + n, signature->pubkey.x, 4);
            break;

        case 6:
            if (signature)
                continue;
            break;
        }
        buf[n++] = '\r';
        buf[n++] = '\n';

        res = f_write(&file, buf, n, &written);
        if (res != FR_OK || (int)written != n)
            return false;
    }

    if (f_close(&file) != FR_OK)
        return false;
    f_chmod("0:readme.txt", AM_RDO, AM_RDO);

    // settings.txt

    res = f_open(&file, "0:settings.txt", FA_WRITE | FA_CREATE_NEW);
    if (res != FR_OK)
        return true;        // probably FR_EXIST, which is fine

    res = f_write(&file, settings_txt, sizeof settings_txt - 1, &written);
    if (res != FR_OK || written != sizeof settings_txt - 1)
        return false;

    n = hexlify(buf, mycelium_public_key.x, sizeof mycelium_public_key.x);
    n += sprintf(buf + n, "\r\n    ");
    n += hexlify(buf + n, mycelium_public_key.y, sizeof mycelium_public_key.y);
    buf[n++] = '\r';
    buf[n++] = '\n';

    res = f_write(&file, buf, n, &written);
    if (res != FR_OK || (int)written != n)
        return false;

    return f_close(&file) == FR_OK;
}

static bool find_image(char fname[13], struct Firmware_signature *signature)
{
    FIL file;
    DIR dir;
    FILINFO fileinfo;
    uint32_t ftype = 0;
    DWORD latest_time = 0;

    if (f_opendir(&dir, "0:") != FR_OK)
        return false;

    fileinfo.lfname = 0;
    fileinfo.lfsize = 0;

    for (;;) {
        const char *ext;

        if (f_readdir(&dir, &fileinfo) != FR_OK || !*fileinfo.fname)
            break;      // error or no more entries

        if ((fileinfo.fattrib & (AM_DIR | AM_HID | AM_SYS)) == 0
                && (ext = index(fileinfo.fname, '.')) != 0) {
            if (strcasecmp(ext + 1, "tsk") == 0) {
                strcpy(fname, fileinfo.fname);
                ftype = FW_MAGIC_TASK;
                break;
            }
            if (strcasecmp(ext + 1, "bin") == 0) {
                DWORD datetime = (DWORD) fileinfo.fdate << 16 | fileinfo.ftime;
                if (datetime > latest_time) {
                    latest_time = datetime;
                    strcpy(fname, fileinfo.fname);
                    ftype = FW_MAGIC_ENTROPY;
                }
            }
        }
    }

    if (!ftype)
        return false;   // no image found

    UINT actual;
    f_open(&file, fname, FA_READ);
    // read signature block, using *signature as a temporary buffer to read
    // file header first
    if (f_read(&file, signature, (FW_SIGN_VECTOR + 1) * 4, &actual) != FR_OK
            || actual != (FW_SIGN_VECTOR + 1) * 4)
        ui_error(UI_E_INVALID_IMAGE);
    f_lseek(&file, ((uint32_t *)signature)[FW_SIGN_VECTOR]);
    if (f_read(&file, signature, sizeof *signature, &actual) != FR_OK
            || actual != sizeof *signature)
        ui_error(UI_E_INVALID_IMAGE);

    // check flavour:
    // - regular can be updated to any;
    // - non-regular can be updated to itself or regular
    const struct Firmware_signature *our_sig = firmware_signature();
    if (our_sig && our_sig->flavour_code != FW_FLAVOUR_REGULAR
                && signature->flavour_code != FW_FLAVOUR_REGULAR
                && signature->flavour_code != our_sig->flavour_code)
        ui_error(UI_E_INVALID_IMAGE);

    // check image size, and whether it's our current running image
    uint32_t max_size = 0;
    switch (ftype) {
        case FW_MAGIC_ENTROPY:
            if (hardware_descr.code[0] == HW_SAM4L2)
               max_size = 112 * 1024;
            else
               max_size = 240 * 1024;
            if (our_sig && memcmp(signature->hash.b, our_sig->hash.b,
                                  sizeof our_sig->hash.b) == 0)
                return false;
            break;
        case FW_MAGIC_TASK:
            max_size = applet_max_task;
            break;
    }
    if (f_size(&file) > max_size)
        ui_error(UI_E_INVALID_IMAGE);

    // check image validity
    if (signature->magic != ftype)
        ui_error(UI_E_INVALID_IMAGE);
    if (signature->hardware_family != hardware_descr.family
            || signature->hardware_code[0] > hardware_descr.code[0]
            || signature->hardware_code[1] > hardware_descr.code[1]
            || signature->hardware_code[2] > hardware_descr.code[2])
        ui_error(UI_E_UNSUPPORTED_HARDWARE);

    return true;
}

static void hash_image(const char *fname, uint32_t hash[8])
{
    FIL file;
    UINT actual;
    uint32_t buf[2][SHA256_BLOCK_SIZE / 4];
    uint32_t total = 0;
    uint32_t left = 0;

    f_open(&file, fname, FA_READ);

    // compute hash
    sha256_init(hash);
    for (;;) {
        if (f_read(&file, buf, sizeof buf, &actual) != FR_OK)
            ui_error(UI_E_HARDWARE_FAULT);
        if (!total) {
            // this is the first iteration;
            // extract total number of bytes to hash, which run until
            // the 'hash' member of the image's signature block
            total = buf[0][FW_SIGN_VECTOR] +
                offsetof(struct Firmware_signature, hash);
            left = total;
        }
        if (left <= actual)
            break;
        if (actual != sizeof buf)
            ui_error(UI_E_INVALID_IMAGE);
        sha256_transform(hash, buf[0]);
        sha256_transform(hash, buf[1]);
        left -= sizeof buf;
    }
    sha256_finish(hash, (uint8_t *) buf[0], left, total);

    f_close(&file);
}

static void process_config(const struct Firmware_signature *signature)
{
    struct Raw_public_key sign_keys[5];
    int i, nkeys;

    nkeys = parse_settings(sign_keys, sizeof sign_keys / sizeof sign_keys[0]);

    // check for parse errors and whether there are too many keys
    if (nkeys < 0)
        ui_error(UI_E_BAD_CONFIG);
    if ((size_t) nkeys > sizeof sign_keys / sizeof sign_keys[0])
        ui_error(UI_E_INVALID_KEY);

    // check validity of signing keys
    for (i = 0; i < nkeys; i++) {
        curve_point key;
        printf("Validating key %d... ", i);
        // Mycelium key is known to be valid
        if (memcmp(&sign_keys[i], &mycelium_public_key,
                        sizeof mycelium_public_key) != 0) {
            // check foreign key validity
            bn_read_be(sign_keys[i].x, &key.x);
            bn_read_be(sign_keys[i].y, &key.y);
            if (!ecdsa_validate_pubkey(&key)) {
                puts("oops.");
                ui_error(UI_E_INVALID_KEY);
            }
        }
        puts("done.");
    }

    if (!signature)
        return;

    // check whether the signature is authorised
    i = 0;
    do {
        if (i == nkeys) {
            // actual image signing key not found in settings.txt
            ui_error(UI_E_INVALID_SIGNATURE);
        }
    } while (memcmp(&sign_keys[i++], &signature->pubkey,
                    sizeof signature->pubkey) != 0);

    // confirm if using a non-Mycelium key
    if (memcmp(&signature->pubkey, &mycelium_public_key,
                    sizeof mycelium_public_key) != 0)
        ui_confirm(UI_W_FOREIGN_KEY);
}

static void run_applet(const char *fname, uint32_t ftype, FATFS *fs)
{
    FIL file;
    struct Applet_param *param = (struct Applet_param *) applet_param_addr;

    f_open(&file, fname, FA_READ);

    param->fat_addr = fs->fatbase * 512;
    param->data_addr = fs->database * 512;
    param->csize = fs->csize * 512;
    param->file_scl = file.sclust;
    param->file_size = f_size(&file);
    param->ftype = ftype;

    memcpy((void *) applet_addr, applet_bin, applet_bin_len);

    __set_MSP(applet_addr);
    asm ("bx %0" :: "r" (applet_addr | 1));
}

DWORD get_fattime(void)
{
    return fattime;
}
