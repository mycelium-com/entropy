/*
 * Replacement for newlib's __assert_func().
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void __assert_func(const char *file, int line, const char *func, const char *e)
{
    printf("%s:%d(%s): Assertion `%s' failed.\n", file, line, func, e);
    abort();
}
