#
# Mycelium Entropy tools
#
# The author has waived all copyright and related or neighbouring rights
# to this file and placed it in public domain.

CC = gcc
CFLAGS = -I.. -I../.. -I$(ASF)/thirdparty/fatfs/fatfs-r0.09/src \
	-I../../platforms/entropy-1.0 \
	-O2 -pipe -D_CONF_ACCESS_H_ -DAT25DFX_MEM=0 -DTESTING

SRC = ../sss.c ../keygen.c ../../lib/base58enc.c ../../lib/sha512.c \
	../../lib/bignum.c ../../lib/secp256k1.c ../../lib/ecdsa.c \
	../../lib/sha256.c ../../lib/ripemd.c ../../lib/rs-enc.c \
	../../lib/pbkdf2.c ../../lib/hex.c ../data.c ../hd.c stubs.c

check: check.c ../jpeg.c ../layout.c ../qr.c ../jpeg-data.c ../jpeg-data-ext.c \
	$(SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: test.c ../settings.c ../../lib/xxtea.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run-check: check
	./$<
	./$< -s
	./$< -l
	./$< -l -s
	./$< -1
	./$< -1 -s
	./$< -1 -l
	./$< -1 -l -s
	./$< -dd
	./$< -dd -l
	./$< -dd -t
	./$< -dd -1
	./$< -dd -1 -l

run-test: test test.py
	./$< | ./test.py

clean:
	rm -f check test

.PHONY: clean
