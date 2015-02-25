/*
 * Hierarchical Deterministic wallets with mnemonic seeds.
 * BIP-32 and BIP-39 subset implementation for Mycelium Entropy.
 *
 * Copyright 2015 Mycelium SA, Luxembourg.
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
#include <string.h>

#include "lib/sha256.h"
#include "lib/sha512.h"
#include "lib/pbkdf2.h"
#include "lib/ripemd.h"
#include "lib/base58.h"
#include "lib/ecdsa.h"
#include "settings.h"
#include "data.h"
#include "layout.h"
#include "xflash.h"
#include "keygen.h"
#include "hd.h"

// We can temporarily use the memory at _estack for the word list.
extern uint8_t _estack[];

// Hash of the genuine BIP-39 US English word list.
static const uint8_t bip39_wordlist_hash[] = {
    0x19, 0xa7, 0x18, 0x88, 0x00, 0x94, 0x11, 0x2c,
    0x8a, 0x3e, 0xd5, 0x1c, 0x5c, 0x8c, 0xba, 0xb8,
    0x42, 0xdc, 0x61, 0xbb, 0x7c, 0x15, 0x25, 0x28,
    0xc6, 0xf7, 0xd7, 0x00, 0x61, 0x98, 0xf9, 0x7c
};

bool hd_gen_seed_with_mnemonic(int ent, uint64_t seed[8], char *mnemonic)
{
    union {
        uint8_t  b[33];
        uint32_t w[8];
    } rnum;

    union {
        uint8_t  cs;
        uint32_t hash[8];
    } checksum;

    // load wordlist from serial flash
    uint8_t *words = _estack;
    xflash_read(words, 2048 * 5, xflash_num_blocks * 512 - XFLASH_WORDLIST_OFFSET);
    // check if it's genuine
    sha256_hash(checksum.hash, words, 2048 * 5);
    if (memcmp(checksum.hash, bip39_wordlist_hash, sizeof bip39_wordlist_hash) != 0) {
        global_error_flags |= NO_WORDLIST;
        return false;
    }
    puts("Wordlist check OK.");

    // generate a (possibly salted) random number; we use the first 'ent' bytes
    keygen_get_entropy(rnum.w);
    // append BIP-39 checksum
    sha256_hash(checksum.hash, rnum.b, ent);
    rnum.b[ent] = checksum.cs;

    int n = ent / 4 * 3;        // number of words
    const uint8_t *r = rnum.b;  // input pointer: random number
    char *m = mnemonic;         // output pointer: mnemonic
    uint32_t rbuf = 0;          // to silence -Wmaybe-uninitialized
    int rbits = 0;

    do {
        rbuf = rbuf << 8 | *r++;
        if (rbits < 3) {
            rbuf = rbuf << 8 | *r++;
            rbits += 8;
        }
        const uint8_t *word = words + (rbuf >> (rbits -= 3) & 0x7ff) * 5;
        uint32_t mbuf = 0;
        int mbits = 0;
        do {
            if (mbits < 5) {
                mbuf |= *word++ << mbits;
                mbits += 8;
            }
            mbits -= 5;
            if (!mbuf)
                break;
            *m++ = '`' + (mbuf & 0x1f);
            mbuf >>= 5;
        } while (mbits);
        *m++ = ' ';
    } while (--n);

    *--m = 0;

    printf("Mnemonic: %s.\n", mnemonic);

    // from mnemonic to seed: PBKDF2 with
    //  - mnemonic sentence as the password
    //  - string "mnemonic" + passphrase (empty) as the salt
    //  - 2048 iterations
    // mnemonics longer than 128 bytes are not currently supported
#ifdef TESTING
    pbkdf2_512(seed, mnemonic, "mnemonicTREZOR", 2048);
#else
    pbkdf2_512(seed, mnemonic, "mnemonic", 2048);
#endif

    puts("Seed ready.");

    return true;
}

#define HARDENED    0x80000000ul

// Generate xpub for the HD node at settings.hd_path.
// If hd_path is empty, set it to BIP-44 Account 0: m/44'/coin'/0'.
// Return false if either:
//  - hd_path is incorrect; or
//  - the seed or one of the keys on the path is invalid.
// Since the latter is extremely unlikely (less than 1 in 2**123),
// the two errors are combined.  Invalid seed will be indicated like
// an incorrect hd_path.
bool hd_make_xpub(const uint8_t *seed, int len)
{
    static const uint8_t xpub_version[3][4] = { // version bytes for serialisation
        { 0x04, 0x88, 0xB2, 0x1E }, // mainnet
        { 0x04, 0x35, 0x87, 0xCF }, // testnet
        { 0x01, 0x9d, 0xa4, 0x62 }, // Litecoin
    };
    union {
        struct {                    // buffer for input to HMAC-SHA512
            uint8_t  unused[3];
            uint8_t  first_byte;    // 0x00 if private, 0x02/03 if public
            uint8_t  parent[32];    // private or public key for child derivation
            uint32_t index_be;      // child node's index in big endian
        };
        uint32_t aux[5];            // auxiliary buffer for RIPE-MD
    } buf;
    union {
        uint64_t hash[8];           // result of HMAC-SHA512
        struct {
            uint8_t kadd[32];       // material for private key
            uint8_t chain[32];      // chain code
        };
        struct {                    // buffer for building xpub key
            uint8_t  unused[19];
            uint8_t  version[4];
            uint8_t  depth;
            uint32_t fingerprint;   // fingerprint of the parent's public key
            uint32_t child_num_be;  // child number
            uint8_t  chain[32];     // chain code
            uint8_t  key_prefix;    // 2 or 3
            uint8_t  x[32];         // public key's x
        } xpub;
        uint32_t aux[8];            // auxiliary buffer for SHA-256
    } node;
    bignum256 priv;                 // current node's private key
    bignum256 add;                  // additional material from HMAC
    curve_point pub;                // current node's public key (when needed)

    char *p = settings.hd_path;
    uint32_t index = 0;
    int depth = 0;

    // make BIP-32 master node
    sha512_hmac(node.hash, (const uint8_t *) "Bitcoin seed", 12, seed, len);
    bn_read_be(node.kadd, &priv);
    if (!bn_is_less(&priv, &order256k1) || bn_is_zero(&priv))
        return false;               // invalid seed (extremely unlikely)

    // if settings.hd_path is not provided, make it BIP-44 Account 0
    if (!*p)
        sprintf(p, "m/44'/%u'/0'", settings.coin.bip44);

    // derive extended public key for the node at path p

    if (*p != 'm')
        return false;   // path must start at the master node
    if (*++p) {
        bool number_seen = false;
        char c;

        if (*p != '/')
            return false;

        do {
            switch (c = *++p) {
            case '0' ... '9':
                if (!number_seen)
                    index = 0;
                number_seen = true;
                uint32_t next = index * 10 + (c - '0');
                if (next < index || (index & HARDENED))
                    return false;   // overflow
                index = next;
                continue;

            case '\'':
                if (!number_seen || (index & HARDENED))
                    return false;   // illegal hardening (')
                index |= HARDENED;
                continue;

            case '/':
                if (!number_seen)
                    return false;   // illegal path separator
                break;

            case '\0':
                break;

            default:
                return false;       // invalid character
            }

            if (!number_seen)
                continue;

            // CKDpriv
            buf.index_be = cpu_to_be32(index);
            if (index & HARDENED) {
                buf.first_byte = 0;
                bn_write_be(&priv, buf.parent);
            } else {
                scalar_multiply(&priv, &pub);
                buf.first_byte = 0x02 | (pub.y.val[0] & 1);
                bn_write_be(&pub.x, buf.parent);
            }
            sha512_hmac(node.hash, node.chain, sizeof node.chain, &buf.first_byte, 37);
            bn_read_be(node.kadd, &add);
            bn_addmod(&priv, &add, &order256k1);
            // check validity
            if (!bn_is_less(&add, &order256k1) || bn_is_zero(&priv))
                return false;   // invalid key (probability lower than 1 in 2**127)

            number_seen = false;
            depth++;
        } while (c);
    }

    // serialise extended public key

    // compute fingerprint of the parent's public key (0 if no parent)
    if (depth == 0) {
        node.xpub.fingerprint = 0;
    } else {
        if (index & HARDENED) {
            // last derivation was hardened; buf contains the parent's private key;
            // convert it to public
            bignum256 parent;
            bn_read_be(buf.parent, &parent);
            scalar_multiply(&parent, &pub);
            buf.first_byte = 0x02 | (pub.y.val[0] & 1);
            bn_write_be(&pub.x, buf.parent);
        }
        sha256_hash(node.aux, &buf.first_byte, 33);
        ripemd160_hash(buf.aux, (uint8_t *) node.aux, SHA256_SIZE);
        node.xpub.fingerprint = buf.aux[0];
    }

    // fill other fields
    node.xpub.depth = depth;
    node.xpub.child_num_be = cpu_to_be32(index);
    uint8_t xpub = settings.coin.bip44;     // index of the xpub magic prefix
    if (xpub > sizeof xpub_version / sizeof xpub_version[0])
        xpub = 0;                           // use normal BIP-32 xpub by default
    memcpy(node.xpub.version, xpub_version[xpub], 4);

    // compute the current node's public key and serialise it into xpub buffer
    scalar_multiply(&priv, &pub);
    node.xpub.key_prefix = 0x02 | (pub.y.val[0] & 1);
    bn_write_be(&pub.x, node.xpub.x);

    // encode
    base58check_encode(node.xpub.version, 78, texts[IDX_XPUB]);
    return true;
}
