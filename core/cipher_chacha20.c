#include "cipher_chacha20.h"
#include "crypto_primitives.h"
#include <string.h>

#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

#define QR(a, b, c, d) \
    a += b; d ^= a; d = ROTL32(d, 16); \
    c += d; b ^= c; b = ROTL32(b, 12); \
    a += b; d ^= a; d = ROTL32(d, 8);  \
    c += d; b ^= c; b = ROTL32(b, 7);

static uint32_t load32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
          ((uint32_t)p[1] << 8) |
          ((uint32_t)p[2] << 16) |
          ((uint32_t)p[3] << 24);
}

static void store32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void chacha20_init(chacha20_ctx *ctx, const uint8_t key[CHACHA20_KEY_SIZE],
                   const uint8_t nonce[CHACHA20_NONCE_SIZE], uint32_t counter) {
    if (!ctx || !key || !nonce) return;

    /* Constants: "expand 32-byte k" */
    ctx->state[0] = 0x61707865;
    ctx->state[1] = 0x3320646e;
    ctx->state[2] = 0x79622d32;
    ctx->state[3] = 0x6b206574;

    /* Key */
    for (int i = 0; i < 8; i++) {
        ctx->state[4 + i] = load32_le(&key[i * 4]);
    }

    /* Counter */
    ctx->state[12] = counter;

    /* Nonce */
    ctx->state[13] = load32_le(&nonce[0]);
    ctx->state[14] = load32_le(&nonce[4]);
    ctx->state[15] = load32_le(&nonce[8]);
}

static void chacha20_block(const uint32_t in_state[16], uint8_t out_block[64]) {
    uint32_t x[16];
    memcpy(x, in_state, sizeof(x));

    for (int i = 0; i < 10; i++) {
        /* Column round */
        QR(x[0], x[4], x[8],  x[12]);
        QR(x[1], x[5], x[9],  x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);

        /* Diagonal round */
        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8],  x[13]);
        QR(x[3], x[4], x[9],  x[14]);
    }

    for (int i = 0; i < 16; i++) {
        x[i] += in_state[i];
        store32_le(&out_block[i * 4], x[i]);
    }

    crypto_zeroize(x, sizeof(x));
}

void chacha20_xor(chacha20_ctx *ctx, const uint8_t *src, uint8_t *dst, size_t len) {
    if (!ctx || !dst || (!src && len > 0)) return;

    uint8_t key_stream[64];
    size_t offset = 0;

    while (offset < len) {
        chacha20_block(ctx->state, key_stream);
        ctx->state[12]++; /* Increment block counter */

        size_t chunk = (len - offset >= 64) ? 64 : (len - offset);
        for (size_t i = 0; i < chunk; i++) {
            dst[offset + i] = src[offset + i] ^ key_stream[i];
        }
        offset += chunk;
    }

    crypto_zeroize(key_stream, sizeof(key_stream));
}

void chacha20_crypt(const uint8_t key[CHACHA20_KEY_SIZE],
                    const uint8_t nonce[CHACHA20_NONCE_SIZE],
                    uint32_t counter,
                    const uint8_t *src, uint8_t *dst, size_t len) {
    chacha20_ctx ctx;
    chacha20_init(&ctx, key, nonce, counter);
    chacha20_xor(&ctx, src, dst, len);
    crypto_zeroize(&ctx, sizeof(ctx));
}
