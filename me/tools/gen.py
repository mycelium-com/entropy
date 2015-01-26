#!/usr/bin/python
#
# JPEG data generation tool.
# This tool converts graphics, texts and font glyphs into JPEG bitstream
# elements that can be used by the embedded firmware in Mycelium Entropy.
#
# Copyright 2014 Mycelium SA, Luxembourg.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.  See file GPL in the source code
# distribution or <http://www.gnu.org/licenses/>.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import sys
import struct
import string
import binascii
import re
import os.path
from PIL import Image, ImageFont, ImageDraw

# Configuration parameters
JWIDTH      = 226
JHEIGHT     = 304
DPI         = 220

# Font for section headers
FONT        = "/System/Library/Fonts/Optima.ttc", 44
TEXTS       = ("Bitcoin Address", "Private Key", "Share 1 of 3", "Share 2 of 3",
               "Share 3 of 3", "(any two shares reveal the key)",
               "Litecoin Address",
               "Entropy for verification:", "Key = SHA-256(salt1 || Entropy)")

# Small font and text for the small print at the bottom
SFONT       = "/System/Library/Fonts/Optima.ttc", 32
STEXT       = "Created by Mycelium Entropy.   mycelium.com"

# Monospace font for text next to QR codes
MFONT       = "/System/Library/Fonts/Menlo.ttc", 40
MTEXT       = " 0123456789-ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

# Just for tuning the JPEG optimiser.  It's ok to have occasional 33x33 QRs.
QR_SIZE = 29


def main():
    if sys.argv[1:2] == ["--check"]:
        ImageFont.truetype(*FONT)
        ImageFont.truetype(*SFONT)
        ImageFont.truetype(*MFONT)
        print "OK"
        return

    if len(sys.argv) != 2:
        print "Usage: ", sys.argv[0], "jpeg-data"
        print "(to generate jpeg-data.{h,c,jpg,bin} and jpeg-data-ext.c)."
        sys.exit(1)

    # parameters derived from configuration
    jdata_name  = sys.argv[1]
    jpeg_name   = jdata_name + ".jpg"
    font        = ImageFont.truetype(*FONT)
    mfont       = ImageFont.truetype(*MFONT)
    mtw, mth    = mfont.getsize(MTEXT)
    mth         = mth / 8 + 1
    mtw         = mtw / len(MTEXT) / 8

    print "Character: %dx%d" % (mtw, mth)

    rects = make_jpeg(jpeg_name, font, mfont, mtw, mth)
    make_c_code(jdata_name, jpeg_name, font.getname(), mfont.getname(), rects)

def put_qr_stripes(drw, row):
    # draw a sequence of alternating B/W macroblocks to tune the optimiser
    yend = (row + QR_SIZE) * 8 - 1
    for x in range(0, QR_SIZE*2*8, 16):
        drw.rectangle((x, row*8, x+7, yend), fill="black")

nonid_re = re.compile("[^a-zA-Z0-9_]")
def text_to_id(text):
    text = nonid_re.sub(" ", text.lower()).split()
    return "_".join(text[:4])

def add_text_fragment(rects, drw, font, y, text, idn = None):
    if not isinstance(font, list):
        font = [font]
        text = [text]
    x = 0
    xx = []
    H = 0
    for i in range(len(font)):
        xx += [x]
        w, h = font[i].getsize(text[i])
        x += w
        H = max(H, h)
    w = (x + 7) / 8 + 1 # width in macroblocks plus one white at the end
    h = (H + 14) / 8    # height in macroblocks

    print '"' + "".join(text) + '":', "%dx%d" % (w, h)

    if not idn:
        idn = text_to_id("".join(text))

    for i in range(len(font)):
        drw.text((xx[i], y*8), text[i], font=font[i])
    fgm = Fragment(idn + "_fragment", w, h)
    rect = Rect(0, y, w, h, fgm)
    rects.append(rect)

    return y + h


