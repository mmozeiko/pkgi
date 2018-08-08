#include "sha256.h"

#include <string.h>

#if __ARM_NEON__

#include <arm_neon.h>

// Optimized SHA-256 Neon implementation is based on following whitepaper from
// Intel: "Fast SHA-256 Implementations on Intel(R) Architecture Processors"
// https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/sha-256-implementations-paper.pdf

// It is ~2x faster on PlayStation Vita - ~23 MB/s

#endif

static const uint32_t sha256_K[64] GCC_ALIGN(16) = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z)
{
    return z ^ (x & (y ^ z));
}

static inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z)
{
    return ((x | y) & z) | (x & y);
}

static inline uint32_t Sigma0(uint32_t x)
{
    return ror32(x, 2) ^ ror32(x, 13) ^ ror32(x, 22);
}

static inline uint32_t Sigma1(uint32_t x)
{
    return ror32(x, 6) ^ ror32(x, 11) ^ ror32(x, 25);
}

static inline uint32_t Gamma0(uint32_t x)
{
    return ror32(x, 7) ^ ror32(x, 18) ^ (x >> 3);
}

static inline uint32_t Gamma1(uint32_t x)
{
    return ror32(x, 17) ^ ror32(x, 19) ^ (x >> 10);
}

#define ROUND(tmp, a, b, c, d, e, f, g, h) \
    do                                     \
    {                                      \
        uint32_t t = tmp;                  \
        t += h + Sigma1(e) + Ch(e, f, g);  \
        d += t;                            \
        t += Sigma0(a) + Maj(a, b, c);     \
        h = g;                             \
        g = f;                             \
        f = e;                             \
        e = d;                             \
        d = c;                             \
        c = b;                             \
        b = a;                             \
        a = t;                             \
    } while (0)

#if __ARM_NEON__

#define ROUNDx4(x, n, a, b, c, d, e, f, g, h)  \
    do                                         \
    {                                          \
        uint32x4_t tmp;                        \
        uint32_t arr[4];                       \
        tmp = vld1q_u32(sha256_K + n);         \
        tmp = vaddq_u32(tmp, x);               \
        vst1q_u32(arr, tmp);                   \
        ROUND(arr[0], a, b, c, d, e, f, g, h); \
        ROUND(arr[1], a, b, c, d, e, f, g, h); \
        ROUND(arr[2], a, b, c, d, e, f, g, h); \
        ROUND(arr[3], a, b, c, d, e, f, g, h); \
    } while (0)

#define PREPARE_NEXT()                         \
    do                                         \
    {                                          \
        uint32x4_t q0, q1, q2, q3, q4, q5;     \
        uint32x2_t d0, d1, d2, d3, d4, d5, d6; \
                                               \
        q0 = vextq_u32(x2, x3, 1);             \
        q0 = vaddq_u32(q0, x0);                \
                                               \
        q1 = vextq_u32(x0, x1, 1);             \
        q2 = vshrq_n_u32(q1, 7);               \
        q3 = vshlq_n_u32(q1, 32 - 7);          \
        q4 = vshrq_n_u32(q1, 18);              \
        q5 = vshlq_n_u32(q1, 32 - 18);         \
        q1 = vshrq_n_u32(q1, 3);               \
        q1 = veorq_u32(q1, q2);                \
        q2 = veorq_u32(q3, q4);                \
        q1 = veorq_u32(q1, q2);                \
        q1 = veorq_u32(q1, q5);                \
                                               \
        d0 = vget_high_u32(x3);                \
        d1 = vshr_n_u32(d0, 17);               \
        d2 = vshl_n_u32(d0, 32 - 17);          \
        d3 = vshr_n_u32(d0, 19);               \
        d4 = vshl_n_u32(d0, 32 - 19);          \
        d5 = vshr_n_u32(d0, 10);               \
        d0 = veor_u32(d1, d2);                 \
        d1 = veor_u32(d3, d4);                 \
        d0 = veor_u32(d0, d1);                 \
        d6 = veor_u32(d0, d5);                 \
                                               \
        d0 = vget_low_u32(q0);                 \
        d1 = vget_low_u32(q1);                 \
        d0 = vadd_u32(d0, d6);                 \
        d0 = vadd_u32(d0, d1);                 \
                                               \
        d1 = vshr_n_u32(d0, 17);               \
        d2 = vshl_n_u32(d0, 32 - 17);          \
        d3 = vshr_n_u32(d0, 19);               \
        d4 = vshl_n_u32(d0, 32 - 19);          \
        d5 = vshr_n_u32(d0, 10);               \
        d0 = veor_u32(d1, d2);                 \
        d1 = veor_u32(d3, d4);                 \
        d0 = veor_u32(d0, d1);                 \
        d0 = veor_u32(d0, d5);                 \
                                               \
        q0 = vaddq_u32(q0, q1);                \
        q1 = vcombine_u32(d6, d0);             \
        q0 = vaddq_u32(q0, q1);                \
                                               \
        x0 = x1;                               \
        x1 = x2;                               \
        x2 = x3;                               \
        x3 = q0;                               \
    } while (0)

