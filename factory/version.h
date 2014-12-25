/*
 * Version and other identification information for the signature block.
 *
 * This file is run through the C preprocessor by the signing tool.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#define MAGIC           FW_MAGIC_ENTROPY
#define VERSION         "FT.2"
#define VERSION_CODE    2
#define FLAVOUR_CODE    FW_FLAVOUR_REGULAR

// Hardware requirements:
// no serial flash required.
#define XFLASH          HW_XSFL_NONE
#define XFLASH_SIZE     0