def make_jpeg(jpeg_name, font, mfont, mtw, mth):
    global QR_ROW
    rects = []

    img = Image.new("L", (JWIDTH * 8, JHEIGHT * 8), "white")
    drw = ImageDraw.Draw(img)

    QR_ROW = 54
    put_qr_stripes(drw, QR_ROW)

    # draw Base-58 alphabet to extract font macroblocks from
    y = QR_ROW + 40
    drw.text((0, y*8 - 6), MTEXT, font=mfont)
    fgm = FontData(mtw, mth)
    rect = Rect(0, y, mtw * len(MTEXT), mth, fgm)
    rects.append(rect)
    y += 7

    # "cut here"
    dingfont = ImageFont.truetype("/System/Library/Fonts/ZapfDingbats.ttf", 44)
    yy = y * 8 + 19
#    for x in [0] + range(64, JWIDTH * 8, 24):
#        drw.rectangle((x, yy, x+15, yy+2), fill="black")
#    drw.text((20, y*8), u"\u2702", font=dingfont)

    black = Image.new("L", (JWIDTH * 8, 40), "black")
    cut_here = black.copy()
    cdrw = ImageDraw.Draw(cut_here)
    for x in [0, 24, 48, 72] + range(72+64, JWIDTH * 8, 24):
        cdrw.rectangle((x, 19, x+15, 21), fill="white")
    cdrw.text((72+20, 0), u"\u2702", font=dingfont, fill="white")
    img.paste(black, (0, y * 8), cut_here)

    fgm = Fragment("cut_here_fragment", 0, 5)
    rect = Rect(0, y, JWIDTH, 5, fgm)
    rects.append(rect)
    #cut_here_rect = [0, y*8, 0, y*8 + 39]
    y += 5

    # texts
    for text in TEXTS:
        y = add_text_fragment(rects, drw, font, y, text)
    y = add_text_fragment(rects, drw, ImageFont.truetype(*SFONT), y, STEXT,
                          "small_print")

    # add Mycelium logo
    logo = os.path.dirname(os.path.realpath(sys.argv[0]))
    logo = Image.open(os.path.join(logo, "Mycelium_Logo_CMYK_120dpi.png"))
    logo = logo.convert("RGB").convert("L", (0.214, 0.537, 0.249, 0))
    w, h = logo.size
    logo = logo.crop((0, 0, w, 58 * 8))
    w, h = logo.size
    w = (w + 15) / 8
    h = (h + 7) / 8
    assert h == 58
    x = JWIDTH - w - 1
    y = 10
    #cut_here_rect[2] = h * 8 - 41
    #img.paste(img.crop(cut_here_rect).rotate(270), ((x + w/2 - 3) * 8, 5))
    img.paste(logo, (x*8, y*8))
    if False:   # change to True to draw vertical dashed line
        black = black.crop((64, 0, h * 8 - 1, 39)).rotate(270)
        cut_here = cut_here.crop((64, 0, h * 8 - 1, 39)).rotate(270)
        img.paste(black, ((x + w/2 - 3) * 8 + 1, 8), cut_here)
    tx, tw, th = 28, 10, 20     # top part's offset, width and height
    mx = (w - w/4) / 4          # middle part's offset
    mw = w - 2 * mx             # middle part's width
    mh = 25                     # middle part's height
    if False:   # change to True to draw vertical dashed line
        fgm = Fragment("logo_top_fragment", tw, th + 9)
        rects.append(Rect(x + tx, y - 9, tw, th + 9, fgm))
    else:
        fgm = Fragment("logo_top_fragment", tw, th)
        rects.append(Rect(x + tx, y, tw, th, fgm))
    fgm = Fragment("logo_middle_fragment", mw, mh)
    rects.append(Rect(x + mx, y + th, mw, mh, fgm))
    fgm = Fragment("logo_bottom_fragment", w, h - th - mh)
    rects.append(Rect(x, y + th + mh, w, h - th - mh, fgm))

    img.save(jpeg_name, quality=100, optimize=True)

    return rects

