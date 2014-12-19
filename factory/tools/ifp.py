#!/usr/bin/python
#
# Initial Flash Programming tool.
#
# Copyright 2014 Mycelium SA, Luxembourg.
#
# This file is part of Mycelium Entropy.
#
# Mycelium Entropy is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.  See file GPL in the source code
# distribution or <http://www.gnu.org/licenses/>.
#
# Mycelium Entropy is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import sys
import struct
import hashlib
import argparse

BLOCK_SIZE              = 512   # in bytes
DATA_OFFSET_FROM_END    = 128   # in 512-byte blocks

ap = argparse.ArgumentParser(description="Program serial flash in an IFP mode device.")
ap.add_argument("device", type=argparse.FileType("r+"), help="device file")
ap.add_argument("size", type=int, help="device size in blocks")
ap.add_argument("model", help="IFP model name")

args = ap.parse_args()

fblk = args.size & 1023                 # first block of the serial flash
dblk = args.size - DATA_OFFSET_FROM_END # first block of data
if args.model == "IFP_Mode_2":
    # Mode 2 supports writing from arbitrary addresses
    start = (dblk - fblk) * BLOCK_SIZE
    fblk = dblk
else:
    # other modes require write to start from 0
    start = 0

h = hashlib.sha256()
padding = "\xff" * BLOCK_SIZE
args.device.seek(fblk * BLOCK_SIZE)

while fblk < dblk:
    args.device.write(padding)
    h.update(padding)
    fblk += 1

data = open("en-US.b39", "rb").read()
magic = struct.pack("32s", "End of data in the filesystem.")
data += magic
h.update(data)
args.device.write(data + h.digest() + struct.pack("<I", start))

args.device.close()
