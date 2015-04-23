#!/usr/bin/python
#
# Firmware signing tool.
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
import os
import os.path
import argparse
import subprocess
import struct
import binascii
import hashlib
import random
import ecdsa

CPP = "arm-none-eabi-cpp"

# Mycelium keys
devkey  = ("2d52dce0ee225f14cff621edc3e9e8e9d728633b6a111f02cfe8ce3034467f7c",
           "ca636bc96ca7c233a1b9e0cd54684d92619e6ece1270402139657192cdaadfdc")
mainkey = ("ad787b1f17077f232af7b3b7c0a1e5e07c21a2a4a76d401931133461bf26b917",
           "c3072723bd54a28a651c7d0e04ee342b2fa53295c239ef188a39ed407a84fe1b")

def tohex(data):
    return " ".join([ binascii.hexlify(data[i:i+4]) for i in range(0, len(data), 4) ])

class Signature(dict):
    fmt = "HHBBBBI16s16s32s"
    fields = ("magic", "version_code", "flavour_code", "hardware_family",
              "mcu", "xflash", "xflash_size", "reserved1", "version",
              "flavour", "hardware")

    def __init__(self):
        self["reserved1"] = 0

    def pack_ver(self, image, offset):
        values = [ self[field] for field in self.fields ]
        image[offset : offset+80] = struct.pack("<I" + self.fmt, *values)
        self.h = hashlib.sha256(image[:offset+96]).digest()

    def pack_sig(self, image, offset):
        image[offset+96 :offset+128] = self.h
        image[offset+128:offset+160] = binascii.unhexlify(self["x"])
        image[offset+160:offset+192] = binascii.unhexlify(self["y"])
        image[offset+192:offset+224] = binascii.unhexlify(self["r"])
        image[offset+224:offset+256] = binascii.unhexlify(self["s"])

    def unpack(self, image, offset):
        values = struct.unpack("<4s" + self.fmt, image[offset : offset+80])
        for i in range(len(values)):
            if isinstance(values[i], str):
                self[self.fields[i]] = values[i].split('\0', 1)[0]
            else:
                self[self.fields[i]] = values[i]
        self.h = image[offset+96 :offset+128]
        self.x = image[offset+128:offset+160]
        self.y = image[offset+160:offset+192]
        self.r = image[offset+192:offset+224]
        self.s = image[offset+224:offset+256]

    def check(self, image, offset):
        x = binascii.hexlify(self.x)
        y = binascii.hexlify(self.y)
        pubkey = ecdsa.Point(ecdsa.curve_256, long(x, 16), long(y, 16))
        pubkey = ecdsa.Public_key(ecdsa.g, pubkey)
        h = hashlib.sha256(image[:offset+96]).digest()
        hh = long(binascii.hexlify(self.h), 16)
        signature = ecdsa.Signature(long(binascii.hexlify(self.r), 16),
                                    long(binascii.hexlify(self.s), 16))
        if (x, y) == devkey:
            keyname = "Mycelium development key"
        elif (x, y) == mainkey:
            keyname = "Mycelium firmware signing key"
        else:
            keyname = "unknown key"
        return h == self.h, pubkey.verifies(hh, signature), keyname

    def show(self, image, offset):
        ver = self["version"]
        if self["flavour"]:
            ver += " (" + self["flavour"] + ")"
        print "Magic:        ", self["magic"]
        print "Version:      ", ver
        print "Hardware:     ", self["hardware"]
        print "Version-Code: ", self["version_code"], self["flavour_code"]
        print "Hardware-Code:", "%02X%02X%02X%02X" % (self["hardware_family"],
                self["mcu"], self["xflash"], self["xflash_size"])
        print "Hash:         ", tohex(self.h[:16])
        hash_ok, sig_ok, keyname = self.check(image, offset)
        print "              ", tohex(self.h[16:]), ("(WRONG)", "(correct)")[hash_ok]
        if sig_ok:
            status = ("(bad hash)", "(valid)")[hash_ok]
        else:
            status = "(INVALID)"
        print "Signed-By:    ", tohex(self.x[:16]), status
        print "              ", keyname