def make_c_code(jdata_name, jpeg_name, hdr_font, addr_font, rects):
    jpeg = JpegData(jpeg_name, rects)
    same = mb_to_bits(jpeg.same)
    wtb = mb_to_bits(jpeg.wtb)
    btw = mb_to_bits(jpeg.btw)
    gtw = mb_to_bits(jpeg.gtw)
    ac_eob = mb_to_bits(jpeg.same[1:])
    print "Same: %2d %4x " % same, jpeg.same
    print "WTB:  %2d %4x " % wtb, dc_value(jpeg.wtb), jpeg.wtb
    print "BTW:  %2d %4x " % btw, dc_value(jpeg.btw), jpeg.btw
    print "GTW:  %2d %4x " % gtw, dc_value(jpeg.gtw), jpeg.gtw
    #total_bytes = 0
    #for i in range(len(MTEXT)):
    #    nbytes = [ (r.nbits + 7) / 8 for r in font_data.data[i] ]
    #    s = sum(nbytes)
    #    total_bytes += s
    #    #print MTEXT[i] + ":", nbytes, s
    #print "Font bytes:", total_bytes, "; avg/char:", float(total_bytes) / len(MTEXT)
    #print "Ktext:", len(ktext.data), "bytes,", ktext.nbits, "bits."

    # quick hack to set resolution
    #jpeg.bytes[13] = 1          # units are dpi
    #jpeg.bytes[14] = jpeg.bytes[16] = DPI >> 8
    #jpeg.bytes[15] = jpeg.bytes[17] = DPI & 0xff

    h_file = open(jdata_name + ".h", "w")
    c_file = open(jdata_name + ".c", "w")
    bin_file = open(jdata_name + ".bin", "w+b")

    comment = """// Automatically generated JPEG data file.
// Size: %dx%d pixels / %dx%d macroblocks.
// Header font: %s %s, %s pt.
// Data font:   %s %s, %s pt.
""" % ((jpeg.width, jpeg.height, jpeg.wmb, jpeg.hmb) + hdr_font + (FONT[1],) +
       addr_font + (MFONT[1],))

    h_file.write(comment)
    h_file.write('\n#include <stdint.h>\n')
    c_file.write(comment)
    c_file.write('\n#include "' + os.path.basename(jdata_name) + '.h"\n')
    h_file.write("\n// Parameters (used in #ifdef hence not an enum):\n")
    for p in ("JWIDTH", "JHEIGHT"):
        h_file.write("#define %-10s %4d\n" % (p, globals()[p]))

    h_file.write("\n// JPEG bitstream elements:\n")
    for name, pair in (("SME", same), ("WTB", wtb), ("BTW", btw)):
        h_file.write("#define %s { %2d, %#6x }\n" % ((name,) + pair))
    h_file.write("#define AC_EOB_LEN  %6d\n#define AC_EOB_CODE %#6x\n" % ac_eob)
    same_len, same_bits = same
    mult = 25 / same_len
    mult_bits = int(("0" * (same_len-1) + "1") * mult, 2) * same_bits
    h_file.write("""enum {
    SME_LEN             = %d,
    SME_BITS            = %#x,
    SME_LONG_MB         = %d,
    SME_LONG_LEN        = %d,
    SME_LONG_BITS       = %#x,
    GTW_LEN             = %d,
    GTW_BITS            = %#x,\n};\n"""
        % (same + (mult, same_len * mult, mult_bits) + gtw))

    jpeg.dcht.out(h_file, c_file)
    for rect in rects:
        rect.holder.out(h_file, c_file, bin_file)
        c_file.write("\n")
    h_file.write("\n// JPEG header.\n")
    c_file.write("// JPEG header.\n")
    write_byte_array(h_file, c_file, "jpeg_header", jpeg.bytes[:jpeg.bstr_pos])
    c_file.close()
    h_file.close()

    ext_file = open(jdata_name + "-ext.c", "w")
    ext_file.write("// Automatically generated JPEG external data file.\n\n")
    bin_file.seek(0)
    ext_name = os.path.basename(jdata_name).translate(
            "_" * ord('0') + string.digits + "_" * (ord('A') - ord('9') - 1) +
            string.ascii_uppercase + "_" * 6 + string.ascii_lowercase +
            "_" * (5 + 128)) + "_bin"
    write_byte_array(None, ext_file, ext_name, bytearray(bin_file.read()), 2)
    ext_file.close()
    bin_file.close()

