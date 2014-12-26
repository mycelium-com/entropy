/*
 * Version and other identification information for the signature block.
 *
 * This file is run through the C preprocessor by the signing tool.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

#define MAGIC           FW_MAGIC_ENTROPY
#define VERSION         "1.0"
#define VERSION_CODE    5
#define FLAVOUR_CODE    FW_FLAVOUR_MYCELIUM

// Hardware requirements:
// serial flash with small sector erase capability,
// minimum size 512 kB.
#define XFLASH          HW_XSFL_SSECT
#define XFLASH_SIZE     HW_XSFL_512K
