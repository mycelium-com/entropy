#!/usr/bin/python
#
# Convert a regular (e.g., TTF) font file to a raster representation
# for use on a small OLED screen.
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

try:
    from freetype import *
except ImportError:
    print "Freetype library bindings for Python are required for this product."
    print "See freetype-py package at http://code.google.com/p/freetype-py/ ."
    sys.exit(1)
except RuntimeError:
    try:
        import ctypes.util
        ctypes.util.find_library = lambda x: "/usr/X11/lib/lib" + x + ".6.dylib"
        from freetype import *
    except RuntimeError:
        ctypes.util.find_library = lambda x: "/opt/local/lib/lib" + x + ".6.dylib"
        from freetype import *

# Select font and processing parameters.
fdir  = "/Library/Fonts"
fname = "verdana"
ffile = fdir + "/Verdana.ttf", 12
cut_top = 0 #2
cut_bot = 0 #1

class Object:
    pass

# Load typeface.
face = Face(ffile[0])
face.set_char_size(ffile[1] * 64)
full_height = (face.size.ascender - face.size.descender + 63) >> 6

if face.is_fixed_width:
    w = "fixed"
else:
    w = "variable"

# After cutting cut_top and cut_bot lines:
# h is the effective font height
# base is the number of lines from top to the baseline (i.e. the new ascender)
h = full_height - cut_top - cut_bot
base = ((face.size.ascender + 63) >> 6) - cut_top
p = (h + 7) / 8         # number of 8-bit vertical "pages"

print "Font height:", h, "(total %d)." % full_height

outh = open(fname + ".c", "w")

outh.write("""// Automatically generated from %s size %d.
// Typeface: %s %s; %s width.

#include <stdint.h>

const uint8_t %s_font_height = %d;
const uint8_t %s_font_data[] = {
""" % (ffile + (face.family_name, face.style_name, w, fname, h, fname)))

offset = 0
offsets = []

for char in range(32, 127):
    face.load_char(chr(char), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO)

    bitmap  = face.glyph.bitmap
    left    = face.glyph.bitmap_left
    top     = face.glyph.bitmap_top
    advance = face.glyph.advance.x

    start_row = base - top
    end_row   = start_row + bitmap.rows

    # Check bitmap height to ensure we didn't cut too much.
    if start_row < 0:
        print "Error: '%c' is clipped on the top." % char
        sys.exit(1)
    if end_row > h:
        print "Error: '%c' is clipped on the bottom." % char
        sys.exit(1)

    w = (advance + 63) >> 6             # width (now supported by epd.c)
    cols = [0] * w                      # column bitmasks

    offsets.append(offset)
    offset += w * p + 1

    for x in range(w - left):
        bits = 0
        bptr = 0
        for y in range(h):
            if y >= start_row and y < end_row:
                pixel = bitmap.buffer[bptr + x/8] >> (~x & 7) & 1
                bptr += bitmap.pitch
                bits |= pixel << y
        cols[x + left] = bits

    outh.write("\t%3d,\t// '%c'\n" % (w, char))
    for y in range(p):
        outh.write("\t\t")
        for x in range(w):
            outh.write("0x%02x," % (cols[x] >> y * 8 & 0xff))
        outh.write("\n")

outh.write("};\n\n")

outh.write("const uint16_t %s_font[] = {\n" % fname)
for char in range(32, 127):
    outh.write("\t%5d,\t// '%c'\n" % (offsets[char - 32], char))
outh.write("};\n")
outh.close()
