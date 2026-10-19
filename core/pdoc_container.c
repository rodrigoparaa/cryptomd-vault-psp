#include "pdoc_container.h"
#include "crypto_primitives.h"
#include "cipher_classic.h"
#include "cipher_aes256.h"
#include "cipher_chacha20.h"
#include <string.h>
#include <stdio.h>

int pdoc_is_valid_header(const uint8_t *buf, size_t len, pdoc_header *hdr) {
    if (!buf || len < PDOC_HEADER_SIZE) return 0;
    const pdoc_header *h = (const pdoc_header *)buf;
    if (memcmp(h->magic, PDOC_MAGIC, 4) != 0) return 0;
    if (h->version != PDOC_VERSION) return 0;
    if (h->cipher_level < 1 || h->cipher_level > 4) return 0;
    if (h->plain_len > PDOC_MAX_DOC_SIZE) return 0;
    if (h->cipher_len > PDOC_MAX_DOC_SIZE + 16) return 0;

    if (h->cipher_level == PDOC_LEVEL_AES256) {
        if (h->cipher_len <= h->plain_len || h->cipher_len > h->plain_len + 16 || (h->cipher_len % 16) != 0) {
            return 0;
        }
    } else {
        if (h->cipher_len != h->plain_len) {
            return 0;
        }
    }

    if (h->cipher_level >= PDOC_LEVEL_AES256) {
        if (h->kdf_iters < 1000 || h->kdf_iters > 100000) {
            return 0;
        }
    }

    if (hdr) {
        memcpy(hdr, h, sizeof(pdoc_header));
    }
    return 1;
}

int pdoc_pack_encrypt(uint8_t level, uint8_t format, uint8_t key_mode,
                      const uint8_t *key_data, size_t key_len,
                      const uint8_t salt[16], const uint8_t iv_nonce[16],
                      uint64_t timestamp_utc,
                      const uint8_t *plain, size_t plain_len,
                      uint8_t *out_buf, size_t *out_len) {
    if (level < 1 || level > 4 || !plain || plain_len > PDOC_MAX_DOC_SIZE || !out_buf || !out_len) {
        return PDOC_ERR_INVALID_PARAM;
    }

    pdoc_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, PDOC_MAGIC, 4);
    hdr.version = PDOC_VERSION;
    hdr.cipher_level = level;
    hdr.orig_format = format;
    hdr.key_mode = key_mode;
    hdr.key_len_hint = (uint8_t)(key_len > 255 ? 255 : key_len);
    hdr.plain_len = (uint32_t)plain_len;
    hdr.created_utc = timestamp_utc;

    if (salt) memcpy(hdr.salt, salt, 16);
    if (iv_nonce) memcpy(hdr.iv_nonce, iv_nonce, 16);

    uint8_t k_enc[32];
    uint8_t k_mac[32];
    memset(k_enc, 0, sizeof(k_enc));
    memset(k_mac, 0, sizeof(k_mac));

    uint8_t *cipher_dest = out_buf + PDOC_HEADER_SIZE;
    size_t cipher_written = 0;

    if (level == PDOC_LEVEL_CESAR) {
        hdr.kdf_iters = 0;
        uint32_t crc = classic_calc_crc32(plain, plain_len);
        hdr.salt[0] = (uint8_t)(crc >> 24);
        hdr.salt[1] = (uint8_t)(crc >> 16);
        hdr.salt[2] = (uint8_t)(crc >> 8);
        hdr.salt[3] = (uint8_t)(crc);

        uint8_t shift = (key_len > 0) ? key_data[0] : 3;
        caesar_encrypt((const char *)plain, plain_len, shift, (char *)cipher_dest);
        cipher_written = plain_len;

        /* Derive pseudo MAC key from shift */
        uint8_t seed[2] = { shift, 0x42 };
        sha256_hash(seed, sizeof(seed), k_mac);
    }
    else if (level == PDOC_LEVEL_VIGENERE) {
        hdr.kdf_iters = 0;
        uint32_t crc = classic_calc_crc32(plain, plain_len);
        hdr.salt[0] = (uint8_t)(crc >> 24);
        hdr.salt[1] = (uint8_t)(crc >> 16);
        hdr.salt[2] = (uint8_t)(crc >> 8);
        hdr.salt[3] = (uint8_t)(crc);

        char key_str[MAX_VIGENERE_KEY_LEN + 1];
        size_t cpy_len = (key_len < MAX_VIGENERE_KEY_LEN) ? key_len : MAX_VIGENERE_KEY_LEN;
        memcpy(key_str, key_data, cpy_len);
        key_str[cpy_len] = '\0';

        vigenere_encrypt((const char *)plain, plain_len, key_str, (char *)cipher_dest);
        cipher_written = plain_len;

        sha256_hash((const uint8_t *)key_str, cpy_len, k_mac);
        crypto_zeroize(key_str, sizeof(key_str));
    }
    else if (level == PDOC_LEVEL_AES256) {
        hdr.kdf_iters = 10000;
        uint8_t derived_keys[64];
        pbkdf2_hmac_sha256(key_data, key_len, hdr.salt, 16, hdr.kdf_iters, derived_keys, 64);
        memcpy(k_enc, derived_keys, 32);
        memcpy(k_mac, derived_keys + 32, 32);
        crypto_zeroize(derived_keys, sizeof(derived_keys));

        size_t aes_out = 0;
        if (aes256_cbc_encrypt(k_enc, hdr.iv_nonce, plain, plain_len, cipher_dest, &aes_out) != 0) {
            crypto_zeroize(k_enc, sizeof(k_enc));
            crypto_zeroize(k_mac, sizeof(k_mac));
            return PDOC_ERR_INVALID_PARAM;
        }
        cipher_written = aes_out;
    }
    else if (level == PDOC_LEVEL_CHACHA20) {
        hdr.kdf_iters = 15000;
        uint8_t derived_keys[64];
        pbkdf2_hmac_sha256(key_data, key_len, hdr.salt, 16, hdr.kdf_iters, derived_keys, 64);
        memcpy(k_enc, derived_keys, 32);
        memcpy(k_mac, derived_keys + 32, 32);
        crypto_zeroize(derived_keys, sizeof(derived_keys));

        /* 96-bit nonce from first 12 bytes of iv_nonce, initial counter = 1 */
        chacha20_crypt(k_enc, hdr.iv_nonce, 1, plain, cipher_dest, plain_len);
        cipher_written = plain_len;
    }

    hdr.cipher_len = (uint32_t)cipher_written;
    memcpy(out_buf, &hdr, PDOC_HEADER_SIZE);

    /* Encrypt-then-MAC: HMAC over header + ciphertext */
    uint8_t tag[PDOC_TAG_SIZE];
    hmac_sha256_ctx hctx;
    hmac_sha256_init(&hctx, k_mac, 32);
    hmac_sha256_update(&hctx, out_buf, PDOC_HEADER_SIZE + cipher_written);
    hmac_sha256_final(&hctx, tag);

    memcpy(out_buf + PDOC_HEADER_SIZE + cipher_written, tag, PDOC_TAG_SIZE);
    *out_len = PDOC_HEADER_SIZE + cipher_written + PDOC_TAG_SIZE;

    crypto_zeroize(k_enc, sizeof(k_enc));
    crypto_zeroize(k_mac, sizeof(k_mac));
    crypto_zeroize(tag, sizeof(tag));
    return PDOC_OK;
}

