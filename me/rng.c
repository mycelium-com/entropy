/*
 * Entropy collection and extraction, and random number generation.
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
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <ioport.h>
#include <adcife.h>
#include <pdca.h>
#include <usart.h>

#include "conf_uart_serial.h"
#include "sys/conf_system.h"
#include "lib/sha256.h"
#include "lib/sha512.h"
#include "rng.h"

#ifndef RNG_NO_FLASH
#include "xflash.h"
#endif


#define RNG_DIAGNOSTICS 0

// How much raw material to collect.
enum {
    TRNG_WORDS_FIRST    = 13,
    TRNG_WORDS_NEXT     = 6,
    ADC_BYTES_FIRST     = 400,
    ADC_BYTES_NEXT      = 200,
};

// Uninitialised memory is between _estack and __ram_end__ inclusively.
// These symbols are defined by the linker.
// For the initial entropy collection on power up, we stage a temporary entropy
// pool at the beginning of this memory:
//  1. hash the raw SRAM data under the pool;
//  2. fill the pool with the above hash + entropy from other (non-SRAM) sources;
//  3. hash the pool with the remaining SRAM data.
extern union {
    struct {    // temporary entropy pool
        uint64_t hash[8];                   // for SHA-512
        uint32_t trng[TRNG_WORDS_FIRST];    // from hardware TRNG
        uint8_t  adc[ADC_BYTES_FIRST];      // noise from floating input to ADC
    } tmp;
    uint8_t  raw_bytes[0];
    uint32_t raw_words[0];
} _estack;
extern uint32_t __ram_end__;
// Amount of uninitialised memory in SRAM pool in bytes.
// Will be computed at the beginning of rnd_init().
static unsigned sram_pool_size;

// RNG state.
// The SHA-512 hash obtained by extracting entropy from the initial raw entropy
// sources is split into two halves.
// One half is used as the key to the HMAC-based PRNG;
// the other half acts as the seed, which is concatenated with the counter
// before being fed to HMAC.
// hmac_key is regularly mixed with new entropy from TRNG and ADC.
// This is probably over-engineered; however, a lot of PRNGs are based on HMAC
// functions, including the one in BIP32.
static struct {
    union {
        uint64_t hash[8];
        struct {
            uint32_t hmac_key[8];
            uint32_t hmac_seed[8];
        };
    };
    uint32_t counter;
} state;

static uint32_t rng_health;
enum {
    GOOD_HEALTH = 0x44fdc808        // randomly selected magic number
};

// End-of-transfer flag from the DMA controller's ADC RX channel.
static volatile bool adc_done;

#if RNG_DIAGNOSTICS
static void diag_dump(const char *name, const void *data,
        int total_len, int sample_len)
{
    int i;
    if (sample_len > total_len)
        sample_len = total_len;
    printf("%s (%d bytes%s):\n", name, total_len,
            total_len == sample_len ? "" : " total");
    for (i = 0; ; i++) {
        printf(" %02x", ((uint8_t *)data)[i]);
        if (i == sample_len - 1)
            break;
        if ((i & 15) == 15)
            putchar('\n');
    }
    puts(total_len == sample_len ? "" : " ...");
}
#else
#define diag_dump(n,d,t,s)
#endif

static bool check_sram_entropy(void);
static bool check_sram_difference(void);
static bool start_adc(void);
static void restart_adc(uint8_t *buf, unsigned length);
static void mini_hmac_sha_256(uint32_t hash[8],
                 const uint32_t *key, int key_len,
                 const uint32_t *data, int data_len);

// PBA bus clock control.
// TRNG and ADC modules sit on the PBA bus.
// Its clock should be divided by 4 to run at 12 MHz for proper operation.
// However, all USART peripherals, which we use for console output and SPI,
// are also on this bus and want it to run at full speed of 48 MHz.
// The clock is divided by 2**shift.
static void bus_clock(unsigned shift)
{
    if (shift)
        do ; while (!usart_is_tx_empty(CONF_UART));
    sysclk_set_prescalers(0, shift, 0, 0, 0);
}

bool rng_init(void)
{
    sram_pool_size = (uint8_t *)&__ram_end__ - _estack.raw_bytes + 4;

    printf("Entropy source in SRAM: %u bytes.\n", sram_pool_size);

    // Health checks.
    rng_health = 0;
    if (!check_sram_entropy() || !check_sram_difference())
        return false;

    // Divide PBA bus clock by 4 for TRNG and ADC.
    bus_clock(2);

    // Hash the raw SRAM data in the temporary entropy pool.
    sha512_hash(state.hash, _estack.raw_bytes, sizeof _estack.tmp);

    // Enable True Random Number Generator module.
    sysclk_enable_peripheral_clock(TRNG);
    TRNG->TRNG_CR = TRNG_CR_ENABLE | TRNG_CR_KEY(0x524E47);     // 'RNG'

    // Begin filling the temporary pool with ADC data.
    // This is handled automatically by the Peripheral DMA;
    // the adc_done flag will be set when finished.
    if (!start_adc())
        return false;

    // Meanwhile, fill the temporary pool with the hash obtained above, and
    // with the data from hardware TRNG.
    memcpy(_estack.tmp.hash, state.hash, sizeof _estack.tmp.hash);
    int i = TRNG_WORDS_FIRST;
    do {
        if (TRNG->TRNG_ISR & TRNG_ISR_DATRDY)
            _estack.tmp.trng[--i] = TRNG->TRNG_ODATA;
    } while (i);
    TRNG->TRNG_CR = TRNG_CR_KEY(0x524E47);      // disable TRNG

    do ; while (!adc_done);

    // Finished with hardware entropy acquisition, return bus to normal speed.
    bus_clock(0);

    diag_dump("TRNG output", _estack.tmp.trng, sizeof _estack.tmp.trng,
            sizeof _estack.tmp.trng);
    diag_dump("ADC data", _estack.tmp.adc, sizeof _estack.tmp.adc, 32);

    // Now, hash the temporary pool with the remaining SRAM data.
    // Store the result into the generator's state.
    sha512_hash(state.hash, _estack.raw_bytes, sram_pool_size);

    // Clear last word so that we'll notice upon restart if the SRAM hasn't been
    // powered off properly.
    __ram_end__ = 0;

    rng_health = GOOD_HEALTH;

    return true;
}

void rng_next(uint32_t random_number[8])
{
    // Prevent number generation if the generator has failed a health test,
    // as recommended in NIST Special Publication 800-90A.
    if (rng_health != GOOD_HEALTH)
        for (;;);

    // If this is the first call since rng_init(), we have a ready to use
    // seed in state.hash with plenty of entropy and can make a random number
    // right away.
    // On subsequent calls, let's add more entropy to the mix because we can.

    if (state.counter != 0) {
        struct {
            uint32_t hmac_key[8];
            uint32_t trng[TRNG_WORDS_NEXT];
            uint8_t  adc[ADC_BYTES_NEXT];
        } mixer;

        bus_clock(2);
        TRNG->TRNG_CR = TRNG_CR_ENABLE | TRNG_CR_KEY(0x524E47);     // 'RNG'
        restart_adc(mixer.adc, ADC_BYTES_NEXT);

        memcpy(mixer.hmac_key, state.hmac_key, sizeof mixer.hmac_key);

        int i = TRNG_WORDS_NEXT;
        do {
            if (TRNG->TRNG_ISR & TRNG_ISR_DATRDY)
                mixer.trng[--i] = TRNG->TRNG_ODATA;
        } while (i);
        TRNG->TRNG_CR = TRNG_CR_KEY(0x524E47);      // disable TRNG

        do ; while (!adc_done);
        bus_clock(0);

        diag_dump("TRNG output", mixer.trng, sizeof mixer.trng, sizeof mixer.trng);
        diag_dump("ADC data", mixer.adc, sizeof mixer.adc, 32);

        // Mix new entropy into the main state.
        sha256_hash(state.hmac_key, (const uint8_t *)&mixer, sizeof mixer);
    }

    state.counter++;

    // Use HMAC/SHA-256 as the PRNG function.
    mini_hmac_sha_256(random_number, state.hmac_key, 8, state.hmac_seed, 9);

    diag_dump("HMAC key", state.hmac_key, sizeof state.hmac_key, sizeof state.hmac_key);
    diag_dump("HMAC seed and counter", state.hmac_seed, 36, 36);
    diag_dump("Private key", random_number, 32, 32);
}

// Minimalistic HMAC/SHA-256 implementation specifically for our PRNG.
static void mini_hmac_sha_256(uint32_t hash[8],
                 const uint32_t *key, int key_len,
                 const uint32_t *data, int data_len)
{
    struct {
        uint32_t key[16];
        uint32_t data[9];
    } step1;
    struct {
        uint32_t key[16];
        uint32_t hash[8];
    } step2;
    int i;

    assert((unsigned) key_len <= sizeof step1.key / sizeof (step1.key[0]));
    assert((unsigned) data_len <= sizeof step1.data / sizeof (step1.data[0]));

    for (i = 0; i < 16; i++) {
        uint32_t k = i < key_len ? key[i] : 0;
        step1.key[i] = k ^ 0x36363636;
        step2.key[i] = k ^ 0x5C5C5C5C;
    }
    memcpy(step1.data, data, data_len * 4);
    sha256_hash(step2.hash, (uint8_t *) &step1, sizeof step1.key + data_len * 4);
    sha256_hash(hash, (uint8_t *) &step2, sizeof step2);
}

static unsigned count_ones(uint32_t x)
{
    unsigned num_ones;
    for (num_ones = 0; x; num_ones++)
        x &= x - 1;
    return num_ones;
}

static bool check_sram_entropy(void)
{
    // Check that the memory was powered off long enough to become random.
    if (__ram_end__ == 0) {
        puts("Memory is not random.");
        return false;
    }

    diag_dump("SRAM state", _estack.raw_bytes, sram_pool_size, 32);

    // Check that it's not almost all ones or almost all zeros.
    int i = 0;
    unsigned ones = 0;
    do {
        ones += count_ones(_estack.raw_words[i++]);
        if (ones >= 8192 && i * 32 - ones >= 8192)
            return true;
    } while (i <= &__ram_end__ - _estack.raw_words);

    printf("Low SRAM entropy: %u ones, %u zeros.\n", ones, i * 32 - ones);
    return false;
}

#ifndef RNG_NO_FLASH
static bool check_sram_difference(void)
{
    uint32_t buf[128];
    unsigned distance = 0;
    unsigned flash_addr = xflash_num_blocks * 512 - XFLASH_RAM_SNAPSHOT_OFFSET;
    unsigned addr = flash_addr;
    unsigned i;
    uint32_t *ram = &__ram_end__ + 1 - XFLASH_RAM_SNAPSHOT_SIZE / sizeof (uint32_t);
    const uint32_t *ramptr = ram;

    // Check that a small RAM area (1 kB) is sufficiently different
    // from its previous power-on state.
    do {
        if (addr == flash_addr + XFLASH_RAM_SNAPSHOT_SIZE) {
            printf("SRAM is stuck: distance %u bits on %u byte area.\n",
                    distance, XFLASH_RAM_SNAPSHOT_SIZE);
            return false;
        }

        xflash_read((uint8_t *)buf, sizeof buf, addr);
        addr += sizeof buf;

        for (i = 0; i < sizeof buf / sizeof (buf[0]); i++)
            distance += count_ones(*ramptr++ ^ buf[i]);
    } while (distance < XFLASH_RAM_SNAPSHOT_SIZE / 32);

#if RNG_DIAGNOSTICS
    printf("SRAM distance is %u bits on %u bytes.\n",
            distance, addr - flash_addr);
#endif

    // Write current values into flash.
    xflash_erase_4k(flash_addr);
    xflash_write((uint8_t *)ram, XFLASH_RAM_SNAPSHOT_SIZE, flash_addr);

    return true;
}
#else
static bool check_sram_difference(void)
{
    return true;
}
#endif

static struct adc_dev_inst adc;

static void adc_done_handler(enum pdca_channel_status status)
{
#ifdef ASF_BUG_3257_FIXED
    adc_disable(&adc);
#else
    // Disable ADC manually.
    // Cannot use adc_disable() due to bug 3257:
    // http://asf.atmel.com/bugzilla/show_bug.cgi?id=3257
    // This is still not fixed as of ASF 3.18.0.
    ADCIFE->ADCIFE_CR = ADCIFE_CR_DIS | ADCIFE_CR_REFBUFDIS |
            ADCIFE_CR_BGREQDIS;
#endif
    pdca_channel_disable_interrupt(CONFIG_ADC_PDCA_RX_CHANNEL, PDCA_IER_TRC);
    adc_done = true;

#if RNG_DIAGNOSTICS
    if (status != PDCA_CH_TRANSFER_COMPLETED)
        printf("Unexpected PDCA status %#x.\n", status);
#endif
}

static bool start_adc(void)
{
    static const struct adc_config cfg = {
        .prescal    = ADC_PRESCAL_DIV16,    // assuming PBA is clocked at 12 MHz
        .clksel     = ADC_CLKSEL_APBCLK,
        .speed      = ADC_SPEED_300K,       // affects current consumption
        .refsel     = ADC_REFSEL_1,         // 0.625*Vcc
        .start_up   = CONFIG_ADC_STARTUP,   // cycles of the ADC clock
    };
    static const struct adc_seq_config seq_cfg = {
        .muxneg     = ADC_MUXNEG_1,         // negative input from Pad Ground
        .muxpos     = ADC_MUXPOS_0,         // positive input from AD0
        .internal   = ADC_INTERNAL_2,       // internal neg, external pos
        .res        = ADC_RES_12_BIT,
        .bipolar    = ADC_BIPOLAR_SINGLEENDED,
        .gain       = ADC_GAIN_HALF,
        .trgsel     = ADC_TRIG_CON,         // continuous trigger mode
    };
    static const pdca_channel_config_t pdca_cfg = {
        .addr       = _estack.tmp.adc,      // destination address
        .pid        = ADCIFE_PDCA_ID_RX,    // peripheral ID
        .size       = sizeof _estack.tmp.adc,
        .transfer_size = PDCA_MR_SIZE_BYTE, // using LSbyte from each sample
    };

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
        bus_clock(0);
        puts("Cannot enable ADC.");
        return false;
    }

    ioport_set_pin_mode(PIN_PA04A_ADCIFE_AD0, MUX_PA04A_ADCIFE_AD0);
    ioport_disable_pin(PIN_PA04A_ADCIFE_AD0);

    adc_done = false;
    pdca_enable(PDCA);
    pdca_channel_set_config(CONFIG_ADC_PDCA_RX_CHANNEL, &pdca_cfg);
    pdca_channel_set_callback(CONFIG_ADC_PDCA_RX_CHANNEL, adc_done_handler,
            PDCA_0_IRQn, CONFIG_ADC_IRQ_PRIO, PDCA_IER_TRC);
    pdca_channel_enable(CONFIG_ADC_PDCA_RX_CHANNEL);

    ADCIFE->ADCIFE_SEQCFG = *(const uint32_t *)&seq_cfg;

    return true;
}

static void restart_adc(uint8_t *buf, unsigned length)
{
    if (adc_enable(&adc) != STATUS_OK) {
        // Failure to restart ADC is not fatal, because we already have a
        // full-entropy seed.
        // Furthermore, it's probably impossible, since the ADC already started
        // successfully in rng_init().
        bus_clock(0);
        puts("Cannot enable ADC.");
        bus_clock(2);
        return;
    }
    adc_done = false;
    pdca_channel_write_load(CONFIG_ADC_PDCA_RX_CHANNEL, buf, length);
    pdca_channel_enable_interrupt(CONFIG_ADC_PDCA_RX_CHANNEL, PDCA_IER_TRC);
}
