#include "cipher_classic.h"
#include <ctype.h>
#include <string.h>

size_t classic_fold_spanish_utf8(char *text, size_t len) {
    if (!text || len == 0) return 0;
    size_t r = 0;
    size_t w = 0;

    while (r < len) {
        unsigned char b1 = (unsigned char)text[r];
        if (b1 == 0xC3 && (r + 1 < len)) {
            unsigned char b2 = (unsigned char)text[r + 1];
            char replacement = 0;
            switch (b2) {
                case 0xA1: /* á */
                case 0xA0: /* à */
                case 0xA4: /* ä */ replacement = 'a'; break;
                case 0xA9: /* é */
                case 0xA8: /* è */
                case 0xAB: /* ë */ replacement = 'e'; break;
                case 0xAD: /* í */
                case 0xAC: /* ì */
                case 0xAF: /* ï */ replacement = 'i'; break;
                case 0xB3: /* ó */
                case 0xB2: /* ò */
                case 0xB6: /* ö */ replacement = 'o'; break;
                case 0xBA: /* ú */
                case 0xB9: /* ù */
                case 0xBC: /* ü */ replacement = 'u'; break;
                case 0xB1: /* ñ */ replacement = 'n'; break;

                case 0x81: /* Á */
                case 0x80: /* À */
                case 0x84: /* Ä */ replacement = 'A'; break;
                case 0x89: /* É */
                case 0x88: /* È */
                case 0x8B: /* Ë */ replacement = 'E'; break;
                case 0x8D: /* Í */
                case 0x8C: /* Ì */
                case 0x8F: /* Ï */ replacement = 'I'; break;
                case 0x93: /* Ó */
                case 0x92: /* Ò */
                case 0x96: /* Ö */ replacement = 'O'; break;
                case 0x9A: /* Ú */
                case 0x99: /* Ù */
                case 0x9C: /* Ü */ replacement = 'U'; break;
                case 0x91: /* Ñ */ replacement = 'N'; break;
                default: break;
            }
            if (replacement) {
                text[w++] = replacement;
                r += 2;
                continue;
            }
        }
        text[w++] = text[r++];
    }
    return w;
}

int caesar_encrypt(const char *src, size_t len, uint8_t shift, char *dst) {
    if (!src || !dst) return -1;
    shift = (uint8_t)(shift % 26);
    for (size_t i = 0; i < len; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            dst[i] = (char)('A' + ((c - 'A' + shift) % 26));
        } else if (c >= 'a' && c <= 'z') {
            dst[i] = (char)('a' + ((c - 'a' + shift) % 26));
        } else {
            dst[i] = c;
        }
    }
    return 0;
}

int caesar_decrypt(const char *src, size_t len, uint8_t shift, char *dst) {
    if (!src || !dst) return -1;
    shift = (uint8_t)(shift % 26);
    uint8_t rev_shift = (uint8_t)((26 - shift) % 26);
    return caesar_encrypt(src, len, rev_shift, dst);
}

int vigenere_encrypt(const char *src, size_t len, const char *key, char *dst) {
    if (!src || !dst || !key) return -1;
    size_t key_len = strlen(key);
    if (key_len == 0) return -1;

    /* Extract shift values from key */
    uint8_t shifts[MAX_VIGENERE_KEY_LEN];
    size_t valid_key_len = 0;
    for (size_t i = 0; i < key_len && valid_key_len < MAX_VIGENERE_KEY_LEN; i++) {
        char k = key[i];
        if (k >= 'A' && k <= 'Z') {
            shifts[valid_key_len++] = (uint8_t)(k - 'A');
        } else if (k >= 'a' && k <= 'z') {
            shifts[valid_key_len++] = (uint8_t)(k - 'a');
        }
    }
    if (valid_key_len == 0) return -1;

    size_t k_idx = 0;
    for (size_t i = 0; i < len; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            uint8_t s = shifts[k_idx % valid_key_len];
            dst[i] = (char)('A' + ((c - 'A' + s) % 26));
            k_idx++;
        } else if (c >= 'a' && c <= 'z') {
            uint8_t s = shifts[k_idx % valid_key_len];
            dst[i] = (char)('a' + ((c - 'a' + s) % 26));
            k_idx++;
        } else {
            dst[i] = c;
        }
    }
    return 0;
}

int vigenere_decrypt(const char *src, size_t len, const char *key, char *dst) {
    if (!src || !dst || !key) return -1;
    size_t key_len = strlen(key);
    if (key_len == 0) return -1;

    uint8_t shifts[MAX_VIGENERE_KEY_LEN];
    size_t valid_key_len = 0;
    for (size_t i = 0; i < key_len && valid_key_len < MAX_VIGENERE_KEY_LEN; i++) {
        char k = key[i];
        if (k >= 'A' && k <= 'Z') {
            shifts[valid_key_len++] = (uint8_t)((26 - (k - 'A')) % 26);
        } else if (k >= 'a' && k <= 'z') {
            shifts[valid_key_len++] = (uint8_t)((26 - (k - 'a')) % 26);
        }
    }
    if (valid_key_len == 0) return -1;

    size_t k_idx = 0;
    for (size_t i = 0; i < len; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            uint8_t s = shifts[k_idx % valid_key_len];
            dst[i] = (char)('A' + ((c - 'A' + s) % 26));
            k_idx++;
        } else if (c >= 'a' && c <= 'z') {
            uint8_t s = shifts[k_idx % valid_key_len];
            dst[i] = (char)('a' + ((c - 'a' + s) % 26));
            k_idx++;
        } else {
            dst[i] = c;
        }
    }
    return 0;
}

/* Standard IEEE 802.3 CRC32 (polynomial 0xEDB88320) */
uint32_t classic_calc_crc32(const uint8_t *data, size_t len) {
    if (!data || len == 0) return 0;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(int32_t)(crc & 1)));
        }
    }
    return ~crc;
}
