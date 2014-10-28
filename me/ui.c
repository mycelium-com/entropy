/*
 * User interface.
 * Button and LED handling.
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

#include <led.h>
#include <ioport.h>
#include <gpio.h>
#include <tc.h>

#include "sys/conf_system.h"
#include "sys/now.h"
#include "xflash.h"
#include "ui.h"

#include <stdio.h>

// Parameters for the slow pulsating mode.
enum {
    TC_CLK_FREQ = 48000000 / 8,
    PWM_FREQ    = 200,
    GLOW_PERIOD = 2,

    TC_PERIOD   = TC_CLK_FREQ / PWM_FREQ,
    TC_PWM_STEP = TC_PERIOD * 2 / PWM_FREQ / GLOW_PERIOD,
};

static int duty;
static int step = TC_PWM_STEP / 2;
static int led_pulses, led_pause, led_extend;

volatile int ui_btn_count;
volatile uint32_t ui_btn_timestamp;

static void button_interrupt(void)
{
    ui_btn_count++;
    ui_btn_timestamp = now();
    LED_On(LED0);
    led_extend = 2;
}

static void ast_per_interrupt(void)
{
    ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);

    if (led_extend)
        --led_extend;
    else if (!led_pulses)
        LED_Toggle(LED0);
    else {
        static int cnt;

        if (cnt >= 0)
            LED_Toggle(LED0);
        if (++cnt >= led_pulses)
            cnt = -led_pause;
    }

    xflash_periodic_interrupt();
}

void ui_init(void)
{
    // Initialise LED.
    LED_On(LED0);

    // Initialise AST for LED blinking.
    ast_init();

    // Initialise button.
    ui_btn_count = 0;
    ioport_set_pin_dir(BUTTON_0_PIN, IOPORT_DIR_INPUT);
    ioport_set_pin_mode(BUTTON_0_PIN, IOPORT_MODE_PULLUP |
            IOPORT_MODE_GLITCH_FILTER);
    ioport_set_pin_sense_mode(BUTTON_0_PIN, IOPORT_SENSE_FALLING);
    if (!gpio_set_pin_callback(BUTTON_0_PIN, button_interrupt, CONFIG_BUTTON_IRQ_PRIO))
        puts("Error setting GPIO callback.");
    gpio_enable_pin_interrupt(BUTTON_0_PIN);

    // Initialise Timer-Counter for the glow/pulsating effect using PWM.
    // Note: PBA peripheral clock must be enabled by caller.
    tc_init(TC_UI, TC_UI_CHANNEL,
            TC_CMR_TCCLKS_TIMER_CLOCK3 |    // PBA clock / 8
            TC_CMR_WAVE |                   // waveform mode is enabled
            TC_CMR_WAVSEL_UP_AUTO);         // UP mode, trigger on RC Compare
    tc_write_rc(TC_UI, TC_UI_CHANNEL, TC_PERIOD);

    duty = tc_read_ra(TC_UI, TC_UI_CHANNEL) << 2;
    if (duty < 35)
        duty = 100;

    irq_register_handler(TC_UI_IRQn, TC_UI_IRQ_PRIO);   // enable IRQ in NVIC
}

// Key generation mode: rapid blinking.
void ui_keygen(void)
{
    led_pulses = 0;
    tc_stop(TC_UI, TC_UI_CHANNEL);
    ast_write_periodic0_value(AST, AST_PER_1HZ - 4);    // ~8 Hz blinking
    ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);
    ast_set_callback(AST, AST_INTERRUPT_PER, ast_per_interrupt, AST_PER_IRQn,
                     CONFIG_AST_IRQ_PRIO);
}

void ui_pulsate(void)
{
    tc_enable_interrupt(TC_UI, TC_UI_CHANNEL,
            TC_IER_CPAS |                   // interrupt on RA Compare
            TC_IER_CPCS);                   // interrupt on RC Compare
    tc_start(TC_UI, TC_UI_CHANNEL);
}

void ui_off(void)
{
    tc_stop(TC_UI, TC_UI_CHANNEL);
    tc_disable_interrupt(TC_UI, TC_UI_CHANNEL,
            TC_IER_CPAS |                   // interrupt on RA Compare
            TC_IER_CPCS);                   // interrupt on RC Compare
    ast_disable_interrupt(AST, AST_INTERRUPT_PER);
    LED_Off(LED0);
}

void ui_btn_off(void)
{
    gpio_disable_pin_interrupt(BUTTON_0_PIN);
}

static void ui_msg(enum Ui_message_code code)
{
    tc_stop(TC_UI, TC_UI_CHANNEL);
    LED_Off(LED0);
    ast_write_periodic0_value(AST, AST_PER_1HZ - 3);    // ~4 Hz blinking
    ast_enable_interrupt(AST, AST_INTERRUPT_PER);
    led_extend = 4;
    led_pulses = code << 1;
    led_pause = 8;
    printf("UI message %d.\n", code);
}

void ui_error(enum Ui_message_code code)
{
    gpio_disable_pin_interrupt(BUTTON_0_PIN);
    ui_msg(code);
    for (;;);
}

void ui_confirm(enum Ui_message_code code)
{
#if 1
    ui_msg(code);
#else
    // experimental "railway crossing" type notification
    tc_stop(TC_UI, TC_UI_CHANNEL);
    LED_Off(LED0);
    LED_On(LED1);
    ast_write_periodic0_value(AST, AST_PER_1HZ - 3);    // ~4 Hz blinking
    ast_enable_interrupt(AST, AST_INTERRUPT_PER);
    printf("UI confirm.\n");
#endif
    ui_btn_count = 0;
    do ; while (ui_btn_count == 0);
    ui_keygen();
}

void ui_powerdown(void)
{
    LED_Off(LED0);
}

void ui_wakeup(void)
{
    //LED_On(LED0);
}

void ui_start_read(void)
{
    LED_Toggle(LED0);
}

void ui_stop_read(void)
{
    LED_Toggle(LED0);
}

void ui_start_write(void)
{
}

void ui_stop_write(void)
{
}

void ui_process(uint16_t framenumber)
{
    if (0 == framenumber) {
        LED_On(LED0);
    }
    if (1000 == framenumber) {
        LED_Off(LED0);
    }
}

void TC_UI_Handler(void)
{
    unsigned status = tc_get_status(TC_UI, TC_UI_CHANNEL);

    if (status & TC_SR_CPCS) {
        // RC compare has occurred, i.e. end of period
        if (duty >= TC_PERIOD || (duty < 35 && step < 0))
            step = -step;
        duty += step * duty >> 11;
        tc_write_ra(TC_UI, TC_UI_CHANNEL, duty);
        LED_On(LED0);
    } else if (status & TC_SR_CPAS) {
        LED_Off(LED0);
    }
}
