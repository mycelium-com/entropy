/*
 * Streaming JPEG generator.
 *
 * Copyright 2013-2015 Mycelium SA, Luxembourg.
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <ff.h>

#include "qr.h"
#include "xflash.h"
#include "jpeg.h"
#include "data.h"
#include "jpeg-data.h"

#define DEBUG           0
#define USE_EXT_FLASH   0

// Image is built out of fragments, which are rectangular arrays of macroblocks.
// A fragment is described by its type, parameters, and body.
// During output, active fragments have their state, describing how to
// output the next row of macroblocks for this fragment.

// Fragment type: Large Picture.
// Occupies full rows.  Has no state, because it is output immediately.
// Body consists of uint16_t fields:
//  height              - in macroblocks
//  nwords              - number of words to follow
//  nbits               - number of leftover bits to follow
//  word                - 
//   ..                  | raw JPEG (before byte stuffing)
//  word                 |
//  bits                -

// Fragment type: (Small) Picture.
// This is a rectangular picture fragment.
// Body consists of uint16_t fields:
//  { width, height }   - in macroblocks
//  { nwords: 12 msb, nbits: 4 lsb } -
//  words                             | raw JPEG, repeated height times
//  bits                             -
struct pic_state {
    uint16_t rows_left;     // number of macroblock rows remaining
    uint16_t width;         // width in macroblocks
    const uint16_t *ptr;    // pointer into the body at next { nbits, nwords }
    uint32_t addr;          // next row's address in serial flash
};

// Rectangle type: QR Code.
// QR codes have no static bodies.  They are generated dynamically directly
// into their state.
struct qr_state {
    uint8_t  size;          // QR code size in modules (29 or 33)
    uint8_t  idx;           // index of the text to render (address, key, etc.)
    uint16_t row;           // index of the next row of modules/macroblocks
    qr_row_t qr[QR_SIZE(QR_MAX_VERSION)];   // bit map
};

// Rectangle type: Text.
// Texts have no static bodies (aside from the font data, which is shared
// between all texts).
struct text_state {
    uint16_t    idx;        // index of the text to render
    uint8_t     width;      // width in characters
    uint8_t     row;        // index of the next macroblock row for this line
    const char *text;       // text
};

union fgm_state {
    struct pic_state  pic;
    struct qr_state   qr;
    struct text_state text;
};

// Common fragment control structure
struct fgm_ctl {
    struct fgm_ctl *next;   // chain of active fragments for the current row
    unsigned x;             // start of this fragment in macroblocks
    bool (*render)(union fgm_state *state, int total_width);
    union fgm_state state[0];
};

// Checkpoint (not implemented yet)
struct checkpoint {
    uint16_t y;     // macroblock row
    // TODO
};

// Pool of fragment states
static struct {
    struct fgm_ctl ctl;
    struct pic_state state;
} pic_fragments[3];

static struct {
    struct fgm_ctl ctl;
    struct qr_state state;
} qr_fragments[2];

static struct {
    struct fgm_ctl ctl;
    struct text_state state;
} text_fragments[2];

// Allocate fragment state
static struct fgm_ctl * alloc(void *pool, size_t element_size, size_t pool_size)
{
    void *end = (uint8_t*) pool + pool_size;
    ((void) end);

    while (((struct fgm_ctl *)pool)->next) {
        pool = (uint8_t*) pool + element_size;
        assert(pool < end);
    }

    return pool;
}

#define ALLOC(pool) alloc(pool, sizeof (pool[0]), sizeof pool)

static inline void dealloc(struct fgm_ctl *fgm)
{
    fgm->next = 0;
}

// JPEG file markers
enum {
    SOF0_BASELINE_DCT   = 0xC0, // start of frame: baseline DCT
    DHT                 = 0xC4, // define Huffman table(s)
    SOI                 = 0xD8, // start of image
    EOI                 = 0xD9, // end of image
    SOS                 = 0xDA, // start of scan
    DQT                 = 0xDB, // define quantisation table(s)
    APP0_JFIF           = 0xE0, // application segment: JFIF
};

enum {
    UNITS_DPI   = 1,    // densities in dots per inch
    UNITS_DPCM  = 2,    // densities in dots per cm
};

// JPEG module encoding table for bit deltas <new,old>
// 0 means white, 1 means black
static const struct {
    uint16_t len;
    uint16_t bits;
} jpeg_encoding[] = {
    SME,    // 0->0: same
    BTW,    // 1->0: black to white
    WTB,    // 0->1: white to black
    SME,    // 1->1: same
};

static struct {
    uint8_t *buf;
    unsigned leftover_bits;
    unsigned num_leftover_bits;
} jpeg;


static void copy_bitstream(const uint16_t *src, uint16_t nwords,
                           uint16_t nbits, uint16_t bits, int gap)
{
    unsigned word = jpeg.leftover_bits;
    unsigned len  = jpeg.num_leftover_bits;
    uint8_t *p = jpeg.buf;

    for (; nwords; --nwords) {
        word = word << 16 | *src++;
        *p++ = word >> (len+8);
        if ((word >> (len+8) & 0xff) == 0xff)
            *p++ = 0;   // stuffing
        *p++ = word >> len;
        if ((word >> len & 0xff) == 0xff)
            *p++ = 0;   // stuffing
    }

    if (nbits) {
        word = word << nbits | bits;
        len += nbits;
        while (len >= 8) {
            len -= 8;
            *p++ = word >> len;
            if ((word >> len & 0xff) == 0xff)
                *p++ = 0;   // stuffing
        }
    }

    // add whitespace
    for (; gap >= SME_LONG_MB; gap -= SME_LONG_MB) {
        word = word << SME_LONG_LEN | SME_LONG_BITS;
        len += SME_LONG_LEN;
        while (len >= 8) {
            len -= 8;
            *p++ = word >> len;
        }
    }
    for (; gap; --gap) {
        word = word << SME_LEN | SME_BITS;
        len += SME_LEN;
        if (len >= 8) {
            len -= 8;
            *p++ = word >> len;
        }
    }

    jpeg.leftover_bits = word;
    jpeg.num_leftover_bits = len;
    jpeg.buf = p;
}

static inline void whitespace(int size_in_mb)
{
    copy_bitstream(0, 0, 0, 0, size_in_mb);
}

#if USE_EXT_FLASH
static FIL lay_file;
// copy bitstream from external serial flash
static unsigned copy_bitstream_from_flash(
        unsigned addr,      // address of bitstream in serial flash
        uint16_t nwords,    // length of bitstream in words
        uint16_t nbits,     // number of leftover bits
        uint16_t bits,      // contents of leftover bits
        int gap)            // gap in macroblocks
{
    uint16_t buf[256];
    unsigned len;

    do {
        len = nwords < sizeof buf / 2 ? nwords : sizeof buf / 2;

        if (len) {
            UINT bytes_read;
            FRESULT res1 = f_lseek(&lay_file, addr);
            FRESULT res2 = f_read(&lay_file, buf, len * 2, &bytes_read);
            if (res1 != FR_OK || res2 != FR_OK || bytes_read != len * 2) {
                printf("lseek and read: %d %d\n", res1, res2);
                global_error_flags |= FLASH_ERROR;
                return addr;
            }
            addr += len * 2;
            nwords -= len;
        }

        if (nwords)
            copy_bitstream(buf, len, 0, 0, 0);
    } while (nwords);

    copy_bitstream(buf, len, nbits, bits, gap);

    return addr;
}
#else
// copy bitstream from the built-in flash
extern const uint16_t jpeg_data_bin[];
static inline unsigned copy_bitstream_from_flash(
        unsigned addr,      // address of bitstream in serial flash
        uint16_t nwords,    // length of bitstream in words
        uint16_t nbits,     // number of leftover bits
        uint16_t bits,      // contents of leftover bits
        int gap)            // gap in macroblocks
{
    copy_bitstream(jpeg_data_bin + addr / 2, nwords, nbits, bits, gap);
    return addr + nwords * 2;
}
#endif

static bool render_pic(union fgm_state *state, int total_width)
{
    struct pic_state *st = &state->pic;
    // fragment state
    const uint16_t *src = st->ptr;  // pointer into the body
    unsigned i = *src++;            // { nwords: 12 msb, nbits: 4 lsb }

    st->addr = copy_bitstream_from_flash(st->addr, i >> 4, i & 0xf, *src,
            total_width - st->width);

    if (i & 0xf)
        src++;
    st->ptr = src;

    return --st->rows_left != 0;
}

static bool render_qr(union fgm_state *state, int total_width)
{
    struct qr_state *st = &state->qr;
    // stream buffer state
    unsigned word = jpeg.leftover_bits;
    unsigned len  = jpeg.num_leftover_bits;
    uint8_t *p = jpeg.buf;

    // load the next row (LSbit first), adding the preceding 0
    qr_row_t line = st->qr[st->row] << 1;   // previous 0 bit

    // convert this row, adding margins up to total_width
    do {
        unsigned nbits = jpeg_encoding[line & 3].len;
        word = word << nbits | jpeg_encoding[line & 3].bits;
        len += nbits;
        line >>= 1;
        while (len >= 8) {
            len -= 8;
            *p++ = word >> len;
            if ((word >> len & 0xff) == 0xff)
                *p++ = 0;   // stuffing
        }
    } while (--total_width);

    jpeg.leftover_bits = word;
    jpeg.num_leftover_bits = len;
    jpeg.buf = p;

    return ++st->row != st->size;
}

static inline int count_significant_bits(int x)
{
#ifdef __ARM_ARCH_7EM__
    // In arm7e-m __builtin_clz is implemented with the clz instruction,
    // which is defined at 0 as 32.
    return 32 - __builtin_clz((unsigned)(x ^ x >> 1));
#else
    // Generic implementation, which is usually done as a call to libgcc.
    return 31 - __builtin_clrsb(x);
#endif
}

static bool render_text(union fgm_state *state, int total_width)
{
    struct text_state *st = &state->text;
    // stream buffer state
    unsigned word = jpeg.leftover_bits;
    unsigned len  = jpeg.num_leftover_bits;
    uint8_t *p = jpeg.buf;

    int i;
    int last_dc = 0;

    for (i = 0; i < st->width && st->text[i]; i++) {
        if (st->text[i] == ' ') {
            // check for line break
            const char *next_space = index(st->text + i + 1, ' ');
            if (!next_space)
                next_space = index(st->text + i + 1, '\0');
            if (next_space > st->text + st->width)
                break;          // Me name's Break.  Line Break.
        }

        // character index in the font array
        unsigned chr_idx = font_map[st->text[i] - ' '];
        // this character's descriptor for this row
        struct Chr_row_descr chr = addr_font[chr_idx][st->row];

        // encode DC
        int dc = chr.dc_in - last_dc;
        if (dc < 0) --dc;   // encoding rule
        int cat = count_significant_bits(dc);
        word = word << huff_dc[cat].len | huff_dc[cat].code;
        word = word << cat | (dc & ((1 << cat) - 1));
        len += huff_dc[cat].len + cat;
        while (len >= 8) {
            len -= 8;
            *p++ = word >> len;
            if ((word >> len & 0xff) == 0xff)
                *p++ = 0;   // stuffing
        }
        last_dc = chr.dc_out;

        // copy rest of the character row
        const uint8_t *src = addr_font_data + chr.offset;
        while (chr.nbytes) {
            word = word << 8 | *src++;
            *p++ = word >> len;
            if ((word >> len & 0xff) == 0xff)
                *p++ = 0;   // stuffing
            --chr.nbytes;
        }
        word = word << chr.nbits | (*src & ((1 << chr.nbits) - 1));
        len += chr.nbits;
        if (len >= 8) {
            len -= 8;
            *p++ = word >> len;
            if ((word >> len & 0xff) == 0xff)
                *p++ = 0;   // stuffing
        }
    }

    if (++st->row == CHR_HEIGHT) {
        st->row = 0;
        st->text += i;
        if (*st->text == ' ')
            st->text++;     // skip space at line break
    }

    // return to white (add one white macroblock)
    int dc = -last_dc;
    if (dc < 0) --dc;   // encoding rule
    int cat = count_significant_bits(dc);
    word = word << huff_dc[cat].len | huff_dc[cat].code;
    word = word << cat | (dc & ((1 << cat) - 1));
    // Check how much we can squeeze into word before it overflows:
    //   max initial length:  7
    //   max dc code length:  12
    //   max dc value length: 11
    //   total: 30
    // We can safely add the AC EOB code if it is not longer than 2 bits.
#if AC_EOB_LEN > 2
#error This algorithm may cause word to overflow.
#endif
    word = word << AC_EOB_LEN | AC_EOB_CODE;
    len += huff_dc[cat].len + cat + AC_EOB_LEN;
    while (len >= 8) {
        len -= 8;
        *p++ = word >> len;
        if ((word >> len & 0xff) == 0xff)
            *p++ = 0;   // stuffing
    }

    // add whitespace
    for (i = i * CHR_WIDTH + 1; i < total_width; i++) {
        word = word << SME_LEN | SME_BITS;
        len += SME_LEN;
        if (len >= 8) {
            len -= 8;
            *p++ = word >> len;
        }
    }

    jpeg.leftover_bits = word;
    jpeg.num_leftover_bits = len;
    jpeg.buf = p;

    return *st->text;
}

static inline void finalise_jpeg(void)
{
    unsigned len = jpeg.num_leftover_bits;
    uint8_t *p = jpeg.buf;

    if (len) {
        // pad the last byte with ones
        len = 8 - len;
        unsigned word = ((jpeg.leftover_bits + 1) << len) - 1;
        *p++ = word;
    }

    // add footer
    *p++ = 0xFF;
    *p++ = EOI;

    jpeg.buf = p;
}

#define BLKSIZE 512

static unsigned dy;
static struct fgm_ctl end = { .next = 0, .x = JWIDTH };
static struct fgm_ctl *chain;
static const struct Layout *layout;

struct {
    unsigned minblk;
    uint8_t *minblk_ptr;
    uint8_t *tail;
    uint8_t *buf;
    uint8_t *end;
    unsigned endblk;
    const struct Layout *layout;
} stream;

#if DEBUG
static void jpeg_dump(void)
{
    printf("  minblk %u at %#x\n", stream.minblk, stream.minblk_ptr - stream.buf);
    printf("  tail at %#x\n", stream.tail - stream.buf);
    printf("  end at %#x\n", stream.end - stream.buf);
    printf("  jpeg.buf at %#x\n", jpeg.buf - stream.buf);
    printf("  endblk %u\n", stream.endblk);
}
#endif

static void jpeg_fill(void);

static void jpeg_start(void)
{
    uint8_t *buf = stream.buf;

    cbd_buf_owner = CBD_JPEG;

    stream.minblk = 0;
    stream.minblk_ptr = buf;
    stream.tail = buf;
    stream.endblk = 0;

    chain = &end;
    layout = stream.layout;
    memcpy(buf, jpeg_header, sizeof jpeg_header);

    // initialise bitstream output and change the background to white
    jpeg.buf = buf + sizeof jpeg_header;
    jpeg.leftover_bits = 0;
    jpeg.num_leftover_bits = 0;

    copy_bitstream(0, 0, GTW_LEN, GTW_BITS, JWIDTH - 1);
    dy = 1;

    memset(pic_fragments, 0, sizeof pic_fragments);
    memset(qr_fragments, 0, sizeof qr_fragments);
    memset(text_fragments, 0, sizeof text_fragments);

    jpeg_fill();
}

void jpeg_init(uint8_t *buf, uint8_t *endbuf, const struct Layout *l)
{
    stream.buf = buf;
    stream.end = endbuf;
    stream.layout = l;

#if USE_EXT_FLASH
    FRESULT res = f_open(&lay_file, "0:default.lay", FA_READ);
    if (res != FR_OK) {
        // probably no such file
        global_error_flags |= NO_LAYOUT_FILE;
        return;
    }
#endif
}

static bool jpeg_more(void)
{
    struct fgm_ctl **fgm;

    if (chain->next == 0) {
        // no active fragments; output whitespace until the next layout item
        int gap = layout->vstep - dy;
        if (gap > 20) {
            gap = 20;
            whitespace(gap * JWIDTH);
            dy += gap;
            return true;
        }
        if (gap)
            whitespace(gap * JWIDTH);
        dy += gap;
    }

    while (dy == layout->vstep) {
        // next layout item becomes active
        const uint16_t *pic;

        while (layout->type == FGM_LARGE_PICTURE) {
            pic = layout++->pic;
            // pic points at the body, which consists of uint16_t fields:
            //  addr_l    - address in serial flash, lower word
            //  addr_h    - address in serial flash, higher word
            //  height    - in macroblocks
            //  nwords    - number of words in bistream in serial flash
            //  nbits     - number of leftover bits to follow
            //  [bits]    - leftover bits if nbits > 0
            unsigned addr = pic[0] | pic[1] << 16;      // address in flash
            int vgap = layout->vstep - pic[2];          // step minus height
            copy_bitstream_from_flash(addr, pic[3], pic[4], pic[5], vgap * JWIDTH);
        }

        if (layout->type == FGM_STOP) {
            finalise_jpeg();
            return false;
        }

        dy = 0;

        struct fgm_ctl *new_item = &end;

        switch (layout->type) {
        case FGM_PICTURE_BY_REF:
            pic = *layout->picref;
            goto fgm_picture;

        case FGM_PICTURE:
            pic = layout->pic;
fgm_picture:
            // pic points at body of uint16_t fields:
            //  addr_l              - address in flash, lower word
            //  addr_h              - address in flash, higher word
            //  { width, height }   - in macroblocks
            //  then for each row (height times):
            //    { nbits, nwords } - bitstream parameters
            //    [bits]            - leftover bits if nbits > 0
            new_item = ALLOC(pic_fragments);
            new_item->state->pic.addr = pic[0] | pic[1] << 16;  // addr in flash
            new_item->state->pic.rows_left = pic[2] & 0xff;     // height
            new_item->state->pic.width = pic[2] >> 8;           // width
            new_item->state->pic.ptr = pic + 3;                 // data
            new_item->render = render_pic;
            break;

        case FGM_QR:
            new_item = ALLOC(qr_fragments);
            new_item->state->qr.row = 0;
            new_item->state->qr.idx = layout->qr.idx;
            new_item->state->qr.size = layout->qr.size;
            qr_encode(texts[layout->qr.idx], new_item->state->qr.qr,
                      layout->qr.size);
            new_item->render = render_qr;
            break;

        case FGM_TEXT:
            new_item = ALLOC(text_fragments);
            new_item->state->text.idx = layout->text.idx;
            new_item->state->text.width = layout->text.width;
            new_item->state->text.row = 0;
            new_item->state->text.text = texts[layout->qr.idx];
            new_item->render = render_text;
            break;

        default:
            assert(0);
        }

        // insert new_item into the chain according to x
        new_item->x = layout->x;
        for (fgm = &chain; (*fgm)->x < new_item->x; fgm = &(*fgm)->next);
        new_item->next = *fgm;
        *fgm = new_item;

        layout++;
    }

    // render current row; we know it's not empty
    unsigned x = chain->x;
    whitespace(x);
    fgm = &chain;
    do {
        unsigned next_x = (*fgm)->next->x;
        bool keep = (*fgm)->render((*fgm)->state, next_x - x);
        x = next_x;
        if (keep)
            fgm = &(*fgm)->next;
        else {
            struct fgm_ctl *finished_item = *fgm;
            *fgm = finished_item->next;
            dealloc(finished_item);
        }
    } while ((*fgm)->next != 0);

    dy++;

    return true;
}

#if DEBUG
static bool jpeg_debug_more(void)
{
    uint8_t *prev = jpeg.buf;
    bool res = jpeg_more();
    printf("jpeg_more(): consumed %5d, dy %u, layout %d\n", jpeg.buf - prev,
            dy, layout >= shamir_layout ? layout - shamir_layout : layout - main_layout);
    jpeg_dump();
    return res;
}
#define jpeg_more   jpeg_debug_more
#endif

static void jpeg_fill(void)
{
    if (stream.endblk)
        return;

    enum {
        // Maximum size of data that a single call to jpeg_more can generate.
        // This is usually the maximum size (in bytes) of one macroblock row
        // and determined empirically.  It shouldn't exceed 1/2.5 of the space
        // available for jpeg buffering, i.e. entropy_size.
        RESERVE = 8 * 1024
    };

    uint8_t *limit = stream.minblk_ptr > jpeg.buf ? stream.minblk_ptr :
                                                    stream.end;
    limit -= RESERVE;
    while (jpeg.buf <= limit)
        if (!jpeg_more()) {
            int left = ((stream.buf - jpeg.buf) & (BLKSIZE - 1)) + BLKSIZE;
            memset(jpeg.buf, 0xff, left);
            jpeg.buf += left;
            stream.endblk = stream.minblk + (
                    jpeg.buf > stream.minblk_ptr ? jpeg.buf - stream.minblk_ptr
                    : stream.tail - stream.minblk_ptr + (jpeg.buf - stream.buf)
                ) / BLKSIZE;
            return;
        }
    assert(jpeg.buf <= limit + RESERVE);

    if (jpeg.buf < stream.minblk_ptr)
        return;

    // We hit the end of the buffer, let's move to the beginning
    // if there is space.
    if (stream.minblk_ptr >= stream.buf + BLKSIZE) {
        int carry = (jpeg.buf - stream.buf) & (BLKSIZE - 1);
        stream.tail = jpeg.buf - carry;
        if (carry)
            memcpy(stream.buf, stream.tail, carry);
        jpeg.buf = stream.buf + carry;
    }
}

uint8_t * jpeg_get_block(unsigned blk)
{
    if (cbd_buf_owner != CBD_JPEG || blk < stream.minblk) {
#if DEBUG
        printf("Requested block %u, ", blk);
        if (cbd_buf_owner == CBD_JPEG)
            printf ("minimum now %u.  Restarting.\n", stream.minblk);
        else
            printf ("taking ownership from %u.  Restarting.\n", cbd_buf_owner);
#endif
        jpeg_start();
    }

    for (; blk > stream.minblk + 4; jpeg_fill()) {
        if (jpeg.buf < stream.minblk_ptr) {
            if (stream.minblk_ptr < stream.tail) {
                stream.minblk++;
                stream.minblk_ptr += BLKSIZE;
                continue;
            }
            stream.minblk_ptr = stream.buf;
        }
        if (jpeg.buf >= stream.minblk_ptr + BLKSIZE) {
            stream.minblk++;
            stream.minblk_ptr += BLKSIZE;
            continue;
        }
        if (stream.endblk && blk >= stream.endblk)
            return jpeg.buf - BLKSIZE;
    }

    // Here stream.minblk <= blk <= stream.minblk + 4.

    if (stream.endblk && blk >= stream.endblk)
        return jpeg.buf - BLKSIZE;

    uint8_t *blk_ptr = stream.minblk_ptr + (blk - stream.minblk) * BLKSIZE;

    if (jpeg.buf < blk_ptr && blk_ptr >= stream.tail)
        blk_ptr = stream.buf + (blk_ptr - stream.tail); // wrap around

    return blk_ptr;
}
