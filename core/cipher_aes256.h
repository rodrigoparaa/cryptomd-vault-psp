#ifndef CIPHER_AES256_H
#define CIPHER_AES256_H

#include <stdint.h>
#include <stddef.h>

#define AES256_KEY_SIZE   32
#define AES256_BLOCK_SIZE 16
#define AES256_ROUNDS     14

typedef struct {
    uint32_t round_keys[60];
} aes256_ctx;

/* Key expansion for AES-256 (32-byte key) */
void aes256_set_encrypt_key(aes256_ctx *ctx, const uint8_t key[AES256_KEY_SIZE]);
void aes256_set_decrypt_key(aes256_ctx *ctx, const uint8_t key[AES256_KEY_SIZE]);

/* Single block (16-byte) encrypt/decrypt */
void aes256_encrypt_block(const aes256_ctx *ctx, const uint8_t in[16], uint8_t out[16]);
void aes256_decrypt_block(const aes256_ctx *ctx, const uint8_t in[16], uint8_t out[16]);

/* AES-256-CBC with PKCS#7 padding.
 * out_buf must be large enough to hold ((src_len / 16) + 1) * 16 bytes.
 * Returns total padded/encrypted length, or -1 on error. */
int aes256_cbc_encrypt(const uint8_t *key, const uint8_t iv[16],
                       const uint8_t *src, size_t src_len,
                       uint8_t *dst, size_t *out_len);

/* AES-256-CBC with PKCS#7 unpadding.
 * src_len must be a multiple of 16.
 * Returns 0 on success (with *out_len updated), or -1 if padding is corrupted. */
int aes256_cbc_decrypt(const uint8_t *key, const uint8_t iv[16],
                       const uint8_t *src, size_t src_len,
                       uint8_t *dst, size_t *out_len);

#endif /* CIPHER_AES256_H */