def write_byte_array(h, c, name, array, alignment = ""):
    if h:
        h.write("extern const unsigned char %s[%d];\n" % (name, len(array)))
    if alignment:
        alignment = " __attribute__ ((aligned (%d)))" % alignment
    c.write("const unsigned char %s[%d]%s = {\n   " % (name, len(array), alignment))
    for i in range(len(array)):
        c.write(" 0x%02X," % array[i])
        if i % 12 == 11 and i != len(array):
            c.write("\n   ")
    c.write("\n};\n")

def write_word(f, i, word):
    if (i & 7) == 0:
        f.write("\n   ")
    f.write(" 0x%04X," % word)

class HuffmanTable(dict):
    DC = 0
    AC = 1

    def __init__(self, data):
        data = bytearray(data)
        self.type = data[0] >> 4
        self.index = data[0] & 0xf
        code = 0                # current code
        i = 17                  # index of the next symbol
        for l in range(1, 17):  # code length
            code <<= 1
            for j in range(data[l]):
                self[(l, code)] = data[i]
                i += 1
                code += 1
        assert i == len(data)

    def show(self):
        t = []
        for length, code in self:
            c = "0" * 15 + bin(code)[2:]
            c = c[-length:]
            t += [(c, self[(length, code)])]
        t.sort()
        print "Huffman %cC table %d:" % ("DA"[self.type], self.index)
        for c in t:
            print "  %-8s %02x" % c

    def out(self, h, c):
        t = [(0, 0)] * 12
        for code in self:
            t[self[code]] = code
        decl = "\n// Huffman %cC table %d\n" % ("DA"[self.type], self.index)
        h.write(decl)
        c.write(decl)
        h.write("""struct Huffman_table {
    uint16_t len;
    uint16_t code;
};
extern """)
        decl = "const struct Huffman_table huff_%cc[12]" % "da"[self.type]
        h.write(decl + ";\n")
        c.write(decl + " = {\n")
        for code in t:
            c.write("    { %2d, %#06x },\n" % code)
        c.write("};\n")

class BitReader:
    def __init__(self, data, idx):
        self.data = data
        self.idx = idx
        self.buf = 0
        self.nbits = 0
        self.ckp = 0
        self.ckpname = ""

    def next_bit(self):
        if self.nbits == 0:
            self.buf = self.data[self.idx]
            self.idx += 1
            if self.buf == 0xff:
                self.idx += 1
            self.nbits = 8
        self.nbits -= 1
        return (self.buf >> self.nbits) & 1

    def next_value(self, ht):
        cl = 0          # accumulated code length
        code = 0        # accumulated code
        value = 0       # accumulated value
        while True:
            code = code << 1 | self.next_bit()
            cl += 1
            if (cl, code) in ht:
                break
        vl = ht[(cl, code)]
        rl = vl >> 4    # run length for AC
        vl = vl & 0xf   # value length
        for i in range(vl):
            value = value << 1 | self.next_bit()

        return (cl, code), (vl, value), rl

    def show_size(self, what):
        if what != self.ckpname:
            print what + ":", self.idx - self.ckp, "bytes."
            self.ckp = self.idx
            self.ckpname = what

def decode_value((l, v)):
    l = 1 << l
    if v >= l >> 1:
        return v
    return v - l + 1

