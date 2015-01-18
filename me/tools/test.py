#!/usr/bin/python
#
# This script checks results of test.c for correctness.
# Usage:  ./test | ./test.py
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
import binascii
import hashlib
import hmac

def test_ripemd160(param):
    h = hashlib.new("ripemd160", param["Message"])
    return h.digest() == param["Digest"]

def test_sha256(param):
    return hashlib.sha256(param["Message"]).digest() == param["Digest"]

def test_sha512(param):
    return hashlib.sha512(param["Message"]).digest() == param["Digest"]

def test_hmac512(param):
    h = hmac.new(param["Key"], param["Message"], hashlib.sha512)
    return h.digest() == param["Hmac"]

tests = {
        "RIPEMD-160":   test_ripemd160,
        "SHA-256":      test_sha256,
        "SHA-512":      test_sha512,
        "HMAC/SHA-512": test_hmac512,
}

test_cnt = {}

result = True
param = {}
num = 0

for s in sys.stdin:
    s = s.strip()
    if "PASSED" in s or "FAILED" in s:
        num += 1
        if "FAILED" in s:
            result = False
        print s
        continue

    try:
        tag, value = s.split(":")
    except:
        if s in tests:
            num += 1
            test_cnt[s] = test_cnt.get(s, 0) + 1
            if not tests[s](param):
                print "Test", num, s, "failed."
                result = False
        param = {}
        continue

    if tag == "Mnemonic":
        continue
    value = value.split()
    data = ""
    length = int(value[0])
    if length:
        data = binascii.unhexlify(value[1])

    param[tag] = data

if result:
    for s in test_cnt:
        print s, "test PASSED (all", test_cnt[s], "tests)."
print "-----------------"
print "Test result:", ("FAIL", "PASS")[result]
print num, "tests."

sys.exit(not result)

# Below is Shamir's Secret Sharing test, which is currently disabled.

execfile("../../bip-sss/bip-sss.py")

gf = GF(0x11d)

result = True

def test_share(x, share, shares, key, thr, kind_net):
    share, m, keyid, kind, testnet = decode(share)
    compare("x", share[0], x)
    compare("threshold", m, thr)
    kind = kind.split(None, 1)
    compare("secret type", kind[1], "private key")
    kind = kind[0].lower()
    compare("secret kind", ("mainnet", "testnet")[testnet] + " " + kind, kind_net)

    shares.append(share)

    if len(shares) < m:
        return

    random.shuffle(shares[:-1])
    combined = combine(gf, shares[:m-1] + shares[-1:])
    combined = [ (0x80, 0xef)[testnet] ] + combined
    if kind[0] == "c":
        combined += [1]
    compare("key", base58check(combined), key, True)

def compare(what, a, b, show = False):
    global result
    if a != b:
        result = False
        print "Error:", what, "is %s, expected %s." % (str(a), str(b))
    elif show:
        print "OK:", what, a

for s in sys.stdin:
    try:
        tag, value = s.split(":")
    except:
        continue

    value = value.strip()

    if tag == "Key":
        key = value
        shares = []
        print "Key:", key
    elif tag == "Threshold":
        thr = int(value)
    elif tag == "Type":
        kind_net = value
    elif tag == "Share":
        x, share = value.split()
        test_share(int(x), share, shares, key, thr, kind_net)

print "\n-----------------"
print "Test result:", ("FAIL", "PASS")[result]
