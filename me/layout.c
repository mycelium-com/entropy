/*
 * Picture layouts for JPEG generator.
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

#include <stdint.h>
#include "qr.h"
#include "jpeg-data.h"
#include "layout.h"

/*
 * This is middle ground between A4 and Letter, with the theoretical target
 * aspect ratio of 1.3445589513333456.
 * An A4 printer will cut off 6 macroblocks from either side.
 * A Letter printer will chop off 6 macroblocks from top and bottom.
 * The recommended margins are 20 on each side.
 */

const struct Layout main_layout[] = {
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 20,
        .x      = 38,
        .picref = &bitcoin_address_ref,         // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = 156,
        .pic    = private_key_fragment,         // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_QR,
        .vstep  = 10,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_QR,
        .x      = JWIDTH - 29 - 40 - 20,
        .qr     = { .idx = IDX_PRIVKEY, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 2,
        .x      = JWIDTH - 20 - 36,
        .text   = { .idx = IDX_PRIVKEY, .width = 12 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 6,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 18,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 19 + 4,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    {
        .type   = FGM_STOP,
        .vstep  = JHEIGHT - 19 - 4 - 6 - 29 - 16 - 20,
    },
};

const struct Layout shamir_layout[] = {
    {
        .type   = FGM_PICTURE,
        .vstep  = 20,
        .x      = 142,
        .pic    = private_key_fragment,             // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .x      = 170,
        .pic    = share_1_of_3_fragment,            // 30x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 1,
        .x      = 38,
        .picref = &bitcoin_address_ref,             // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 73 - 19,
        .pic    = any_two_shares_reveal_fragment,   // 74x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 9,
        .x      = JWIDTH - 33 - 37 - 20,
        .qr     = { .idx = IDX_SSS_PART(1), .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_QR,
        .vstep  = 2,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 0,
        .x      = JWIDTH - 20 - 33,
        .text   = { .idx = IDX_SSS_PART(1), .width = 11 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 19,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 13 + 4,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_LARGE_PICTURE,
        .vstep  = 5 + 7,
        .pic    = cut_here_fragment,                // 5
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_PICTURE,
        .vstep  = 5 + 10,
        .x      = 142,
        .pic    = private_key_fragment,             // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .x      = 170,
        .pic    = share_2_of_3_fragment,            // 30x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 1,
        .x      = 38,
        .picref = &bitcoin_address_ref,             // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 73 - 19,
        .pic    = any_two_shares_reveal_fragment,   // 74x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 9,
        .x      = JWIDTH - 33 - 37 - 20,
        .qr     = { .idx = IDX_SSS_PART(2), .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_QR,
        .vstep  = 2,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 0,
        .x      = JWIDTH - 20 - 33,
        .text   = { .idx = IDX_SSS_PART(2), .width = 11 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 19,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 13 + 4,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_LARGE_PICTURE,
        .vstep  = 5 + 7,
        .pic    = cut_here_fragment,                // 5
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_PICTURE,
        .vstep  = 5 + 10,
        .x      = 142,
        .pic    = private_key_fragment,             // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .x      = 170,
        .pic    = share_3_of_3_fragment,            // 30x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 1,
        .x      = 38,
        .picref = &bitcoin_address_ref,             // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 73 - 19,
        .pic    = any_two_shares_reveal_fragment,   // 74x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 9,
        .x      = JWIDTH - 33 - 37 - 20,
        .qr     = { .idx = IDX_SSS_PART(3), .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_QR,
        .vstep  = 2,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 0,
        .x      = JWIDTH - 20 - 33,
        .text   = { .idx = IDX_SSS_PART(3), .width = 11 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 19,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 13 + 4,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_STOP,
        .vstep  = JHEIGHT - 3 * (23 + 29 + 31) - 2 * 12 - 5,
    },
};

const struct Layout salt1_layout[] = {
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 20,
        .x      = 38,
        .picref = &bitcoin_address_ref,         // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = 156,
        .pic    = private_key_fragment,         // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_QR,
        .vstep  = 10,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_QR,
        .x      = JWIDTH - 29 - 40 - 20,
        .qr     = { .idx = IDX_PRIVKEY, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 2,
        .x      = JWIDTH - 20 - 36,
        .text   = { .idx = IDX_PRIVKEY, .width = 12 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 6,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 18,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 19 + 4,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_LARGE_PICTURE,
        .vstep  = 5 + 7,
        .pic    = cut_here_fragment,                // 5
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_PICTURE,
        .vstep  = 5 + 9,
        .x      = 20 + QR_SIZE(4) + 4,
        .pic    = entropy_for_verification_fragment,    // 57x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 1,
        .x      = 20,
        .qr     = { .idx = IDX_UNSALTED, .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7 - 1 + 4,
        .x      = 20 + QR_SIZE(4) + 4,
        .text   = { .idx = IDX_UNSALTED, .width = 36 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 10 + 4,
        .x      = 20 + QR_SIZE(4) + 4,
        .pic    = key_sha_256_salt1_fragment,           // 77x7
    },
    {
        .type   = FGM_STOP,
        .vstep  = JHEIGHT - 51 - 19 - 4 - 6 - 29 - 16 - 20,
    },
};

const struct Layout shamir_salt1_layout[] = {
    {
        .type   = FGM_PICTURE,
        .vstep  = 20,
        .x      = 142,
        .pic    = private_key_fragment,             // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .x      = 170,
        .pic    = share_1_of_3_fragment,            // 30x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 1,
        .x      = 38,
        .picref = &bitcoin_address_ref,             // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 73 - 19,
        .pic    = any_two_shares_reveal_fragment,   // 74x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 9,
        .x      = JWIDTH - 33 - 37 - 20,
        .qr     = { .idx = IDX_SSS_PART(1), .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_QR,
        .vstep  = 2,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 0,
        .x      = JWIDTH - 20 - 33,
        .text   = { .idx = IDX_SSS_PART(1), .width = 11 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 19,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_LARGE_PICTURE,
        .vstep  = 13 + 4,
        .pic    = cut_here_fragment,                // 5
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_PICTURE,
        .vstep  = 5 + 4,
        .x      = 142,
        .pic    = private_key_fragment,             // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .x      = 170,
        .pic    = share_2_of_3_fragment,            // 30x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 1,
        .x      = 38,
        .picref = &bitcoin_address_ref,             // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 73 - 19,
        .pic    = any_two_shares_reveal_fragment,   // 74x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 9,
        .x      = JWIDTH - 33 - 37 - 20,
        .qr     = { .idx = IDX_SSS_PART(2), .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_QR,
        .vstep  = 2,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 0,
        .x      = JWIDTH - 20 - 33,
        .text   = { .idx = IDX_SSS_PART(2), .width = 11 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 19,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_LARGE_PICTURE,
        .vstep  = 13 + 4,
        .pic    = cut_here_fragment,                // 5
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_PICTURE,
        .vstep  = 5 + 4,
        .x      = 142,
        .pic    = private_key_fragment,             // 28x7
    },
    {
        .type   = FGM_PICTURE,
        .x      = 170,
        .pic    = share_3_of_3_fragment,            // 30x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_PICTURE_BY_REF,
        .vstep  = 1,
        .x      = 38,
        .picref = &bitcoin_address_ref,             // 38x5
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 73 - 19,
        .pic    = any_two_shares_reveal_fragment,   // 74x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 9,
        .x      = JWIDTH - 33 - 37 - 20,
        .qr     = { .idx = IDX_SSS_PART(3), .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_QR,
        .vstep  = 2,
        .x      = 20,
        .qr     = { .idx = IDX_ADDRESS, .size = QR_SIZE(3) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 0,
        .x      = JWIDTH - 20 - 33,
        .text   = { .idx = IDX_SSS_PART(3), .width = 11 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7,
        .x      = 20 + 29 + 4,
        .text   = { .idx = IDX_ADDRESS, .width = 12 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 19,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_LARGE_PICTURE,
        .vstep  = 13 + 4,
        .pic    = cut_here_fragment,                // 5
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_PICTURE,
        .vstep  = 5 + 3,
        .x      = 20 + QR_SIZE(4) + 4,
        .pic    = entropy_for_verification_fragment,    // 57x7
    },
    {
        .type   = FGM_QR,
        .vstep  = 1,
        .x      = 20,
        .qr     = { .idx = IDX_UNSALTED, .size = QR_SIZE(4) },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 7 - 1 + 2,
        .x      = 20 + QR_SIZE(4) + 4,
        .text   = { .idx = IDX_UNSALTED, .width = 36 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 10 + 2,
        .x      = 20 + QR_SIZE(4) + 4,
        .pic    = key_sha_256_salt1_fragment,           // 77x7
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 7 + 1,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    //------------------------------------------------------------------
    {
        .type   = FGM_STOP,
        .vstep  = 25,
    },
};

const struct Layout hd_layout[] = {
    {
        .type   = FGM_PICTURE,
        .vstep  = 20,
        .x      = 20 + (28*3 - BITCOIN_ADDRESS_FRAGMENT_WIDTH + 1)/2,
        .pic    = bitcoin_account_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .x      = JWIDTH - 21 - 28*3 + (28*3 - SEED_FRAGMENT_WIDTH + 1)/2,
        .pic    = seed_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 6,
        .x      = JWIDTH - 21 - 28*3 + (28*3 - KEEP_SAFE_FRAGMENT_WIDTH + 1)/2,
        .pic    = keep_safe_fragment,
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 1,
        .x      = 20,
        .text   = { .idx = IDX_HD_PATH, .width = 32, .centre = 28 },
    },
    {
        .type   = FGM_TEXT,
        .vstep  = 8,
        .x      = 20,
        .text   = { .idx = IDX_XPUB, .width = 28 },
    },
    {
        .type   = FGM_TEXT,
        .x      = JWIDTH - 21 - 28*3,
        .text   = { .idx = IDX_PRIVKEY, .width = 28 },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = 7,
        .x      = (JWIDTH - LOGO_TOP_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_top_fragment,
    },
    {
        .type   = FGM_QR,
        .vstep  = 17,
        .x      = 20,
        .qr     = { .idx = IDX_XPUB, .size = QR_SIZE(6) },
    },
    {
        .type   = FGM_QR,
        .x      = JWIDTH - 20 - QR_SIZE(6),
        .qr     = { .idx = IDX_PRIVKEY, .size = QR_SIZE(6) },
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_TOP_FRAGMENT_HEIGHT - 17,
        .x      = (JWIDTH - LOGO_MIDDLE_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_middle_fragment,
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_MIDDLE_FRAGMENT_HEIGHT,
        .x      = (JWIDTH - LOGO_BOTTOM_FRAGMENT_WIDTH + 1) / 2,
        .pic    = logo_bottom_fragment,             // 66x13
    },
    {
        .type   = FGM_PICTURE,
        .vstep  = LOGO_BOTTOM_FRAGMENT_HEIGHT + 4,
        .x      = JWIDTH - 19 - SMALL_PRINT_FRAGMENT_WIDTH,
        .pic    = small_print_fragment,
    },
    {
        .type   = FGM_STOP,
        .vstep  = JHEIGHT - 20 - 22 - 58 - 4,
    },
};
