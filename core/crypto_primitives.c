#include "crypto_primitives.h"
#include <string.h>

void crypto_zeroize(void *ptr, size_t len) {
    if (!ptr || len == 0) return;
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) {
        *p++ = 0;
    }
}

int crypto_constant_time_memcmp(const void *a, const void *b, size_t len) {
    const volatile uint8_t *pa = (const volatile uint8_t *)a;
    const volatile uint8_t *pb = (const volatile uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (pa[i] ^ pb[i]);
    }
    return (int)diff;
}

/* SHA-256 Implementation (FIPS 180-4) */
static const uint32_t K256[64] = {
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

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)   (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)   (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static void sha256_transform(sha256_ctx *ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t w[64];
    uint32_t t1, t2;

    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (int i = 0; i < 64; i++) {
        t1 = h + SIGMA1(e) + CH(e, f, g) + K256[i] + w[i];
        t2 = SIGMA0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;

    crypto_zeroize(w, sizeof(w));
}

void sha256_init(sha256_ctx *ctx) {
    if (!ctx) return;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data || len == 0) return;
    size_t buffer_idx = (size_t)(ctx->count & 0x3F);
    ctx->count += len;

    size_t part_len = 64 - buffer_idx;
    size_t i = 0;

    if (buffer_idx > 0 && len >= part_len) {
        memcpy(&ctx->buffer[buffer_idx], data, part_len);
        sha256_transform(ctx, ctx->buffer);
        i = part_len;
        buffer_idx = 0;
    }

    for (; i + 64 <= len; i += 64) {
        sha256_transform(ctx, &data[i]);
    }

    if (i < len) {
        memcpy(&ctx->buffer[buffer_idx], &data[i], len - i);
    }
}

void sha256_final(sha256_ctx *ctx, uint8_t digest[SHA256_DIGEST_SIZE]) {
    if (!ctx || !digest) return;
    uint8_t final_count[8];
    uint64_t bits = ctx->count * 8;

    for (int i = 0; i < 8; i++) {
        final_count[7 - i] = (uint8_t)(bits >> (i * 8));
    }

    size_t buffer_idx = (size_t)(ctx->count & 0x3F);
    size_t pad_len = (buffer_idx < 56) ? (56 - buffer_idx) : (120 - buffer_idx);

    static const uint8_t padding[64] = {
        0x80, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
    };

    sha256_update(ctx, padding, pad_len);
    sha256_update(ctx, final_count, 8);

    for (int i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }

    crypto_zeroize(ctx, sizeof(sha256_ctx));
}

void sha256_hash(const uint8_t *data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]) {
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, digest);
}

/* HMAC-SHA256 Implementation (RFC 2104) */
void hmac_sha256_init(hmac_sha256_ctx *ctx, const uint8_t *key, size_t key_len) {
    if (!ctx) return;
    uint8_t k_block[64];
    memset(k_block, 0, sizeof(k_block));

    if (key_len > 64) {
        sha256_hash(key, key_len, k_block);
    } else if (key && key_len > 0) {
        memcpy(k_block, key, key_len);
    }

    uint8_t ipad[64];
    uint8_t opad[64];
    for (int i = 0; i < 64; i++) {
        ipad[i] = k_block[i] ^ 0x36;
        opad[i] = k_block[i] ^ 0x5C;
    }

    sha256_init(&ctx->inner);
    sha256_update(&ctx->inner, ipad, 64);

    sha256_init(&ctx->outer);
    sha256_update(&ctx->outer, opad, 64);

    crypto_zeroize(k_block, sizeof(k_block));
    crypto_zeroize(ipad, sizeof(ipad));
    crypto_zeroize(opad, sizeof(opad));
}

void hmac_sha256_update(hmac_sha256_ctx *ctx, const uint8_t *data, size_t len) {
    if (!ctx) return;
    sha256_update(&ctx->inner, data, len);
}

void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE]) {
    if (!ctx || !out) return;
    uint8_t inner_hash[SHA256_DIGEST_SIZE];
    sha256_final(&ctx->inner, inner_hash);

    sha256_update(&ctx->outer, inner_hash, SHA256_DIGEST_SIZE);
    sha256_final(&ctx->outer, out);

    crypto_zeroize(inner_hash, sizeof(inner_hash));
    crypto_zeroize(ctx, sizeof(hmac_sha256_ctx));
}

void hmac_sha256_calc(const uint8_t *key, size_t key_len,
                      const uint8_t *data, size_t data_len,
                      uint8_t out[SHA256_DIGEST_SIZE]) {
    hmac_sha256_ctx ctx;
    hmac_sha256_init(&ctx, key, key_len);
    hmac_sha256_update(&ctx, data, data_len);
    hmac_sha256_final(&ctx, out);
}

/* PBKDF2-HMAC-SHA256 (RFC 8018) */
int pbkdf2_hmac_sha256(const uint8_t *password, size_t password_len,
                       const uint8_t *salt, size_t salt_len,
                       uint32_t iterations,
                       uint8_t *out_key, size_t out_len) {
    if (!password && password_len > 0) return -1;
    if (!salt && salt_len > 0) return -1;
    if (iterations == 0 || !out_key || out_len == 0) return -1;

    uint32_t block_count = (uint32_t)((out_len + SHA256_DIGEST_SIZE - 1) / SHA256_DIGEST_SIZE);
    uint8_t u_prev[SHA256_DIGEST_SIZE];
    uint8_t u_curr[SHA256_DIGEST_SIZE];
    uint8_t block_accum[SHA256_DIGEST_SIZE];
    uint8_t counter_bytes[4];

    for (uint32_t b = 1; b <= block_count; b++) {
        counter_bytes[0] = (uint8_t)(b >> 24);
        counter_bytes[1] = (uint8_t)(b >> 16);
        counter_bytes[2] = (uint8_t)(b >> 8);
        counter_bytes[3] = (uint8_t)(b);

        /* U_1 = HMAC(password, salt || INT(b)) */
        hmac_sha256_ctx hctx;
        hmac_sha256_init(&hctx, password, password_len);
        if (salt && salt_len > 0) {
            hmac_sha256_update(&hctx, salt, salt_len);
        }
        hmac_sha256_update(&hctx, counter_bytes, 4);
        hmac_sha256_final(&hctx, u_prev);

        memcpy(block_accum, u_prev, SHA256_DIGEST_SIZE);

        /* U_2 ... U_c */
        for (uint32_t i = 1; i < iterations; i++) {
            hmac_sha256_calc(password, password_len, u_prev, SHA256_DIGEST_SIZE, u_curr);
            for (size_t k = 0; k < SHA256_DIGEST_SIZE; k++) {
                block_accum[k] ^= u_curr[k];
            }
            memcpy(u_prev, u_curr, SHA256_DIGEST_SIZE);
        }

        size_t write_len = (out_len > SHA256_DIGEST_SIZE) ? SHA256_DIGEST_SIZE : out_len;
        memcpy(out_key, block_accum, write_len);
        out_key += write_len;
        out_len -= write_len;
    }

    crypto_zeroize(u_prev, sizeof(u_prev));
    crypto_zeroize(u_curr, sizeof(u_curr));
    crypto_zeroize(block_accum, sizeof(block_accum));
    return 0;
}
