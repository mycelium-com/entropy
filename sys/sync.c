/*
 * Clock synchronisation with USB.
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

/*
 * SOF tokens from host are decoded well by the USB controller even with
 * very imprecise clocking, e.g. at 45.2 MHz, as confirmed by experiment.
 * Thus, we can use the SOF interrupt to synchronise our clock to USB.
 *
 * CPU and USB are clocked by DFLL, which starts in closed loop mode,
 * multiplying the RC32K clock to get approximately 48 MHz.  After attaching
 * USB, and also when going out of suspend, we switch DFLL to open loop mode
 * and begin tracking USB frame frequency every second.
 *
 * This achieves a typical precision of 1000 ppm and always keeps well within
 * the allowed 2500 ppm.
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <sysclk.h>
#include <tc.h>
#include <dfll.h>

#include "conf_system.h"
#include "sync.h"


static bool open_loop;          // is DFLL in open loop mode
static bool in_sync = false;
static int next_sync_frame = 1; // number of frames until next sync

#if SYNC_DIAGNOSTICS
// Since we can't print from the SOF interrupt handler without disturbing
// frequency measurements, here is a circular buffer for recording events
// for later printing.
// Buffer size must be a power of two.
#define LOGBUFSIZE  32
static struct {
    uint16_t tuning_before;
    uint16_t interval;
    uint16_t tuning_after;
} synclog[LOGBUFSIZE];
static int log_head, log_tail;
#endif


void sync_init(void)
{
    // Run timer-counter at 1/8 of system clock, which is the same as
    // USB clock.
    // One USB frame takes 1 ms, or 6000 ticks of the timer at the target
    // system clock frequency of 48 MHz.

    sysclk_enable_peripheral_clock(TC_SYNC);

    tc_init(TC_SYNC, TC_SYNC_CHANNEL,
#if CONFIG_SYSCLK_PBA_DIV == 0
            TC_CMR_TCCLKS_TIMER_CLOCK3 |    // PBA clock / 8
#elif CONFIG_SYSCLK_PBA_DIV == 2
            TC_CMR_TCCLKS_TIMER_CLOCK2 |    // PBA clock / 2
#else
#error Unsupported PBA frequency.
#endif
            TC_CMR_WAVE |                   // waveform mode is enabled
            TC_CMR_WAVSEL_UP_NO_AUTO);      // UP mode, no trigger

    tc_start(TC_SYNC, TC_SYNC_CHANNEL);
}

// To be called from SOF interrupt handler.
void sync_frame(void)
{
    static uint32_t tuning;
    static uint16_t prev_timestamp;
    irqflags_t flags;

    if (--next_sync_frame > 0)
        return;

    uint16_t curr = tc_read_cv(TC_SYNC, TC_SYNC_CHANNEL);

    if (!open_loop) {
        // synchronise DFLL conf registers before reading
        SCIF->SCIF_DFLL0SYNC = SCIF_DFLL0SYNC_SYNC;
        while (!(SCIF->SCIF_PCLKSR & SCIF_PCLKSR_DFLL0RDY));

        uint32_t conf = SCIF->SCIF_DFLL0CONF;
        tuning = SCIF->SCIF_DFLL0VAL;

        // switch DFLL to open loop
        flags = cpu_irq_save();
        SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
            | SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_DFLL0CONF - (uint32_t)SCIF);
        SCIF->SCIF_DFLL0CONF = conf & ~SCIF_DFLL0CONF_MODE;
        cpu_irq_restore(flags);

        //sysclk_set_source(SYSCLK_SRC_DFLL);

        open_loop = true;
    }

    if (next_sync_frame == 0) {
        prev_timestamp = curr;
        return;
    }

    curr -= prev_timestamp;

#if SYNC_DIAGNOSTICS
    synclog[log_head].tuning_before = tuning | tuning >> 8;
    synclog[log_head].tuning_after = tuning | tuning >> 8;
    synclog[log_head].interval = curr;
    log_head = (log_head + 1) & (LOGBUFSIZE - 1);
#endif

    if (curr > 7000 || curr < 5000) {
        // skipped a frame between consecutive measurements, or some other
        // weirdness; try again
        next_sync_frame = 1;
        return;
    }

    int deviation = ((int)curr - 6000) >> 3;

    if (deviation < 0)
        deviation++;

    if (deviation == 0) {
        // we are in sync with USB, next check in one second
        in_sync = true;
        next_sync_frame = 1000;
        return;
    }

    // if we are in sync, use small steps
    if (in_sync)
        deviation = (deviation | INT_MAX) >> (sizeof deviation * 8 - 2);

    if ((tuning - deviation) & 0xFF00) {
        // coarse tuning adjustment
        in_sync = false;
        tuning = (tuning & SCIF_DFLL0VAL_COARSE_Msk) | 127;
        if (deviation > 0)
            tuning -= (1 << SCIF_DFLL0VAL_COARSE_Pos) + 90;
        else
            tuning += (1 << SCIF_DFLL0VAL_COARSE_Pos) + 90;
    } else
        tuning -= deviation;

#if SYNC_DIAGNOSTICS
    synclog[(log_head - 1) & (LOGBUFSIZE - 1)].tuning_after = tuning | tuning >> 8;
#endif

    // write new tuning value to DFLL
    while (!(SCIF->SCIF_PCLKSR & SCIF_PCLKSR_DFLL0RDY));
    flags = cpu_irq_save();
    SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
        | SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_DFLL0VAL - (uint32_t)SCIF);
    SCIF->SCIF_DFLL0VAL = tuning;
    cpu_irq_restore(flags);

    // check again after 1 frame
    next_sync_frame = 1;
}

// To be called on USB suspend.
void sync_suspend(void)
{
#if 1
    // Make sure we are not caught up with next_sync_frame == 0 that would
    // result in a wrong timing measurement upon exit from suspend.
    // Also, setting it to 2 will ensure a quick re-sync when back from suspend.
    next_sync_frame = 2;
    in_sync = false;
#else
    // switch to closed loop to track RC32K, because we won't be getting
    // frames from USB for a while;
    // not really necessary, as DFLL in open loop is quite stable
    //sysclk_set_source(SYSCLK_SRC_RCFAST);

    // synchronise DFLL conf registers before reading
    SCIF->SCIF_DFLL0SYNC = SCIF_DFLL0SYNC_SYNC;
    while (!(SCIF->SCIF_PCLKSR & SCIF_PCLKSR_DFLL0RDY));

    uint32_t conf = SCIF->SCIF_DFLL0CONF;

    // switch DFLL to closed loop
    irqflags_t flags = cpu_irq_save();
    SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xAAUL)
        | SCIF_UNLOCK_ADDR((uint32_t)&SCIF->SCIF_DFLL0CONF - (uint32_t)SCIF);
    SCIF->SCIF_DFLL0CONF = conf | SCIF_DFLL0CONF_MODE;
    open_loop = false;
    next_sync_frame = 1;
    cpu_irq_restore(flags);
#endif
}

bool sync_ok(void)
{
    return in_sync;
}

#if SYNC_DIAGNOSTICS
void sync_diag_print(void)
{
    if (log_head == log_tail)
        return;

    putchar('\n');

    while (log_head != log_tail) {
        printf("interval %5u: tuning %x -> %x\n", synclog[log_tail].interval,
                synclog[log_tail].tuning_before,
                synclog[log_tail].tuning_after);
        log_tail = (log_tail + 1) & (LOGBUFSIZE - 1);
    }
}
#endif