int pdoc_verify_decrypt(const uint8_t *container_buf, size_t container_len,
                        const uint8_t *key_data, size_t key_len,
                        uint8_t *plain_dst, size_t *out_plain_len,
                        pdoc_header *out_hdr) {
    if (!container_buf || container_len < PDOC_HEADER_SIZE + PDOC_TAG_SIZE || !plain_dst || !out_plain_len) {
        return PDOC_ERR_INVALID_PARAM;
    }

    pdoc_header hdr;
    if (!pdoc_is_valid_header(container_buf, container_len, &hdr)) {
        return PDOC_ERR_INVALID_MAGIC;
    }

    size_t expected_total = PDOC_HEADER_SIZE + hdr.cipher_len + PDOC_TAG_SIZE;
    if (container_len != expected_total) {
        return PDOC_ERR_INVALID_PARAM;
    }

    uint8_t k_enc[32];
    uint8_t k_mac[32];
    memset(k_enc, 0, sizeof(k_enc));
    memset(k_mac, 0, sizeof(k_mac));

    if (hdr.cipher_level == PDOC_LEVEL_CESAR) {
        uint8_t shift = (key_len > 0) ? key_data[0] : 3;
        uint8_t seed[2] = { shift, 0x42 };
        sha256_hash(seed, sizeof(seed), k_mac);
    }
    else if (hdr.cipher_level == PDOC_LEVEL_VIGENERE) {
        char key_str[MAX_VIGENERE_KEY_LEN + 1];
        size_t cpy_len = (key_len < MAX_VIGENERE_KEY_LEN) ? key_len : MAX_VIGENERE_KEY_LEN;
        memcpy(key_str, key_data, cpy_len);
        key_str[cpy_len] = '\0';
        sha256_hash((const uint8_t *)key_str, cpy_len, k_mac);
        crypto_zeroize(key_str, sizeof(key_str));
    }
    else if (hdr.cipher_level == PDOC_LEVEL_AES256 || hdr.cipher_level == PDOC_LEVEL_CHACHA20) {
        uint8_t derived_keys[64];
        pbkdf2_hmac_sha256(key_data, key_len, hdr.salt, 16, hdr.kdf_iters, derived_keys, 64);
        memcpy(k_enc, derived_keys, 32);
        memcpy(k_mac, derived_keys + 32, 32);
        crypto_zeroize(derived_keys, sizeof(derived_keys));
    }

    /* Verify HMAC tag in constant time */
    const uint8_t *file_tag = container_buf + PDOC_HEADER_SIZE + hdr.cipher_len;
    uint8_t calc_tag[PDOC_TAG_SIZE];

    hmac_sha256_ctx hctx;
    hmac_sha256_init(&hctx, k_mac, 32);
    hmac_sha256_update(&hctx, container_buf, PDOC_HEADER_SIZE + hdr.cipher_len);
    hmac_sha256_final(&hctx, calc_tag);

    if (crypto_constant_time_memcmp(calc_tag, file_tag, PDOC_TAG_SIZE) != 0) {
        crypto_zeroize(k_enc, sizeof(k_enc));
        crypto_zeroize(k_mac, sizeof(k_mac));
        crypto_zeroize(calc_tag, sizeof(calc_tag));
        return PDOC_ERR_AUTH_FAILED;
    }

    const uint8_t *cipher_src = container_buf + PDOC_HEADER_SIZE;
    int decrypt_res = PDOC_OK;

    if (hdr.cipher_level == PDOC_LEVEL_CESAR) {
        uint8_t shift = (key_len > 0) ? key_data[0] : 3;
        caesar_decrypt((const char *)cipher_src, hdr.cipher_len, shift, (char *)plain_dst);
        *out_plain_len = hdr.plain_len;
    }
    else if (hdr.cipher_level == PDOC_LEVEL_VIGENERE) {
        char key_str[MAX_VIGENERE_KEY_LEN + 1];
        size_t cpy_len = (key_len < MAX_VIGENERE_KEY_LEN) ? key_len : MAX_VIGENERE_KEY_LEN;
        memcpy(key_str, key_data, cpy_len);
        key_str[cpy_len] = '\0';
        vigenere_decrypt((const char *)cipher_src, hdr.cipher_len, key_str, (char *)plain_dst);
        *out_plain_len = hdr.plain_len;
        crypto_zeroize(key_str, sizeof(key_str));
    }
    else if (hdr.cipher_level == PDOC_LEVEL_AES256) {
        size_t plain_out = 0;
        if (aes256_cbc_decrypt(k_enc, hdr.iv_nonce, cipher_src, hdr.cipher_len, plain_dst, &plain_out) != 0) {
            decrypt_res = PDOC_ERR_PADDING;
        } else {
            *out_plain_len = plain_out;
        }
    }
    else if (hdr.cipher_level == PDOC_LEVEL_CHACHA20) {
        chacha20_crypt(k_enc, hdr.iv_nonce, 1, cipher_src, plain_dst, hdr.plain_len);
        *out_plain_len = hdr.plain_len;
    }

    if (out_hdr) {
        memcpy(out_hdr, &hdr, sizeof(pdoc_header));
    }

    crypto_zeroize(k_enc, sizeof(k_enc));
    crypto_zeroize(k_mac, sizeof(k_mac));
    crypto_zeroize(calc_tag, sizeof(calc_tag));
    return decrypt_res;
}

