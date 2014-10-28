// Replacement for newlib's stdio.h.

#ifndef LIB_STDIO_H
#define LIB_STDIO_H

#include <stddef.h>
#include <stdarg.h>

int putchar(int c);
int puts(const char *str);
int printf(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
int sprintf(char *buf, const char *fmt, ...)
            __attribute__((format (printf, 2, 3)));
int snprintf(char *buf, size_t size, const char *fmt, ...)
            __attribute__((format (printf, 3, 4)));

#endif
