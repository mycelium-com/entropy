/*
 * Mycelium Entropy test.
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
#include <board.h>
#include <sleepmgr.h>
#include <adcife.h>
#include <led.h>
#include <udc.h>

#include "conf_usb.h"
#include "conf_access.h"
#include "sys/stdio-uart.h"
#include "sys/now.h"
#include "ui.h"
#include "xflash.h"

static volatile bool main_b_msc_enable = false;

extern unsigned char fs_1_img[];
extern unsigned int fs_1_img_len;
static char *readme = ((char*) fs_1_img) + 0x600;  // char, unsigned char and signed char are 3 different types.
static char *ptr;


static void ast_per_interrupt(void)
{
    static int button_done;

    ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);
    if (ui_btn_count > button_done >> 1) {
        LED_Toggle(LED0);
        button_done++;
    }
}

static void init_ast(void)
{
    ast_init();

    ast_write_periodic0_value(AST, AST_PER_1HZ - 3);    // ~4 Hz flashing
    ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);
    ast_set_callback(AST, AST_INTERRUPT_PER, ast_per_interrupt, AST_PER_IRQn, 1);
}

static unsigned check_chip(void)
{
    static char sam[] = "ATSAM4L... (Rev .)";
    uint32_t cidr = CHIPID->CHIPID_CIDR;
    const char *name = "unknown.\r\n";
    const uint8_t *serial_number = (const uint8_t *)0x80020C;
    int i;

    if ((cidr & 0xFFFEF0E0) == 0xAB0A00E0) {
        uint32_t exid = CHIPID->CHIPID_EXID;
        name = sam;
        sam[7] = exid & 1 << 3 ? 'C' : 'S';
        sam[8] = '0' + (1 << ((cidr >> 9 & 7) - 2));
        sam[9] = 'A' - 2 + (exid >> 24 & 7);
        sam[16] = 'A' + (cidr & 0x1F);
    }

    ptr += sprintf(ptr, "Processor: %s", name);

    if (name == sam)
        ptr += sprintf(ptr, ", %d kB internal flash.\r\n",
                (sam[8] - '0') * 64);

    ptr += sprintf(ptr, "Unique ID:");
    for (i = 0; i < 15; i++)
        ptr += sprintf(ptr, " %02x", serial_number[i]);
    ptr += sprintf(ptr, ".\r\n");

    return serial_number[1] | serial_number[2] << 8;
}

static void check_voltage(void)
{
    static const struct adc_config cfg = {
        .prescal    = ADC_PRESCAL_DIV16,    // assuming PBA is clocked at 12 MHz
        .clksel     = ADC_CLKSEL_APBCLK,
        .speed      = ADC_SPEED_300K,       // affects current consumption
        .refsel     = ADC_REFSEL_0,         // 1V
        .start_up   = CONFIG_ADC_STARTUP,   // cycles of the ADC clock
    };
    static const struct adc_seq_config seq_cfg = {
        .muxneg     = ADC_MUXNEG_1,         // negative input from Pad Ground
        .muxpos     = ADC_MUXPOS_2,         // positive input from Vcc/10
        .internal   = ADC_INTERNAL_3,       // neg and pos internal
        .res        = ADC_RES_12_BIT,
        .bipolar    = ADC_BIPOLAR_SINGLEENDED,
        .gain       = ADC_GAIN_1X,
        .trgsel     = ADC_TRIG_SW,          // software trigger mode
    };
    static struct adc_dev_inst adc;

    adc_init(&adc, ADCIFE, (struct adc_config *)&cfg); // always STATUS_OK
#ifndef ASF_BUG_3257_FIXED
    // adc_init() unintentionally disables clock masks due to bug 3257.
    // We have to restore them here.
    sysclk_enable_pba_divmask(PBA_DIVMASK_TIMER_CLOCK2
        | PBA_DIVMASK_TIMER_CLOCK3
        | PBA_DIVMASK_TIMER_CLOCK4
        | PBA_DIVMASK_TIMER_CLOCK5
        );
#endif

    if (adc_enable(&adc) != STATUS_OK) {
        ptr += sprintf(ptr, "ADC error: cannot enable ADC.\r\n");
        return;
    }

    ADCIFE->ADCIFE_SEQCFG = *(const uint32_t *)&seq_cfg;

    const unsigned nsamples = 4;
    unsigned sample = 0;
    unsigned i;
    for (i = 0; i < nsamples; i++ ) {
        adc_start_software_conversion(&adc);
        do ; while ((adc_get_status(&adc) & ADCIFE_SR_SEOC) == 0);
        sample += adc_get_last_conv_value(&adc);
        adc_clear_status(&adc, ADCIFE_SCR_SEOC);
    }

    sample = sample * 10000 / (nsamples * 4095);    // convert to mV
    ptr += sprintf(ptr, "VDD: %u.%03u V.\r\n", sample / 1000, sample % 1000);
}

static void patch_fat(unsigned unique_id, unsigned length)
{
    char id[10];
    uint8_t *p = fs_1_img + 0x440;  // start of main directory entry
    uint8_t sum = 0;
    int i;

    sprintf(id, "%04X", unique_id);
    memcpy(p + 3, id, 4);
    *(uint32_t *)(void*)(p + 0x1c) = length;
    p[-25] = id[0];
    p[-23] = id[1];
    p[-18] = id[2];
    p[-16] = id[3];

    // compute short name checksum
    for (i = 11; i; --i)
        sum = ((sum & 1) << 7) + (sum >> 1) + *p++;
    p[-30] = sum;

    // adjust FAT if file not longer than 512 bytes
    if (length <= 512) {
        fs_1_img[0x203] = 0xff;
        fs_1_img[0x204] = 0x0f;
        fs_1_img[0x205] = 0x00;
    }
}

int main(void)
{
    sysclk_init();
    dfll_enable_config_defaults(0);
    board_init();
    ui_init();

    strcpy(readme, "-- MYCELIUM ENTROPY TEST --\r\n\r\n");
    ptr = readme + strlen(readme);

    /* Initialize the console uart */
    stdio_uart_init();
    printf("-- Mycelium Entropy Test --\n");
    init_ast();
    ast_enable_interrupt(AST, AST_INTERRUPT_PER);

    unsigned unique_id = check_chip();
    check_voltage();
    xflash_check(&ptr, readme, 1024);

    puts(readme);

    patch_fat(unique_id, ptr - readme);

    // Start USB stack to authorize VBus monitoring
    udc_start();

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
    }
}

void main_suspend_action(void)
{
    ui_powerdown();
}

void main_resume_action(void)
{
    ui_wakeup();
}

void main_sof_action(void)
{
    if (!main_b_msc_enable)
        return;
    //ui_process(udd_get_frame_number());
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

bool read_only(void)
{
    return true;
}
