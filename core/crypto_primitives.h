#ifndef CRYPTO_PRIMITIVES_H
#define CRYPTO_PRIMITIVES_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_DIGEST_SIZE 32
#define SHA256_BLOCK_SIZE  64

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} sha256_ctx;

typedef struct {
    sha256_ctx inner;
    sha256_ctx outer;
} hmac_sha256_ctx;

/* Volatile zeroize to prevent compiler dead-store elimination */
void crypto_zeroize(void *ptr, size_t len);

/* Constant-time memory comparison: returns 0 if identical, non-zero otherwise */
int crypto_constant_time_memcmp(const void *a, const void *b, size_t len);

/* SHA-256 (FIPS 180-4) */
void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t digest[SHA256_DIGEST_SIZE]);
void sha256_hash(const uint8_t *data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]);

/* HMAC-SHA256 (RFC 2104) */
void hmac_sha256_init(hmac_sha256_ctx *ctx, const uint8_t *key, size_t key_len);
void hmac_sha256_update(hmac_sha256_ctx *ctx, const uint8_t *data, size_t len);
void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_DIGEST_SIZE]);
void hmac_sha256_calc(const uint8_t *key, size_t key_len,
                      const uint8_t *data, size_t data_len,
                      uint8_t out[SHA256_DIGEST_SIZE]);

/* PBKDF2-HMAC-SHA256 (RFC 8018) */
int pbkdf2_hmac_sha256(const uint8_t *password, size_t password_len,
                       const uint8_t *salt, size_t salt_len,
                       uint32_t iterations,
                       uint8_t *out_key, size_t out_len);

#endif /* CRYPTO_PRIMITIVES_H */
