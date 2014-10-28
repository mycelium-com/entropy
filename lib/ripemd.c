/*
 * RIPEMD-160 written and placed in the public domain by Wei Dai
 * RIPEMD-320, RIPEMD-128, RIPEMD-256 written by Kevin Springle
 * and also placed in the public domain
 * Adapted for Mycelium Entropy by Mycelium SA, Luxembourg.
 * Mycelium SA have waived all copyright and related or neighbouring rights
 * to their changes to this file and placed these changes in public domain.
 */

#include <string.h>
#include "endian.h"
#include "ripemd.h"


static void ripemd160_init(uint32_t state[5])
{
    static const uint32_t init_value[] = {
        0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
    };
    memcpy(state, init_value, sizeof init_value);
}


#define k0 0
#define k1 0x5a827999UL
#define k2 0x6ed9eba1UL
#define k3 0x8f1bbcdcUL
#define k4 0xa953fd4eUL
#define k5 0x50a28be6UL
#define k6 0x5c4dd124UL
#define k7 0x6d703ef3UL
#define k8 0x7a6d76e9UL
#define k9 0


static inline uint32_t rotlFixed(uint32_t x, uint32_t y)
{
    return (uint32_t)((x<<y) | (x>>(sizeof(uint32_t)*8-y)));
}

static void Subround(
        uint32_t X[16],
        unsigned long f,
        unsigned long *a, unsigned long b, unsigned long *c, unsigned long d,
        unsigned long e)
{
    static const uint8_t xidx[160] = {
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
         7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8,
         3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12,
         1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2,
         4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13,
         5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12,
         6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2,
        15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13,
         8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14,
        12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11,
    };
    static const uint8_t s[160] = {
        11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8,
         7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12,
        11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5,
        11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12,
         9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6,
         8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6,
         9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11,
         9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5,
        15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8,
         8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11,
    };
    static const unsigned long k[10] = {
        k0, k1, k2, k3, k4, k5, k6, k7, k8, k9
    };

    switch (f >> 4) {
    case 0:
    case 9:
        *a += (b ^ *c ^ d);
        break;
    case 1:
    case 8:
        *a += (d ^ (b & (*c^d)));
        break;
    case 2:
    case 7:
        *a += (d ^ (b | ~*c));
        break;
    case 3:
    case 6:
        *a += (*c ^ (d & (b^*c)));
        break;
    default:
        *a += (b ^ (*c | ~d));
    }
    *a += X[xidx[f]] + k[f >> 4];
    *a = rotlFixed((uint32_t)*a, s[f]) + e;
    *c = rotlFixed((uint32_t)*c, 10U);
}

static void ripemd160_transform(uint32_t digest[5], const uint32_t Y[16])
{
    int i;
    uint32_t X[16];

    for (i = 0; i < 16; i++)
        X[i] = le32_to_cpu(Y[i]);

    unsigned long a1, b1, c1, d1, e1, a2, b2, c2, d2, e2;
    a1 = a2 = digest[0];
    b1 = b2 = digest[1];
    c1 = c2 = digest[2];
    d1 = d2 = digest[3];
    e1 = e2 = digest[4];
    i = 0;
    do {
        Subround(X, i++, &a1, b1, &c1, d1, e1);
        Subround(X, i++, &e1, a1, &b1, c1, d1);
        Subround(X, i++, &d1, e1, &a1, b1, c1);
        Subround(X, i++, &c1, d1, &e1, a1, b1);
        Subround(X, i++, &b1, c1, &d1, e1, a1);
    } while (i != 80);
    do {
        Subround(X, i++, &a2, b2, &c2, d2, e2);
        Subround(X, i++, &e2, a2, &b2, c2, d2);
        Subround(X, i++, &d2, e2, &a2, b2, c2);
        Subround(X, i++, &c2, d2, &e2, a2, b2);
        Subround(X, i++, &b2, c2, &d2, e2, a2);
    } while (i != 160);

    c1        = digest[1] + c1 + d2;
    digest[1] = digest[2] + d1 + e2;
    digest[2] = digest[3] + e1 + a2;
    digest[3] = digest[4] + a1 + b2;
    digest[4] = digest[0] + b1 + c2;
    digest[0] = c1;
}

#undef Subround


void ripemd160_hash(uint32_t hash[5], const uint8_t *data, int len)
{
    ripemd160_init(hash);
    union {
        uint8_t  b[64];
        uint32_t w[64 / 4];
    } buf;
    int i;
    for (i = 0; i + (int) sizeof buf <= len; i += sizeof buf ) {
        memcpy(buf.b, data + i, sizeof buf);
        ripemd160_transform(hash, buf.w);
    }
    int l = len - i;
    memcpy(buf.b, data + i, l);
    buf.b[l] = 0x80;
    if (l > 55) {
        for (i = l + 1; i != sizeof buf; i++)
            buf.b[i] = 0;
        ripemd160_transform(hash, buf.w);
        l = 0;
        buf.b[l] = 0;
    }

    for (i = l + 1; i <= 55; i++)
        buf.b[i] = 0;
    buf.w[14] = cpu_to_le32(len * 8);   // bit length
    buf.w[15] = 0;                      // high 4 bytes of bit length

    ripemd160_transform(hash, buf.w);

#if __BYTE_ORDER == __BIG_ENDIAN
    for (i = 0; i < 5; i++)
        hash[i] = cpu_to_le32(hash[i]);
#endif
}
