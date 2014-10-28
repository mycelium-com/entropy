/*
 * SHA-512 hash and HMAC implementation.
 * sha.cpp - modified by Wei Dai from Steve Reid's public domain sha1.c
 * Steve Reid implemented SHA-1. Wei Dai implemented SHA-2.
 * Both are in the public domain.
 * Adapted for Mycelium Entropy by Mycelium SA, Luxembourg.
 * Mycelium SA have waived all copyright and related or neighbouring rights
 * to their changes to this file and placed these changes in public domain.
 */

#include <string.h>
#include <assert.h>
#include "endian.h"
#include "sha512.h"


enum {
    HASH_SIZE   = 64,
    BLOCK_SIZE  = 128,
    WORD_SIZE   = 8,
#if __BYTE_ORDER == __LITTLE_ENDIAN
    MASK        = WORD_SIZE - 1,
#else
    MASK        = 0,
#endif
};

static inline uint64_t rotrFixed(uint64_t x, unsigned int y)
{
    return (uint64_t)((x>>y) | (x<<(sizeof(uint64_t)*8-y)));
}
    
#define blk0(i) (W[i] = data[i])

#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) (y^((x^y)&(y^z)))

#define a(i) T[(0-i)&7]
#define b(i) T[(1-i)&7]
#define c(i) T[(2-i)&7]
#define d(i) T[(3-i)&7]
#define e(i) T[(4-i)&7]
#define f(i) T[(5-i)&7]
#define g(i) T[(6-i)&7]
#define h(i) T[(7-i)&7]

#define S0(x) (rotrFixed(x,28)^rotrFixed(x,34)^rotrFixed(x,39))
#define S1(x) (rotrFixed(x,14)^rotrFixed(x,18)^rotrFixed(x,41))
#define s0(x) (rotrFixed(x,1)^rotrFixed(x,8)^(x>>7))
#define s1(x) (rotrFixed(x,19)^rotrFixed(x,61)^(x>>6))

#define R(i,j,k) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+SHA512_K[k]+(j?blk2(i):blk0(i));\
        d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))


static inline void sha512_InitState(uint64_t *state)
{
    static const uint64_t s[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };
    memcpy(state, s, sizeof s);
}

static const uint64_t SHA512_K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

static void sha512_Transform(uint64_t *state, const uint64_t *data)
{
    uint64_t W[16];
    uint64_t T[8];
    unsigned i, k;

    memcpy(T, state, sizeof T);

    for (k = 0; k < 16; k++) {
        R(k, 0, k);
    }
    for (; k < 80; k++)
    {
        i = k & 15;
        R(i, 1, k);
    }

    state[0] += a(0);
    state[1] += b(0);
    state[2] += c(0);
    state[3] += d(0);
    state[4] += e(0);
    state[5] += f(0);
    state[6] += g(0);
    state[7] += h(0);
}

static void sha512_finalise(uint64_t *hash, const uint8_t *data, int len,
        int total_len)
{
    union {
        uint8_t  b[BLOCK_SIZE];
        uint64_t w[BLOCK_SIZE / sizeof (uint64_t)];
    } buf;
    int i;

    // copy data blocks in big endian and process them
    i = 0;
    while (i < len) {
        buf.b[i ^ MASK] = *data++;
        if (++i == BLOCK_SIZE) {
            len -= i;
            i = 0;
            sha512_Transform(hash, buf.w);
        }
    }

    // append bit '1', zero padding and length
    buf.b[i++ ^ MASK] = 0x80;
    while (i & (WORD_SIZE - 1))
        buf.b[i++ ^ MASK] = 0;  // align

    if (BLOCK_SIZE - i < WORD_SIZE * 2) {
        memset(buf.b + i, 0, BLOCK_SIZE - i);
        sha512_Transform(hash, buf.w);
        i = 0;
    }

    memset(buf.b + i, 0, BLOCK_SIZE - WORD_SIZE - i);
    buf.w[BLOCK_SIZE / WORD_SIZE - 1] = total_len * 8;
    sha512_Transform(hash, buf.w);

#if __BYTE_ORDER == __LITTLE_ENDIAN
    for (i = 0; i < HASH_SIZE / WORD_SIZE; i++)
        hash[i] = __builtin_bswap64(hash[i]);
#endif
}

void sha512_hash(uint64_t hash[8], const uint8_t *data, int len)
{
    sha512_InitState(hash);
    sha512_finalise(hash, data, len, len);
}

void sha512_hmac(uint64_t hash[8],
                 const uint8_t *key, int key_len,
                 const uint8_t *data, int data_len)
{
    struct {
        union {
            uint8_t  b[BLOCK_SIZE];
            uint64_t w[BLOCK_SIZE / sizeof (uint64_t)];
            uint32_t w32[BLOCK_SIZE / sizeof (uint32_t)];
        } key;
        union {
            uint8_t  b[HASH_SIZE];
            uint64_t w[HASH_SIZE / sizeof (uint64_t)];
        } hash;
    } buf;
    int i;

    // caller must hash the key if it is longer than BLOCK_SIZE
    assert(key_len <= BLOCK_SIZE);

    // copy key xor i_pad in big endian order
    for (i = 0; i != BLOCK_SIZE; i++)
        buf.key.b[i ^ MASK] = (i < key_len ? *key++ : 0) ^ 0x36;

    // first hash pass
    sha512_InitState(buf.hash.w);
    sha512_Transform(buf.hash.w, buf.key.w);
    sha512_finalise(buf.hash.w, data, data_len, sizeof buf.key + data_len);

    // replace i_pad with o_pad
    for (i = 0; i != BLOCK_SIZE / sizeof (uint32_t); i++)
        buf.key.w32[i] ^= 0x5C5C5C5C ^ 0x36363636;

    // second hash pass
    sha512_InitState(hash);
    sha512_Transform(hash, buf.key.w);
    sha512_finalise(hash, buf.hash.b, sizeof buf.hash, sizeof buf);
}
