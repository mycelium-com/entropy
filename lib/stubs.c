/*
 * Replacement for newlib's abort() and unused functions.
 *
 * This helps avoid linking with impure data and memory allocation routines.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void abort(void)
{
    puts("Aborted.");
    for (;;);
}

void __register_exitproc(void);
void __register_exitproc(void) {}
void exit(int code) { _exit(code); }