class KeySigner:
    def __init__(self, privkey):
        self.pubkey = ecdsa.Public_key(ecdsa.g, ecdsa.g * privkey)
        self.privkey = ecdsa.Private_key(self.pubkey, privkey)

    def sign(self, sig):
        k = random.SystemRandom().randrange(1, ecdsa.g.order())
        hh = long(binascii.hexlify(sig.h), 16)
        signature = self.privkey.sign(hh, k)
        sig["x"] = "%064x" % self.pubkey.point.x()
        sig["y"] = "%064x" % self.pubkey.point.y()
        sig["r"] = "%064x" % signature.r
        sig["s"] = "%064x" % signature.s

    def pubkey_hex(self):
        return [ " ".join([ h[i:i+8] for i in range(0, len(h), 8) ]) for h in
                 ("%064x" % self.pubkey.point.x(), "%064x" % self.pubkey.point.y()) ]

class ExternalSigner:
    def __init__(self):
        import serial
        from serial.tools import list_ports

        ports = list(list_ports.grep("EDBG|UART|Mycelium"))
        if not ports:
            print "Signing device is not connected."
            sys.exit(2)
        if len(ports) > 1:
            print "Ambiguous port."
            sys.exit(2)
        self.port = serial.Serial(ports[0][0], 115200, timeout=10)
        self.port.write('\r')
        if "enter hash" not in str(self.port.readline()) \
                and "enter hash" not in str(self.port.readline()) \
                and "enter hash" not in str(self.port.readline()):
            print "Cannot connect to signing device."
            sys.exit(2)

    def sign(self, sig):
        # someone somewhere can't cope with 65 bytes at once
        self.port.write(binascii.hexlify(sig.h[:16]))
        self.port.write(binascii.hexlify(sig.h[16:]) + '\r')
        while 's' not in sig:
            l = self.port.readline().strip()
            sys.stderr.write(l + '\n')
            if l[:5] != "Error":
                l = l.split(':')
                if len(l) == 2:
                    sig[l[0]] = l[1].translate(None, " ")

        if sig.h != binascii.unhexlify(sig["Hash"]):
            sys.stderr.write("Hash mismatch.\n")
            sys.exit(3)

def get_signer(keycfg):
    signer = None
    try:
        rc = open(keycfg)
        for l in rc:
            l = l.split(None, 1)
            if not l:
                continue
            if l[0] == "external":
                signer = ExternalSigner()
                break
            if l[0] == "key":
                if len(l) > 1:
                    keyhex = l[1].strip().translate(None, " ")
                    key = long(keyhex, 16)
                signer = KeySigner(key)
                break
    except IOError:
        key = random.SystemRandom().randrange(1, ecdsa.g.order())
        keyhex = "%064x" % key
        os.umask(0077)
        rc = open(keycfg, "w")
        signer = KeySigner(key)
        x, y = signer.pubkey_hex()
        rc.write("""# Firmware signing key configuration file.
# This file was automatically generated for you with a new key,
# but you can use your own key if you like.
#
# If you use the automatically generated key, below is the corresponding
# public key, which you should add to the settings.txt file on your
# Mycelium Entropy device with the 'sign' keyword as follows:
# sign """ + x + "\n#      " + y + """
#
# You can either provide the key directly with the 'key' statement,
# or specify that a special hardware device is to be used with the
# 'external' statement.

#external
key """ + " ".join([ keyhex[i:i+8] for i in range(0, len(keyhex), 8) ]) + "\n")
        print os.path.normpath(keycfg), "created with a new key."

    rc.close()
    if not signer:
        print "Bad config."
        sys.exit(4)
    return signer

