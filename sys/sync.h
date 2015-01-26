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

#ifndef SYNC_H_INCLUDED
#define SYNC_H_INCLUDED

// Enable diagnostic output?
#ifndef SYNC_DIAGNOSTICS
#define SYNC_DIAGNOSTICS 0
#endif

void sync_init(void);
void sync_frame(void);
void sync_suspend(void);
bool sync_ok(void);

#if SYNC_DIAGNOSTICS
void sync_diag_print(void);
#else
#define sync_diag_print()
#endif

#endif