static void sha256_process(
        uint32_t* state, const uint8_t* buffer, uint32_t blocks)
{
    for (uint32_t i = 0; i < blocks; i++)
    {
        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        uint32x4_t x0 =
                vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(buffer + 0 * 16)));
        uint32x4_t x1 =
                vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(buffer + 1 * 16)));
        uint32x4_t x2 =
                vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(buffer + 2 * 16)));
        uint32x4_t x3 =
                vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(buffer + 3 * 16)));
        buffer += 64;

        // rounds [0..47]
        for (uint32_t r = 0; r < 48; r += 16)
        {
            ROUNDx4(x0, r + 0, a, b, c, d, e, f, g, h);
            PREPARE_NEXT();

            ROUNDx4(x0, r + 4, a, b, c, d, e, f, g, h);
            PREPARE_NEXT();

            ROUNDx4(x0, r + 8, a, b, c, d, e, f, g, h);
            PREPARE_NEXT();

            ROUNDx4(x0, r + 12, a, b, c, d, e, f, g, h);
            PREPARE_NEXT();
        }

        // rounds [48..63]
        ROUNDx4(x0, 48, a, b, c, d, e, f, g, h);
        ROUNDx4(x1, 52, a, b, c, d, e, f, g, h);
        ROUNDx4(x2, 56, a, b, c, d, e, f, g, h);
        ROUNDx4(x3, 60, a, b, c, d, e, f, g, h);

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
}

#else

static void sha256_process(
        uint32_t* state, const uint8_t* buffer, uint32_t blocks)
{
    for (uint32_t i = 0; i < blocks; i++)
    {
        uint32_t w[64];
        for (uint32_t r = 0; r < 16; r++)
        {
            w[r] = get32be(buffer + 4 * r);
        }
        for (uint32_t r = 16; r < 64; r++)
        {
            w[r] = Gamma1(w[r - 2]) + Gamma0(w[r - 15]) + w[r - 7] + w[r - 16];
        }
        buffer += SHA256_BLOCK_SIZE;

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        for (uint32_t r = 0; r < 64; r++)
        {
            ROUND(sha256_K[r] + w[r], a, b, c, d, e, f, g, h);
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
}

#endif

void sha256_init(sha256_ctx* ctx)
{
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

void sha256_update(sha256_ctx* ctx, const uint8_t* buffer, uint32_t size)
{
    if (size == 0)
    {
        return;
    }

    uint32_t left = ctx->count % SHA256_BLOCK_SIZE;
    uint32_t fill = SHA256_BLOCK_SIZE - left;
    ctx->count += size;

    if (left && size >= fill)
    {
        memcpy(ctx->buffer + left, buffer, fill);
        sha256_process(ctx->state, ctx->buffer, 1);
        buffer += fill;
        size -= fill;
        left = 0;
    }

    uint32_t full = size / SHA256_BLOCK_SIZE;
    if (full != 0)
    {
        sha256_process(ctx->state, buffer, full);
        uint32_t used = full * SHA256_BLOCK_SIZE;
        buffer += used;
        size -= used;
    }

    memcpy(ctx->buffer + left, buffer, size);
}

void sha256_finish(sha256_ctx* ctx, uint8_t* digest)
{
    static const uint8_t padding[SHA256_BLOCK_SIZE] = {0x80};

    uint8_t bits[8];
    set64be(bits, ctx->count * 8);

    uint32_t last = ctx->count % SHA256_BLOCK_SIZE;
    uint32_t pad = (last < SHA256_BLOCK_SIZE - 8)
                           ? (SHA256_BLOCK_SIZE - 8 - last)
                           : (2 * SHA256_BLOCK_SIZE - 8 - last);

    sha256_update(ctx, padding, pad);
    sha256_update(ctx, bits, sizeof(bits));

    for (uint32_t i = 0; i < 8; i++)
    {
        set32be(digest + 4 * i, ctx->state[i]);
    }
}
