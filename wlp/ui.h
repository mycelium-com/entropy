/*
 * User interface.
 * Button and LED handling.
 *
 * Copyright 2015 Mycelium SA, Luxembourg.
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

// Error codes.
enum Ui_message_code {
    UI_INIT = 2,
    UI_ERASE,
    UI_WRITE,
    UI_VERIFY,
};

void ui_init(void);
void ui_error(int no) __attribute__ ((noreturn));

#endif
