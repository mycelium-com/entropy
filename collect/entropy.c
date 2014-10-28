/*
 * Entropy collection utiliy.
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
#include <bpm.h>
#include <eic.h>
#include <ast.h>
#include <led.h>
#include <sysclk.h>
#include <delay.h>
#include <status_codes.h>
#include <conf_board.h>

#ifdef CONF_BOARD_SD_MMC_SPI
#include <sd_mmc.h>
#endif

#ifdef TEMP_TWIM
#include "temperature.h"
#endif
#include "sys/stdio-uart.h"


enum {
    PORTION = 16384,
    LOGSEC  = 5,
};


// Initialise AST.
static void config_ast(void)
{
    struct ast_config ast_conf;

    /* Enable RC32K oscillator */
#define OSC OSC_ID_RC32K
    if (!osc_is_ready(OSC)) {
        osc_enable(OSC);
        osc_wait_ready(OSC);
    }
    bpm_set_clk32_source(BPM, BPM_CLK32_SOURCE_RC32K);

    ast_conf.mode = AST_COUNTER_MODE;
    ast_conf.osc_type = AST_OSC_32KHZ;  // bpm does not enable 1 kHz on RC32K
    ast_conf.psel = AST_PSEL_32KHZ_1HZ;
    ast_conf.counter = 0;
    if (!ast_set_config(AST, &ast_conf)) {
        puts("\nError initialising AST.");
        bpm_set_clk32_source(BPM, BPM_CLK32_SOURCE_OSC32K);
        return;
    }
    ast_stop(AST);
    ast_clear_prescalar(AST);

    /* Set up periodic interrupt. */
    ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);
    ast_write_periodic0_value(AST, LOGSEC + 15);      // 32 kHz / 2^n
    ast_enable_wakeup(AST, AST_WAKEUP_PER);
}

// Configure backup mode wakeup.
static void config_backup_wakeup(void)
{
    /* Only AST can wakeup the device */
    bpm_enable_wakeup_source(BPM, 1 << BPM_BKUPWEN_AST);

    /*
     * Retain I/O lines after wakeup from backup.
     * Disable to undo the previous retention state then enable.
     */
    bpm_disable_io_retention(BPM);
    bpm_enable_io_retention(BPM);
    /* Enable fast wakeup */
    //bpm_enable_fast_wakeup(BPM);
}


int main(void)
{
    sysclk_init();
    board_init();
    stdio_uart_init();

    config_ast();

    // On wakeup from backup mode, I/O pins are held in their pre-backup state
    // by BPM.  Calling this here will release them.  This should be done after
    // configuring UART to prevent noise on TX.
    config_backup_wakeup();

    unsigned cycle = BSCIF->BSCIF_BR[0].BSCIF_BR;
    uint32_t from_bkup = PM->PM_RCAUSE & 1 << 6;

    if (cycle & 1)
        LED_On(LED0);
    else
        LED_Off(LED0);

#ifdef CONF_BOARD_SD_MMC_SPI
    sd_mmc_init();
#endif

    if (!from_bkup) {
        puts("\n-- Entropy collection --");
        puts(BOARD_NAME);
        printf("Sample size: %d bytes.\nInterval: %d sec.\n",
                PORTION, 1 << LOGSEC);
    }

#ifdef TEMP_TWIM
    /* Initialise temperature sensor, if available */
    bool have_temperature = false;
    have_temperature = temperature_init() == STATUS_OK;
    if (!have_temperature)
        puts("Error initialising temperature sensor.");
#endif

    uint32_t * const page = (uint32_t *)(0x20008000 - PORTION);

#ifdef CONF_BOARD_SD_MMC_SPI
    unsigned sd_capacity = 0;
    do ; while (sd_mmc_check(0) == SD_MMC_INIT_ONGOING);
    sd_mmc_check(0);
    sd_mmc_check(0);
    if (sd_mmc_check(0) == SD_MMC_OK) {
        sd_capacity = sd_mmc_get_capacity(0);
        if (!from_bkup)
            printf("SD/MMC: %u MB.\n", sd_capacity / 1024);
    } else
        printf("SD/MMC check error %d.\n", sd_mmc_check(0));
#endif

    char tbuf[32];
    tbuf[0] = 0;
#ifdef TEMP_TWIM
    if (have_temperature && temperature_read(&t) == STATUS_OK) {
        snprintf(tbuf, sizeof tbuf - 1, " temperature %d", t * 5);
        int i = strlen(tbuf);
        tbuf[i] = tbuf[i-1];
        tbuf[i-1] = '.';
        tbuf[i+1] = 0;
    }
#endif

#ifdef CONF_BOARD_SD_MMC_SPI
    unsigned start = cycle * (1 + PORTION / SD_MMC_BLOCK_SIZE);
    bool sd_full = start + (1 + PORTION / SD_MMC_BLOCK_SIZE)
            > sd_capacity * (1024 / SD_MMC_BLOCK_SIZE);

    if (sd_capacity && sd_full) {
        for (;;) {
            LED_On(LED0);
            delay_ms(500);
            LED_Off(LED0);
            delay_ms(500);
        }
    }

    if (sd_capacity) {
        uint32_t blk[SD_MMC_BLOCK_SIZE / sizeof (uint32_t)];
        memset(blk, 0, sizeof blk);
        blk[0] = cycle;
        blk[1] = t;
        blk[2] = PORTION;
        blk[3] = 1 << LOGSEC;
        if (sd_mmc_init_write_blocks(0, start, 1 + PORTION / SD_MMC_BLOCK_SIZE) == SD_MMC_OK) {
            sd_mmc_err_t e1, e2, e3;
            e1 = sd_mmc_start_write_blocks(blk, 1);
            e2 = sd_mmc_wait_end_of_write_blocks();
            e3 = sd_mmc_start_write_blocks(page, PORTION / SD_MMC_BLOCK_SIZE);
            if (e1 | e2 | e3)
                printf("SD/MMC error %d %d %d\n", e1, e2, e3);
        } else
            printf("Init write blocks failed.\n");
    }
#endif

    printf("\nCycle %07u (from %s)%s\n", cycle,
            from_bkup ? "backup mode" : "reset", tbuf);
    if (!cycle != !from_bkup)
        printf("!!! Cycle number invalid for the reset cause !!!\n");

    for (int i = 0; i < PORTION / 4; i++) {
        printf(" %08lx", page[i]);
        if ((i & 7) == 7)
            printf("\n");
        page[i] = 0;
    }

#ifdef CONF_BOARD_SD_MMC_SPI
    if (sd_capacity)
        sd_mmc_wait_end_of_write_blocks();
#endif

    BSCIF->BSCIF_BR[0].BSCIF_BR = cycle + 1;

    bpm_configure_power_scaling(BPM, BPM_PS_1, BPM_PSCM_CPU_NOT_HALT);
    while ((bpm_get_status(BPM) & BPM_SR_PSOK) == 0);

    ast_start(AST);
    /*
     * Wait for the printf operation to finish before setting the
     * device in a power save mode.
     */
    delay_ms(30);
    bpm_sleep(BPM, BPM_SM_BACKUP);
    printf("\n--Exit Backup mode: should never happen.\n");

    return 0;
}
