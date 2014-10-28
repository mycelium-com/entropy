/*
 * SHA-256 - modified by Wei Dai from Steve Reid's public domain sha1.c.
 * Steve Reid implemented SHA-1. Wei Dai implemented SHA-2.
 * Both are in the public domain.
 * Adapted for Mycelium Entropy by Mycelium SA, Luxembourg.
 * Mycelium SA have waived all copyright and related or neighbouring rights
 * to their changes to this file and placed these changes in public domain.
 */

#include <string.h>
#include "endian.h"
#include "sha256.h"

void sha256_init(uint32_t state[8])
{
    static const uint32_t s[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    memcpy(state, s, sizeof s);
}

static const uint32_t SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define SHA256_K_GET(i) SHA256_K[i]

static uint32_t rotrFixed(uint32_t x, unsigned int y)
{
    return (uint32_t)((x>>y) | (x<<(sizeof(uint32_t)*8-y)));
}

#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))

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

#define blk0(i) (W[i] = be32_to_cpu(data[i]))
#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define R(i) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+SHA256_K_GET(i+j)+(j?blk2(i):blk0(i));\
    d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))


// Transform one block and update state.
// State is in native byte order, data is big endian.
void sha256_transform(uint32_t state[8], const uint32_t data[16])
{
    uint32_t T[20];
    uint32_t W[32];
    unsigned int i = 0, j = 0;
    uint32_t *t = T+8;

    memcpy(t, state, 32);
    uint32_t e = t[4], a = t[0];

    do 
    {
        uint32_t w = be32_to_cpu(data[j]);
        W[j] = w;
        w += SHA256_K_GET(j);
        w += t[7];
        w += S1(e);
        w += Ch(e, t[5], t[6]);
        e = t[3] + w;
        t[3] = t[3+8] = e;
        w += S0(t[0]);
        a = w + Maj(a, t[1], t[2]);
        t[-1] = t[7] = a;
        --t;
        ++j;
        if (j%8 == 0)
            t += 8;
    } while (j<16);

    do
    {
        i = j & 0xf;
        uint32_t w = s1(W[i+16-2]) + s0(W[i+16-15]) + W[i] + W[i+16-7];
        W[i+16] = W[i] = w;
        w += SHA256_K_GET(j);
        w += t[7];
        w += S1(e);
        w += Ch(e, t[5], t[6]);
        e = t[3] + w;
        t[3] = t[3+8] = e;
        w += S0(t[0]);
        a = w + Maj(a, t[1], t[2]);
        t[-1] = t[7] = a;

        w = s1(W[(i+1)+16-2]) + s0(W[(i+1)+16-15]) + W[(i+1)] + W[(i+1)+16-7];
        W[(i+1)+16] = W[(i+1)] = w;
        w += SHA256_K_GET(j+1);
        w += (t-1)[7];
        w += S1(e);
        w += Ch(e, (t-1)[5], (t-1)[6]);
        e = (t-1)[3] + w;
        (t-1)[3] = (t-1)[3+8] = e;
        w += S0((t-1)[0]);
        a = w + Maj(a, (t-1)[1], (t-1)[2]);
        (t-1)[-1] = (t-1)[7] = a;

        t -= 2;
        j += 2;
        if (j % 8 == 0)
            t += 8;
    } while (j<64);

    state[0] += a;
    state[1] += t[1];
    state[2] += t[2];
    state[3] += t[3];
    state[4] += e;
    state[5] += t[5];
    state[6] += t[6];
    state[7] += t[7];
}

void sha256_hash(uint32_t hash[8], const uint8_t *data, int len)
{
    sha256_init(hash);
    sha256_finish(hash, data, len, len);
}

void sha256_twice(uint32_t hash[8], const uint8_t *data, int len)
{
    uint32_t hash_tmp[8];
    sha256_hash(hash_tmp, data, len);
    sha256_hash(hash, (uint8_t *)hash_tmp, sizeof hash_tmp);
}

// Update hash with the remaining portion of the message and finalise it.
// Total length of the entire message must be provided, as it will be appended
// at the end according to the specification.
// Input: hash[8] is the accumulated internal state in native byte order.
// Output: hash[8] is the resulting hash that can be considered a byte sequence
// (i.e. in big endian).
void sha256_finish(uint32_t hash[8], const uint8_t *data, int len, int total)
{
    union {
        uint8_t  b[SHA256_BLOCK_SIZE];      // 64 bytes
        uint32_t w[SHA256_BLOCK_SIZE / 4];
    } buf;
    int i;

    for (i = 0; i + (int) sizeof buf <= len; i += sizeof buf) {
        memcpy(buf.b, data + i, sizeof buf);
        sha256_transform(hash, buf.w);
    }

    int l = len - i;
    memcpy(buf.b, data + i, l);
    buf.b[l]=0x80;
    if (l > 55) {
        for (i = l + 1; i != sizeof buf; i++)
            buf.b[i]=0;
        sha256_transform(hash, buf.w);
        l = 0;
        buf.b[l] = 0;
    }

    for (i = l + 1; i <= 55; i++)
        buf.b[i]=0;
    buf.w[14] = 0; // high 4 bytes of bit length
    buf.w[15] = cpu_to_be32(total * 8);   // bit length

    sha256_transform(hash, buf.w);

#if __BYTE_ORDER == __LITTLE_ENDIAN
    for (i = 0; i < 8; i++)
        hash[i] = cpu_to_be32(hash[i]);
#endif
}


#undef S0
#undef S1
#undef s0
#undef s1
#undef R
