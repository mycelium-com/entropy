/*
 * Initial factory firmware and self test for Mycelium Entropy.
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
#include <string.h>
#include <board.h>
#include <adcife.h>
#include <flashcalw.h>
#include <ctrl_access.h>
#include <led.h>
#include <udc.h>

#include "conf_usb.h"
#include "sys/conf_system.h"
#include "sys/stdio-uart.h"
#include "sys/now.h"
#include "sys/sync.h"
#include "lib/fwsign.h"
#include "ui.h"
#include "xflash.h"
#include "main.h"

static bool enable_report = false;

static volatile bool main_b_msc_enable = false;
static volatile unsigned usb_frames_received;
static volatile bool was_suspended = false;

extern uint32_t __ram_end__;
extern const unsigned char boot_bin[];
extern const int boot_bin_len;
extern const unsigned char ifp_bin[];
extern const int ifp_bin_len;
extern unsigned char fs_1_img[];
extern unsigned int fs_1_img_len;
static char *readme = ((char*) fs_1_img) + 0x600;  // char, unsigned char and signed char are 3 different types.
static char *ptr;

#define APP_START_ADDRESS   0x4000
#define APP_START_PAGE      (APP_START_ADDRESS/FLASH_PAGE_SIZE)

void set_boot_pin(void);
bool flash_bootloader(void);
static void run_ifp(void) __attribute__ ((noreturn));

static int faults;

void set_fault(enum Faults fault)
{
    if ((fault | faults) >= 4) {
        if (fault > faults)
            faults = fault;
    } else
        faults |= fault;
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

    if (name != sam || sam[8] != '4')
        set_fault(WRONG_MCU);

    return serial_number[1] | serial_number[2] << 8;
}

static bool check_voltage(void)
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
        return true;
    }

    ADCIFE->ADCIFE_SEQCFG = *(const uint32_t *)&seq_cfg;

    const unsigned nsamples = 64;
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

    if (sample < 3000 || sample >= 3600) {
        set_fault(VDD_FAULT);
        return false;
    }
    return true;
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
    // Check the flag passed by the bootloader to determine if we should do
    // initial flash programming.
    bool do_ifp = (CONFIG_FW_REG & CONFIG_FW_MASK) != 0;

    if (do_ifp)
        run_ifp();

    sysclk_init();
    board_init();
    ui_init();
    sync_init();
    stdio_uart_init();

    strcpy(readme, "-- MYCELIUM ENTROPY TEST --\r\n\r\n");
    ptr = readme + strlen(readme);

    unsigned unique_id = check_chip();

    // Temporarily reduce PBA speed to 12 MHz while the ADC is in operation.
    sysclk_set_prescalers(0, 2, 0, 0, 0);
    bool voltage_ok = check_voltage();
    sysclk_set_prescalers(0, 0, 0, 0, 0);

    if (!xflash_check(&ptr, readme, 1024))
        set_fault(FLASH_FAULT);

    appname("Mycelium Entropy Factory Test");
    puts(readme);

    patch_fat(unique_id, ptr - readme);

    if (voltage_ok)
        set_boot_pin();

    if (faults)
        enable_report = true;

    // Start USB stack.
    udc_start();

    // Timestamp to detect if USB is not connected within 5 seconds.
    uint32_t ts = now();

    while (true) {

        if (main_b_msc_enable) {
            if (!udi_msc_process_trans()) {
                //sleepmgr_enter_sleep();
            }
        }
        if (was_suspended) {
            puts("USB was suspended.\n");
            set_fault(USB_FAULT);
            break;
        } else if (usb_frames_received > 1000 && !was_suspended && sync_ok()) {
            static bool connected;
            if (connected)
                continue;
            connected = true;
            puts("USB OK.");
            if (!faults)
                break;
            ui_error(faults);
        } else if ((now() - ts) > 2 * AST_FREQ) {
            printf("USB connection failed: %u frames,%s %s.\n",
                    usb_frames_received,
                    was_suspended ? " was suspended," : "",
                    sync_ok() ? "sync ok" : "not in sync");
            set_fault(USB_FAULT);
            break;
        }

        sync_diag_print();
    }

    udc_stop();

    if (!faults && !flash_bootloader())
        set_fault(BOOTLOADER_FAULT);

    if (faults)
        ui_error(faults);
    else
        ui_ok();

    for (;;);
}

void main_suspend_action(void)
{
    sync_suspend();
    if (usb_frames_received >= 100)
        was_suspended = true;
}

void main_resume_action(void)
{
}

void main_sof_action(void)
{
    sync_frame();
    usb_frames_received++;
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

Ctrl_status report_test_unit_ready(void)
{
    return enable_report ? CTRL_GOOD : CTRL_NO_PRESENT;
}

// Program the bootloader configuration word in the User page.
// This selects GPIO pin that can be used to enter bootloader.
// We use our button for this.
__attribute__ ((section (".ramfunc")))
void set_boot_pin(void)
{
    cpu_irq_enter_critical();

    uint32_t *boot_config = (uint32_t *) (FLASH_USER_PAGE_ADDR + 0x10);
    uint32_t boot_config_value = ~0x1fful | BUTTON_0_ACTIVE << 8 | BUTTON_0_PIN;
    if (*boot_config != boot_config_value)
        flashcalw_memcpy(boot_config, &boot_config_value, 4, true);

    cpu_irq_leave_critical();
}

// Replace the original bootloader with our own version.
__attribute__ ((section (".ramfunc")))
bool flash_bootloader(void)
{
    unsigned pages_per_region = flashcalw_get_page_count_per_region();
    bool use_region_1 = APP_START_PAGE > pages_per_region;

    cpu_irq_enter_critical();

    // Unlock bootloader region(s).
    flashcalw_lock_page_region(0, false);
    if (use_region_1)
        flashcalw_lock_page_region(pages_per_region, false);

    // Flash bootloader from boot_bin[] at address 0.
    memcpy(readme, boot_bin, boot_bin_len);
    flashcalw_memcpy((volatile void *) 0, readme, boot_bin_len, true);

    // Lock bootloader region(s).
    flashcalw_lock_page_region(0, true);
    if (use_region_1)
        flashcalw_lock_page_region(pages_per_region, true);

    cpu_irq_leave_critical();

    // Verify.
    // memcmp() requires addresses to be non-null, so we verify the value
    // at address 0 separately.
    return *(unsigned char *)0 == boot_bin[0]
        && memcmp((void *) 1, boot_bin + 1, boot_bin_len - 1) == 0;
}

// Load and run the Initial Flash Programming applet in SRAM.
static void run_ifp(void)
{
    uint32_t *ram = (uint32_t *) 0x20000000;

    // Allow Entropy firmware to run after IFP without complaining about
    // lack of entropy.
    __ram_end__ = 239;

    memcpy(ram, ifp_bin, ifp_bin_len);

    // Exception table starts as follows:
    //   ram[0] = initial stack pointer
    //   ram[1] = reset vector
    // This code depends on factory test's stack pointer being higher
    // than ifp's.
    // That way we don't need to deal with it in a potentially problematic
    // way.
    //__set_MSP(ram[0]);
    asm ("bx %0" :: "r" (ram[1]));
    for (;;);   // to satisfy __attribute__ ((noreturn))
}