def mb_to_bits(macroblock):
    word = 0
    nbits = 0
    macroblock = reduce(lambda x,y: x + [y[0], y[1]], macroblock, [])
    for nb, w in macroblock:
        word = word << nb | w
        nbits += nb
    return nbits, word

def dc_value(macroblock):
    nbits, value = macroblock[0][1]
    bias = 1 << nbits
    if value < bias >> 1:
        value -= bias - 1
    return value

class Rect:
    def __init__(self, x, y, w, h, holder):
        self.l = x
        self.t = y
        self.r = x + w
        self.b = y + h
        self.holder = holder

    def has(self, x, y):
        return y >= self.t and y < self.b and x >= self.l and x < self.r

class Fragment:
    def __init__(self, name, width, height):
        self.data = bytearray()
        self.rows = []
        self.macroblocks = 0
        self.nbits = 0
        self.buf = 0
        self.name = name
        self.width = width
        self.height = height

    def add(self, mb):
        nb, w = mb_to_bits(mb)
        w = self.buf << nb | w
        nb += self.nbits
        while nb >= 8:
            nb -= 8
            self.data.append(w >> nb & 255)
        w &= (1 << nb) - 1

        self.macroblocks += 1
        if self.macroblocks == self.width:
            self.macroblocks = 0
            data = self.data
            if len(data) & 1:
                w |= data[-1] << nb
                nb += 8
                data = data[:-1]
            self.rows.append((data, nb, w))
            nb = 0
            w = 0
            self.data = bytearray()

        self.nbits = nb
        self.buf = w

    def write_word_array(self, f, *words):
        i = self.wcnt
        for x in words:
            if isinstance(x, int):
                write_word(f, i, x)
                i += 1
            elif isinstance(x, bytearray):
                for j in range(0, len(x), 2):
                    write_word(f, i, x[j] << 8 | x[j+1])
                    i += 1
            else:
                for j in x:
                    write_word(f, i, j)
                    i += 1
        self.wcnt = i

    def write_bin_data(self, f, data):
        # Data are a bytearray with most significant bit first.
        # We want to write it as 2-byte words, and since ARM is little endian,
        # bytes should be swapped pairwise.
        le_data = bytearray(len(data))
        le_data[::2] = data[1::2]
        le_data[1::2] = data[::2]
        f.write(le_data)

    def out(self, h, c, bin_file):
        if self.width == 0:
            c.write("// Full width picture.\n")
        else:
            c.write("// Rectangular picture.\n")
        h.write("extern const uint16_t %s[];\nenum {\n" % self.name)
        if self.width:
            h.write("    %s_WIDTH  = %d,\n" % (self.name.upper(), self.width))
        h.write("    %s_HEIGHT = %d\n};\n" % (self.name.upper(), self.height))
        c.write("const uint16_t %s[] = {" % self.name)
        self.wcnt = 0
        body_addr = bin_file.tell()
        self.write_word_array(c, body_addr & 0xffff, body_addr >> 16)
        total_wcnt = self.wcnt
        self.wcnt = 0

        if self.width == 0:
            # large picture
            c.write("    // height %d" % self.height)
            data = self.data
            nbits = self.nbits
            buf = self.buf
            if len(data) & 1:
                buf |= data[-1] << nbits
                nbits += 8
                data = data[:-1]
            if len(data) >= 65536 * 2:
                raise Exception("Fragment %s overflow: %d." % (self.name, len(data)))
            tail = []
            if nbits != 0:
                tail = [buf]
            self.write_word_array(c,
                    self.height,
                    len(data) >> 1,     # nwords
                    nbits,
                    tail)
            self.write_bin_data(bin_file, data)
            total_wcnt += self.wcnt
        else:
            # small picture
            self.write_word_array(c, self.width << 8 | self.height)
            c.write("    // width %d, height %d" % (self.width, self.height))
            for data, nbits, buf in self.rows:
                if len(data) >= 65536 / 8:
                    raise Exception("Fragment %s overflow: %d." % (self.name, len(data)))
                tail = []
                if nbits != 0:
                    tail = [buf]
                self.wcnt = 0
                self.write_word_array(c,
                        nbits | len(data) << 3,
                        tail)
                self.write_bin_data(bin_file, data)
                total_wcnt += self.wcnt
        c.write("\n};\n")
        #print self.name + ":", total_wcnt * 2, "bytes."