def load_version(dirname):
    version_h = os.path.join(dirname, "..", "version.h")
    fwsign_h = os.path.join(dirname, "..", "..", "lib", "fwsign.h")
    # duplicate backslashes if under Windows
    version_h = repr(version_h)[1:-1]
    fwsign_h = repr(fwsign_h)[1:-1]
    vsrc = '#include "%s"\n#include "%s"' % (fwsign_h, version_h)
    vsrc += """
#ifndef FLAVOUR
#define FLAVOUR ""
#endif
magic           = MAGIC
version         = VERSION
version_code    = VERSION_CODE
flavour_code    = FLAVOUR_CODE
xflash          = XFLASH
xflash_size     = XFLASH_SIZE
flavour         = FLAVOUR
"""
    popen = subprocess.Popen(CPP, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    output, _ = popen.communicate(vsrc)
    if popen.returncode != 0:
        print "Error: cpp exited with code %d." % popen.returncode
        sys.exit(1)

    sig = Signature()

    for l in output.splitlines():
        l = l.strip()
        if not l or l[0] == '#':
            continue
        l = l.split("/*")[0].split("//")[0].split(",")[0]
        if '=' in l and len(l.split('=')[0].split()) == 1:
            exec l in globals(), sig

    return sig

def update_sig(fname, sig):
    platform = os.path.basename(os.path.dirname(os.path.realpath(fname)))
    sig["hardware_family"], sig["hardware"] = {
        "xpro-kit":     (sig["HW_FAMILY_XPRO"], "SAM4L Xplained Pro"),
        "entropy-1.0":  (sig["HW_FAMILY_ME1"],  "Mycelium Entropy 1.0"),
    }[platform]
    flen = os.stat(fname).st_size + 256
    if flen <= 112 * 1024:
        sig["mcu"] = sig["HW_SAM4L2"]
    else:
        sig["mcu"] = sig["HW_SAM4L4"]

def sign(fname, just_check, force):
    dirname = os.path.dirname(fname)
    image_file = open(fname, ("rb+", "rb")[just_check])
    image = image_file.read()
    sp = struct.unpack("<I", image[:4])[0]
    if sp < 0x20000000 or sp >= 0x20008000:
        print fname + ": not a Mycelium Entropy image"
        return False

    offset = struct.unpack("<I", image[32:36])[0]

    if offset:
        print fname + ": already signed"
        sig = Signature()
        sig.unpack(image, offset)
        sig.show(image, offset)
        if not force:
            return True
        print fname + ": removing existing signature"
        image = image[:offset]
        image_file.seek(offset)
        image_file.truncate(offset)
    elif just_check:
        print fname + ": not signed"
        return True

    print fname + ": signing..."

    offset = (len(image) + 3) & ~3
    simg = bytearray(offset + 256)
    simg[:len(image)] = image
    simg[32:36] = struct.pack("<I", offset)
    sig = load_version(dirname)
    update_sig(fname, sig)
    sig.pack_ver(simg, offset)
    signer = get_signer(os.path.join(dirname, "..", "..", "key.cfg"))
    signer.sign(sig)
    sig.pack_sig(simg, offset)

    # just a check
    sig = Signature()
    sig.unpack(simg, offset)
    hash_ok, sig_ok, keyname = sig.check(simg, offset)
    if not (hash_ok and sig_ok):
        sys.stderr.write("Signature does not verify.\n")
        sys.exit(1)

    image_file.write(simg[len(image):])
    image_file.seek(32)
    image_file.write(simg[32:36])
    image_file.close()
    print "Done."

ap = argparse.ArgumentParser(description="Sign a firmware image or task.")
ap.add_argument("fname", metavar="image.{bin|tsk}", nargs="+")
cfgroup = ap.add_mutually_exclusive_group()
cfgroup.add_argument("-c", "--check", action="store_true", help="check image signature")
cfgroup.add_argument("-f", "--force", action="store_true", help="force signature replacement")

args = ap.parse_args()
for fname in args.fname:
    sign(fname, args.check, args.force)
