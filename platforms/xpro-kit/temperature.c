/**
 * \file
 *
 * \brief Driver for the temperature sensor AT30TSE75x on SAM4L Xplained Pro.
 * Based on Atmel Software Framework.
 *
 * Copyright (C) 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <board.h>
#include <sysclk.h>
#include <twim.h>

#include <stdio.h>
#include <led.h>

#include "temperature.h"


// Target's TWI addresses
#define TEMP_ADDRESS         0x4F
#define EEPROM_ADDRESS       0x57

// AT30TSE75x definitions
#define AT30TSE_TEMPERATURE_REG         0x00
#define AT30TSE_TEMPERATURE_REG_SIZE    2
#define AT30TSE_NON_VOLATILE_REG        0x00
#define AT30TSE_VOLATILE_REG            0x10

static void twim_debug_callback(Twim *twim)
{
    LED_Toggle(LED0);
    twim_default_callback(twim);
}

// Initialise TWIM module for AT30TSE75x.
// Return values:
// - STATUS_OK        Success
// - ERR_INVALID_ARG  Invalid arg resulting in wrong CWGR Exponential
status_code_t temperature_init(void)
{
    /* Set TWIM options */
    static struct twim_config opts = {
        .speed = TEMP_TWIM_SPEED,
        .hsmode_speed = 0,
        .data_setup_cycles = 0,
        .hsmode_data_setup_cycles = 0,
        .smbus = false,
        .clock_slew_limit = 0,
        .clock_drive_strength_low = 0,
        .data_slew_limit = 0,
        .data_drive_strength_low = 0,
        .hs_clock_slew_limit = 0,
        .hs_clock_drive_strength_high = 0,
        .hs_clock_drive_strength_low = 0,
        .hs_data_slew_limit = 0,
        .hs_data_drive_strength_low = 0,
    };
    opts.twim_clk = sysclk_get_peripheral_bus_hz(TEMP_TWIM);

    /* Initialize the TWIM Module */
    twim_set_callback(TEMP_TWIM, 0, twim_debug_callback, 1);

    return twim_set_config(TEMP_TWIM, &opts);
}

/**
 * \brief Read register in AT30TSE75x
 *
 * \param reg       Register in AT30TSE75x to be read, including its type,
 *                  nonvolatile or not
 * \param reg_size  Read length
 * \param buffer    Pointer to the buffer where the read data will be stored
 *
 * \return TWI_SUCCESS (aka STATUS_OK) if success
 */
static status_code_t at30tse_read_register(uint8_t reg, uint8_t reg_size,
        uint8_t *buffer)
{
    static struct twim_package packet = {
        /* Chip addr */
        .chip = TEMP_ADDRESS,
        .addr_length = 1,
    };
    /* Internal chip addr */
    packet.addr[0] = reg;
    /* Data buffer */
    packet.buffer = buffer;
    packet.length = reg_size;

    return twi_master_read(TEMP_TWIM, &packet);
}

/**
 * \brief Read temperature
 *
 * \param temperature  Pointer to the buffer where the temperature will be
 * stored in half-degrees Celsius
 *
 * \return TWI_SUCCESS (aka STATUS_OK) if success
 */
status_code_t temperature_read(int *temperature)
{
    /* Placeholder buffer to put temperature data in. */
    uint8_t buffer[2];
    status_code_t status;
    buffer[0] = 0;
    buffer[1] = 0;

    /* Read the 16-bit temperature register. */
    status = at30tse_read_register(
            AT30TSE_TEMPERATURE_REG | AT30TSE_NON_VOLATILE_REG,
            AT30TSE_TEMPERATURE_REG_SIZE,
            buffer);

    /* Only convert temperature data if read success. */
    if (status == STATUS_OK)
        *temperature = (int)(int8_t)buffer[0] << 1 | buffer[1] >> 7;

    return status;
}