class FontRow:
    def __init__(self, prev_dc):
        self.macroblocks = []
        self.first_dc = prev_dc
        self.total_dc = prev_dc

    def append(self, macroblock):
        self.total_dc += dc_value(macroblock)
        self.macroblocks += macroblock

    def pack(self):
        # separate the first DC
        self.first_dc += dc_value(self.macroblocks)
        self.nbits, self.data = mb_to_bits(self.macroblocks[1:])
        if self.nbits >= 2048:
            raise Exception("Warning: %d bytes." % self.nbits / 8)
        self.macroblocks = None

class FontData:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.data = [ [] for i in MTEXT ]
        self.x = 0

    def add(self, mb):
        x = self.x
        self.x += 1
        if self.x >= self.width * len(MTEXT):
            self.x = 0
        if x == 0:
            self.prev_dc = 0
        char = x / self.width
        x %= self.width
        if x == 0:
            self.font_row = FontRow(self.prev_dc)
            self.data[char].append(self.font_row)
        self.font_row.append(mb)
        if x == self.width - 1:
            self.font_row.pack()
            self.prev_dc = self.font_row.total_dc
            #print "'%s' (row %d): %5d %5d" % (MTEXT[char],
            #        len(self.data[char]) - 1, self.font_row.first_dc,
            #        self.font_row.total_dc)

    def out(self, h, c, bin_file):
        # bin_file is not used
        h.write("\n// Font data encoded as JPEG bitstream\n")
        c.write("\n// Font data encoded as JPEG bitstream\n")
        # Make character mapping table.
        fmap = bytearray(ord(max(MTEXT)) - ord(' ') + 1)
        for i in range(len(MTEXT)):
            fmap[ord(MTEXT[i]) - ord(' ')] = i
        # Aggregate all characters into a single data array,
        # computing offsets as we go.
        array = bytearray()
        for char in self.data:
            for row in char:
                row.offset = len(array)
                tbits = row.nbits & 7           # number of trailing bits
                if row.nbits != tbits:
                    # whole bytes
                    n = row.data >> tbits
                    s = "%0*x" % ((row.nbits - tbits) / 4, n)
                    array += binascii.unhexlify(s)
                if tbits:
                    # trailing bits
                    array.append(row.data & ((1 << tbits) - 1))

        # Output control structures for each character row
        h.write("""
enum {
    CHR_WIDTH   = %d,
    CHR_HEIGHT  = %d,
};

struct Chr_row_descr {
    int16_t dc_in;
    int16_t dc_out;
    uint8_t nbytes;
    uint8_t nbits;
    uint16_t offset;
};

extern const struct Chr_row_descr addr_font[%d][CHR_HEIGHT];
""" % (self.width, self.height, len(self.data)))
        write_byte_array(h, c, "font_map", fmap)
        c.write("""
const struct Chr_row_descr addr_font[%d][CHR_HEIGHT] = {
""" % len(self.data))
        for char_idx in range(len(self.data)):
            c.write("    {  // '%c'\n" % MTEXT[char_idx])
            for row in self.data[char_idx]:
                c.write("        { %5d, %5d, %3d, %d, %5d },\n"
                        % (row.first_dc, row.total_dc, row.nbits / 8,
                           row.nbits & 7, row.offset))
            c.write("    },\n")
        c.write("};\n\n")

        # Output font data
        write_byte_array(h, c, "addr_font_data", array)

        h.write("\n// Pictures.\n")


