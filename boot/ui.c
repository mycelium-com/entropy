/*
 * User interface for SAM4L Bootloader.
 * Uses LED where available.
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

#include <board.h>
#include <interrupt.h>
#include <ioport.h>
#include <led.h>
#include <tc.h>
#include "ui.h"

#ifdef LED0

#define TC          TC0
#define TC_CHANNEL  2
#define TC_Handler  TC02_Handler
#define TC_IRQn     TC02_IRQn
#define TC_IRQ_PRIO 2

enum {
    TC_CLK_FREQ = 12000000,
    PWM_FREQ    = 200,
    GLOW_PERIOD = 2,

    TC_PERIOD   = TC_CLK_FREQ / 8 / PWM_FREQ,
    TC_PWM_STEP = TC_PERIOD * 2 / PWM_FREQ / GLOW_PERIOD,
};

static int duty = 100;
static int step = TC_PWM_STEP;

// Note: PBA peripheral clock must be enabled by caller.
void ui_init(void)
{
#ifdef LED_MASK
    ioport_set_port_dir(LED_PORT, LED_MASK, IOPORT_DIR_OUTPUT);
    ioport_set_port_level(LED_PORT, LED_MASK, LED_INACTIVE);
    ioport_set_port_mode(LED_PORT, LED_MASK, IOPORT_MODE_DRIVE_STRENGTH);
#else
    ioport_set_pin_dir(LED0_PIN, IOPORT_DIR_OUTPUT);
    ioport_set_pin_level(LED0_PIN, LED0_INACTIVE);
    ioport_set_pin_mode(LED0_PIN, IOPORT_MODE_DRIVE_STRENGTH);
#endif

    tc_init(TC, TC_CHANNEL,
            TC_CMR_TCCLKS_TIMER_CLOCK3 |    // PBA clock / 8
            TC_CMR_WAVE |                   // waveform mode is enabled
            TC_CMR_WAVSEL_UP_AUTO);         // UP mode, trigger on RC Compare
    tc_write_rc(TC, TC_CHANNEL, TC_PERIOD);
    tc_enable_interrupt(TC, TC_CHANNEL,
            TC_IER_CPAS |                   // interrupt on RA Compare
            TC_IER_CPCS);                   // interrupt on RC Compare

    irq_register_handler(TC_IRQn, TC_IRQ_PRIO); // enable interrupt in NVIC

    tc_start(TC, TC_CHANNEL);
}

void ui_stop(void)
{
    tc_stop(TC, TC_CHANNEL);
}

void TC_Handler(void)
{
    unsigned status = tc_get_status(TC, TC_CHANNEL);

    if (status & TC_SR_CPCS) {
        // RC compare has occurred, i.e. end of period
        if (duty >= TC_PERIOD || (duty < 35 && step < 0))
            step = -step;
        duty += step * duty / 1024;
        tc_write_ra(TC, TC_CHANNEL, duty);
        LED_On(LED0);
    } else if (status & TC_SR_CPAS) {
        LED_Off(LED0);
    }
}

#endif
