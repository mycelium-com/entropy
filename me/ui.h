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

#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

// Error and warning codes.
enum Ui_message_code {
    UI_E_BAD_CONFIG = 2,            // C: -.-.
    UI_E_NO_ENTROPY,                // E: .
    UI_W_FOREIGN_KEY,               // F: ..-.
    UI_E_INVALID_KEY,               // K: -.-
    UI_E_INVALID_SIGNATURE,         // S: ...
    UI_E_INVALID_IMAGE,
    UI_E_UNSUPPORTED_HARDWARE,      // U: ..-
    UI_E_HARDWARE_FAULT,            // H: ....
};

extern volatile int ui_btn_count;
extern volatile uint32_t ui_btn_timestamp;

// Initialise the user interface.
void ui_init(void);

// Enter the USB suspend mode.
void ui_suspend(void);

// Switch off LEDs.
void ui_off(void);

// Turn off button interrupts.
void ui_btn_off(void);

// Wake up from the USB suspend mode.
void ui_wakeup(void);

// Callbacks to show the MSC read and write access
void ui_start_read(void);
void ui_stop_read(void);
void ui_start_write(void);
void ui_stop_write(void);

// This is called for each USB frame, which happen every 1ms when the interface
// is enabled and not in suspended mode.
void ui_process(uint16_t framenumber);

// Key generation mode: rapid blinking.
void ui_keygen(void);
// Slow pulsating/glow effect.
void ui_pulsate(void);
// Error indication by repeating a specified number of blinks.
void ui_error(enum Ui_message_code code) __attribute__ ((noreturn));
// Show warning and wait for confirmation.
void ui_confirm(enum Ui_message_code code);

#endif