class JpegData:
    def __init__(self, filename, rects):
        jpeg = bytearray(open(filename, "rb").read())
        jpeg[2:20] = exif()
        open(filename, "wb").write(jpeg)
        self.bytes = jpeg
        self.rects = rects
        i = 0
        while True:
            b = jpeg[i]
            if b != 0xff:
                raise Exception("Not a tag: 0x%02X at offset %d." % (b, i))
            b = jpeg[i+1]
            i += 2
            if b == 0xd8:       # start of image
                continue
            if b == 0xd9:       # end of image
                break
            size = jpeg[i] << 8 | jpeg[i+1]
            if b == 0xc4:       # define Huffman tables
                h = HuffmanTable(jpeg[i+2 : i+size])
                if h.type == h.DC:
                    self.dcht = h
                else:
                    self.acht = h
            elif b == 0xc0:     # start of frame
                self.hwpos = i + 3
                self.height, self.width = struct.unpack(">HH", jpeg[i+3 : i+7])
                self.hmb = (self.height + 7) / 8
                self.wmb = (self.width + 7) / 8
            i += size
            if b == 0xda:       # start of scan
                self.scan(BitReader(jpeg, i))
                break

    def scan(self, data):
        #self.dcht.show()
        #self.acht.show()
        for dc0, v in self.dcht.items():
            if v == 0:
                break
        for ac0, v in self.acht.items():
            if v == 0:
                break
        self.same = ((dc0, (0, 0), 0), (ac0, (0, 0), 0))
        self.bstr_pos = data.idx

        x = 0           # coordinates of the
        y = 0           # current macroblock

        while y < self.hmb:
            # read next macroblock
            dc = data.next_value(self.dcht)
            mb = [ dc ] # macroblock
            mbv = [ decode_value(dc[1]) ] + 63 * [0]
            n = 0       # number of AC coefficients processed
            while n < 63:
                code, value, rl = data.next_value(self.acht)
                mb.append((code, value, rl))
                if rl == 0 and value[0] == 0:
                    break       # end of ACs
                n += rl + 1
                mbv[n] = decode_value(value)

            # add this macroblock to all applicable rectangles
            for rect in self.rects:
                if rect.has(x, y):
                    rect.holder.add(mb)
            if y == 0:
                if x == 0:
                    self.gtw = mb
            elif y == QR_ROW:
                if x == 0:
                    self.wtb = mb
                elif x == 1:
                    self.btw = mb
            x += 1
            if x == self.wmb:
                x = 0
                y += 1

def exif():
    hdr = struct.pack("<2sHI", "II", 42, 8)
    offset = len(hdr) + 2 + 4 * 12 + 4
    ifd = (
            0x011a, 5, 1, offset,       # x resolution
            0x011b, 5, 1, offset + 8,   # y resolution
            0x0128, 3, 1, 2,            # resolution unit: dpi
            0x8769, 4, 1, offset + 16,  # offset to Exif sub-IFD
    )
    m = len(ifd) / 4
    ifd = struct.pack("<H" + m * "HHII" + "I", *((m,) + ifd + (0,)))
    content = hdr + ifd + struct.pack("<IIII", DPI, 1, DPI, 1)
    offset = len(content) + 2 + 6 * 12 + 4
    subifd = (
            0x9000, 7, 4, 0x30333230,   # version "0230"
            0xa001, 3, 1, 1,            # colour space: sRGB
            0xa500, 5, 1, offset,       # gamma
            0x9101, 7, 4, 1,            # components configuration: Y
            0xa002, 4, 1, JWIDTH * 8,   # width
            0xa003, 4, 1, JHEIGHT * 8,  # height
    )
    e = len(subifd) / 4
    subifd = struct.pack("<H" + e * "HHII" + "I", *((e,) + subifd + (0,)))
    content += subifd + struct.pack("<II", 22, 10)

    content = struct.pack(">HH6s", 0xffe1, len(content) + 8, "Exif") + content
    return content

main()
