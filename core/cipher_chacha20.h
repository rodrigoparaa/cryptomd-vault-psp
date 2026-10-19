#ifndef CIPHER_CHACHA20_H
#define CIPHER_CHACHA20_H

#include <stdint.h>
#include <stddef.h>

#define CHACHA20_KEY_SIZE   32
#define CHACHA20_NONCE_SIZE 12
#define CHACHA20_BLOCK_SIZE 64

typedef struct {
    uint32_t state[16];
} chacha20_ctx;

/* Initialize ChaCha20 context with 32-byte key, 12-byte nonce, and starting counter */
void chacha20_init(chacha20_ctx *ctx, const uint8_t key[CHACHA20_KEY_SIZE],
                   const uint8_t nonce[CHACHA20_NONCE_SIZE], uint32_t counter);

/* In-place or src->dst stream encryption/decryption (RFC 8439) */
void chacha20_xor(chacha20_ctx *ctx, const uint8_t *src, uint8_t *dst, size_t len);

/* Helper: one-shot ChaCha20 stream cipher */
void chacha20_crypt(const uint8_t key[CHACHA20_KEY_SIZE],
                    const uint8_t nonce[CHACHA20_NONCE_SIZE],
                    uint32_t counter,
                    const uint8_t *src, uint8_t *dst, size_t len);

#endif /* CIPHER_CHACHA20_H */
