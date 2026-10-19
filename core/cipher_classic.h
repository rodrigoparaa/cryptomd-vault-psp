#ifndef CIPHER_CLASSIC_H
#define CIPHER_CLASSIC_H

#include <stdint.h>
#include <stddef.h>

#define MAX_VIGENERE_KEY_LEN 32

/* In-place pre-normalization: folds Spanish 2-byte UTF-8 accented characters and 'ñ'
 * into single-byte ASCII equivalents (e.g. 'á' -> 'a', 'ñ' -> 'n') to prevent visual leaks.
 * Returns the new length of the sanitized string. */
size_t classic_fold_spanish_utf8(char *text, size_t len);

/* Caesar Cipher (ROT-N)
 * shift: 1 to 25.
 * Non-alphabetic characters (Markdown syntax, digits, spaces, symbols) are untouched.
 * Encrypt and Decrypt in-place or from src to dst. */
int caesar_encrypt(const char *src, size_t len, uint8_t shift, char *dst);
int caesar_decrypt(const char *src, size_t len, uint8_t shift, char *dst);

/* Vigenère Cipher
 * key: ASCII keyword string. Only letters A-Z / a-z are used as key shifts.
 * Key index advances ONLY when an alphabetic character is encrypted/decrypted,
 * keeping Markdown formatting aligned. */
int vigenere_encrypt(const char *src, size_t len, const char *key, char *dst);
int vigenere_decrypt(const char *src, size_t len, const char *key, char *dst);

/* Fast 32-bit CRC for quick integrity verification in N1/N2 */
uint32_t classic_calc_crc32(const uint8_t *data, size_t len);

#endif /* CIPHER_CLASSIC_H */
