/*
 * Version and other identification information for the signature block.
 *
 * This file is run through the C preprocessor by the signing tool.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

/*
 * BRT is essentially a container for boot.bin and therefore uses the same
 * version information.
 */

#include "../boot/version.h"

#undef MAGIC
#define MAGIC           FW_MAGIC_TASK
