// Automatically generated JPEG data file.
// Size: 1808x2432 pixels / 226x304 macroblocks.
// Header font: Optima Regular, 44 pt.
// Data font:   Menlo Regular, 40 pt.

#include <stdint.h>

// Parameters (used in #ifdef hence not an enum):
#define JWIDTH      226
#define JHEIGHT     304

// JPEG bitstream elements:
#define SME {  3,    0x0 }
#define WTB { 16, 0xc01c }
#define BTW { 16, 0xdfe0 }
#define AC_EOB_LEN       2
#define AC_EOB_CODE    0x0
enum {
    SME_LEN             = 3,
    SME_BITS            = 0x0,
    SME_LONG_MB         = 8,
    SME_LONG_LEN        = 24,
    SME_LONG_BITS       = 0x0,
    GTW_LEN             = 15,
    GTW_BITS            = 0x5fe0,
};

// Huffman DC table 0
struct Huffman_table {
    uint16_t len;
    uint16_t code;
};
extern const struct Huffman_table huff_dc[12];

// Font data encoded as JPEG bitstream

enum {
    CHR_WIDTH   = 3,
    CHR_HEIGHT  = 5,
};

struct Chr_row_descr {
    int16_t dc_in;
    int16_t dc_out;
    uint8_t nbytes;
    uint8_t nbits;
    uint16_t offset;
};

extern const struct Chr_row_descr addr_font[61][CHR_HEIGHT];
extern const unsigned char font_map[91];
extern const unsigned char addr_font_data[34949];

// Pictures.
extern const uint16_t cut_here_fragment[];
enum {
    CUT_HERE_FRAGMENT_HEIGHT = 5
};
extern const uint16_t bitcoin_address_fragment[];
enum {
    BITCOIN_ADDRESS_FRAGMENT_WIDTH  = 38,
    BITCOIN_ADDRESS_FRAGMENT_HEIGHT = 5
};
extern const uint16_t private_key_fragment[];
enum {
    PRIVATE_KEY_FRAGMENT_WIDTH  = 28,
    PRIVATE_KEY_FRAGMENT_HEIGHT = 7
};
extern const uint16_t share_1_of_3_fragment[];
enum {
    SHARE_1_OF_3_FRAGMENT_WIDTH  = 30,
    SHARE_1_OF_3_FRAGMENT_HEIGHT = 5
};
extern const uint16_t share_2_of_3_fragment[];
enum {
    SHARE_2_OF_3_FRAGMENT_WIDTH  = 30,
    SHARE_2_OF_3_FRAGMENT_HEIGHT = 5
};
extern const uint16_t share_3_of_3_fragment[];
enum {
    SHARE_3_OF_3_FRAGMENT_WIDTH  = 30,
    SHARE_3_OF_3_FRAGMENT_HEIGHT = 5
};
extern const uint16_t any_two_shares_reveal_fragment[];
enum {
    ANY_TWO_SHARES_REVEAL_FRAGMENT_WIDTH  = 74,
    ANY_TWO_SHARES_REVEAL_FRAGMENT_HEIGHT = 7
};
extern const uint16_t litecoin_address_fragment[];
enum {
    LITECOIN_ADDRESS_FRAGMENT_WIDTH  = 41,
    LITECOIN_ADDRESS_FRAGMENT_HEIGHT = 5
};
extern const uint16_t entropy_for_verification_fragment[];
enum {
    ENTROPY_FOR_VERIFICATION_FRAGMENT_WIDTH  = 57,
    ENTROPY_FOR_VERIFICATION_FRAGMENT_HEIGHT = 7
};
extern const uint16_t key_sha_256_salt1_fragment[];
enum {
    KEY_SHA_256_SALT1_FRAGMENT_WIDTH  = 77,
    KEY_SHA_256_SALT1_FRAGMENT_HEIGHT = 7
};
extern const uint16_t small_print_fragment[];
enum {
    SMALL_PRINT_FRAGMENT_WIDTH  = 84,
    SMALL_PRINT_FRAGMENT_HEIGHT = 5
};
extern const uint16_t logo_top_fragment[];
enum {
    LOGO_TOP_FRAGMENT_WIDTH  = 42,
    LOGO_TOP_FRAGMENT_HEIGHT = 45
};
extern const uint16_t logo_bottom_fragment[];
enum {
    LOGO_BOTTOM_FRAGMENT_WIDTH  = 66,
    LOGO_BOTTOM_FRAGMENT_HEIGHT = 13
};

// JPEG header.
extern const unsigned char jpeg_header[422];
