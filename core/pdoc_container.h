#ifndef PDOC_CONTAINER_H
#define PDOC_CONTAINER_H

#include <stdint.h>
#include <stddef.h>

#define PDOC_MAGIC "PDOC"
#define PDOC_VERSION 1
#define PDOC_HEADER_SIZE 64
#define PDOC_TAG_SIZE    32
#define PDOC_MAX_DOC_SIZE (128 * 1024) /* 128 KB */
#define PDOC_MAX_CONTAINER_SIZE (PDOC_MAX_DOC_SIZE + PDOC_HEADER_SIZE + PDOC_TAG_SIZE + 32)

#define PDOC_LEVEL_CESAR     1
#define PDOC_LEVEL_VIGENERE  2
#define PDOC_LEVEL_AES256    3
#define PDOC_LEVEL_CHACHA20  4

#define PDOC_FMT_TXT 0
#define PDOC_FMT_MD  1

#define PDOC_KEYMODE_NUMBER 0
#define PDOC_KEYMODE_WORD   1
#define PDOC_KEYMODE_COMBO  2

#pragma pack(push, 1)
typedef struct {
    char     magic[4];        /* "PDOC" */
    uint8_t  version;         /* 1 */
    uint8_t  cipher_level;    /* 1..4 */
    uint8_t  orig_format;     /* 0 = .txt, 1 = .md */
    uint8_t  key_mode;        /* 0 = Number, 1 = Word, 2 = Button combo */
    uint8_t  key_len_hint;    /* Length of password/combo */
    uint8_t  flags_pad[3];    /* Reserved (0x00) */
    uint32_t kdf_iters;       /* PBKDF2 iterations */
    uint32_t plain_len;       /* Plaintext byte length */
    uint32_t cipher_len;      /* Ciphertext byte length */
    uint8_t  salt[16];        /* PBKDF2 Salt or L1/L2 metadata */
    uint8_t  iv_nonce[16];    /* IV (AES) or Nonce (ChaCha20) */
    uint64_t created_utc;     /* Creation timestamp (Unix UTC) */
} pdoc_header;
#pragma pack(pop)

/* Error codes */
#define PDOC_OK                0
#define PDOC_ERR_INVALID_MAGIC -1
#define PDOC_ERR_VERSION       -2
#define PDOC_ERR_CORRUPT_TAG   -3
#define PDOC_ERR_AUTH_FAILED   -4
#define PDOC_ERR_PADDING       -5
#define PDOC_ERR_TOO_LARGE     -6
#define PDOC_ERR_INVALID_PARAM -7

/* Packs and encrypts a plaintext buffer into a .pdoc binary container.
 * out_buf must be at least plain_len + PDOC_HEADER_SIZE + PDOC_TAG_SIZE + 16 bytes.
 * *out_len receives total file size. */
int pdoc_pack_encrypt(uint8_t level, uint8_t format, uint8_t key_mode,
                      const uint8_t *key_data, size_t key_len,
                      const uint8_t salt[16], const uint8_t iv_nonce[16],
                      uint64_t timestamp_utc,
                      const uint8_t *plain, size_t plain_len,
                      uint8_t *out_buf, size_t *out_len);

/* Verifies HMAC tag in constant time, then decrypts container into plain_dst.
 * *out_plain_len receives decrypted plaintext length. */
int pdoc_verify_decrypt(const uint8_t *container_buf, size_t container_len,
                        const uint8_t *key_data, size_t key_len,
                        uint8_t *plain_dst, size_t *out_plain_len,
                        pdoc_header *out_hdr);

/* Checks if a buffer starts with a valid PDOC binary header */
int pdoc_is_valid_header(const uint8_t *buf, size_t len, pdoc_header *hdr);

/* Helpers for Visible Markdown (.cpr.md) headers */
int pdoc_create_visible_header(char *dst, size_t max_len, uint8_t level,
                               size_t plain_len, uint32_t crc, uint8_t shift_or_hint);
int pdoc_parse_visible_header(const char *src, size_t len, uint8_t *level,
                              size_t *plain_len, uint32_t *crc, uint8_t *shift_or_hint);

#endif /* PDOC_CONTAINER_H */
