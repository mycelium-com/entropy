// External serial flash support.

#ifndef XFLASH_H_INCLUDED
#define XFLASH_H_INCLUDED

#include <stdbool.h>

void xflash_init(void);
bool xflash_check(char **ptr, char *buf, int buflen);

#endif