int pdoc_create_visible_header(char *dst, size_t max_len, uint8_t level,
                               size_t plain_len, uint32_t crc, uint8_t shift_or_hint) {
    if (!dst || max_len < 64) return -1;
    return snprintf(dst, max_len, "<!-- PDOC-VISIBLE v1 LEVEL=%u LEN=%zu CRC32=%08X HINT=%u -->\n",
                    (unsigned int)level, plain_len, (unsigned int)crc, (unsigned int)shift_or_hint);
}

int pdoc_parse_visible_header(const char *src, size_t len, uint8_t *level,
                              size_t *plain_len, uint32_t *crc, uint8_t *shift_or_hint) {
    if (!src || len < 40) return 0;
    if (strncmp(src, "<!-- PDOC-VISIBLE v1", 20) != 0) return 0;

    unsigned int lvl = 0, hnt = 0, c = 0;
    size_t plen = 0;
    if (sscanf(src, "<!-- PDOC-VISIBLE v1 LEVEL=%u LEN=%zu CRC32=%08X HINT=%u -->",
               &lvl, &plen, &c, &hnt) == 4) {
        if (level) *level = (uint8_t)lvl;
        if (plain_len) *plain_len = plen;
        if (crc) *crc = (uint32_t)c;
        if (shift_or_hint) *shift_or_hint = (uint8_t)hnt;
        return 1;
    }
    return 0;
}
