#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "../core/crypto_primitives.h"
#include "../core/cipher_classic.h"
#include "../core/cipher_aes256.h"
#include "../core/cipher_chacha20.h"
#include "../core/pdoc_container.h"
#include "../core/input_logic.h"

static size_t parse_combo_tokens(const char *str, uint8_t *out_buf, size_t max_len) {
    size_t count = 0;
    for (size_t i = 0; str[i] != '\0' && count < max_len; i++) {
        char c = str[i];
        if (c == ' ' || c == ',' || c == '-' || c == '_') continue;
        uint8_t code = BTN_CODE_NONE;
        switch (c) {
            case 'U': case 'u': code = BTN_CODE_UP; break;
            case 'D': case 'd': code = BTN_CODE_DOWN; break;
            case 'L': case 'l': code = BTN_CODE_L; break;
            case 'R': case 'r': code = BTN_CODE_R; break;
            case 'X': case 'x': code = BTN_CODE_CROSS; break;
            case 'S': case 's': case '[': code = BTN_CODE_SQUARE; break;
            case 'T': case 't': case '/': code = BTN_CODE_TRIANGLE; break;
            case '<': code = BTN_CODE_LEFT; break;
            case '>': code = BTN_CODE_RIGHT; break;
            default: code = BTN_CODE_NONE; break;
        }
        if (code != BTN_CODE_NONE) {
            out_buf[count++] = code;
        }
    }
    return count;
}

static void print_usage(const char *prog) {
    printf("===================================================================\n");
    printf(" 05_CryptoMD_Vault Host CLI Tool (macOS / Linux)\n");
    printf("===================================================================\n");
    printf("Usage:\n");
    printf("  %s pack    --level <1..4> --key <key> [--format txt|md] <input> <output.pdoc>\n", prog);
    printf("  %s unpack  --key <key> <input.pdoc> <output>\n", prog);
    printf("  %s inspect <input.pdoc>\n", prog);
    printf("  %s verify  --key <key> <input.pdoc>\n", prog);
    printf("  %s shred   <file>\n", prog);
    printf("\nLevels:\n");
    printf("  1 = Caesar (ROT-N, key is 1..25)\n");
    printf("  2 = Vigenere (key is keyword)\n");
    printf("  3 = AES-256-CBC + HMAC-SHA256 (key is combo/passphrase)\n");
    printf("  4 = ChaCha20-256 + HMAC-SHA256 (key is combo/passphrase)\n");
    printf("===================================================================\n");
}

static uint8_t *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > PDOC_MAX_DOC_SIZE) {
        fclose(f);
        return NULL;
    }
    uint8_t *buf = (uint8_t *)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);
    *out_len = rd;
    return buf;
}

static int write_file(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t wr = fwrite(data, 1, len, f);
    fclose(f);
    return (wr == len) ? 0 : -1;
}

static void fill_host_random(uint8_t *buf, size_t len) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        if (fread(buf, 1, len, f) == len) {
            fclose(f);
            return;
        }
        fclose(f);
    }
    /* Fallback pseudo-random */
    static int s_seeded = 0;
    if (!s_seeded) {
        srand((unsigned int)time(NULL) ^ (unsigned int)clock());
        s_seeded = 1;
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(rand() & 0xFF);
    }
}

static int cmd_pack(int argc, char **argv) {
    int level = 4;
    int format = PDOC_FMT_MD;
    const char *key_str = NULL;
    const char *in_path = NULL;
    const char *out_path = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--level") == 0 && i + 1 < argc) {
            level = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            key_str = argv[++i];
        } else if (strcmp(argv[i], "--format") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "txt") == 0) format = PDOC_FMT_TXT;
            else format = PDOC_FMT_MD;
        } else if (!in_path) {
            in_path = argv[i];
        } else if (!out_path) {
            out_path = argv[i];
        }
    }

    if (!key_str || !in_path || !out_path || level < 1 || level > 4) {
        print_usage(argv[0]);
        return 1;
    }

    size_t plain_len = 0;
    uint8_t *plain = read_file(in_path, &plain_len);
    if (!plain) {
        fprintf(stderr, "[-] Error: Could not read input file '%s'\n", in_path);
        return 1;
    }

    uint8_t salt[16];
    uint8_t iv_nonce[16];
    fill_host_random(salt, 16);
    fill_host_random(iv_nonce, 16);

    uint8_t key_buf[64];
    memset(key_buf, 0, sizeof(key_buf));
    size_t key_len = strlen(key_str);
    if (key_len >= sizeof(key_buf)) key_len = sizeof(key_buf) - 1;
    uint8_t key_mode = PDOC_KEYMODE_COMBO;

    if (level == PDOC_LEVEL_CESAR) {
        key_mode = PDOC_KEYMODE_NUMBER;
        key_buf[0] = (uint8_t)atoi(key_str);
        key_len = 1;
    } else if (level == PDOC_LEVEL_VIGENERE) {
        key_mode = PDOC_KEYMODE_WORD;
        memcpy(key_buf, key_str, key_len);
    } else {
        key_mode = PDOC_KEYMODE_COMBO;
        key_len = parse_combo_tokens(key_str, key_buf, sizeof(key_buf));
        if (key_len == 0) {
            fprintf(stderr, "[-] Error: Invalid button combo '%s'\n", key_str);
            crypto_zeroize(plain, plain_len);
            crypto_zeroize(key_buf, sizeof(key_buf));
            free(plain);
            return 1;
        }
    }

    size_t max_out = plain_len + PDOC_HEADER_SIZE + PDOC_TAG_SIZE + 64;
    uint8_t *out_buf = (uint8_t *)malloc(max_out);
    if (!out_buf) {
        crypto_zeroize(plain, plain_len);
        crypto_zeroize(key_buf, sizeof(key_buf));
        free(plain);
        return 1;
    }
    size_t final_size = 0;

    int res = pdoc_pack_encrypt((uint8_t)level, (uint8_t)format, key_mode,
                                key_buf, key_len, salt, iv_nonce,
                                (uint64_t)time(NULL),
                                plain, plain_len,
                                out_buf, &final_size);

    crypto_zeroize(key_buf, sizeof(key_buf));
    crypto_zeroize(salt, sizeof(salt));
    crypto_zeroize(iv_nonce, sizeof(iv_nonce));

    if (res != PDOC_OK) {
        fprintf(stderr, "[-] Encryption failed with error code: %d\n", res);
        crypto_zeroize(plain, plain_len);
        free(plain);
        free(out_buf);
        return 1;
    }

    if (write_file(out_path, out_buf, final_size) != 0) {
        fprintf(stderr, "[-] Error writing output file '%s'\n", out_path);
        crypto_zeroize(plain, plain_len);
        crypto_zeroize(out_buf, final_size);
        free(plain);
        free(out_buf);
        return 1;
    }

    printf("[+] Successfully packed and encrypted: %s -> %s\n", in_path, out_path);
    printf("    Level: %d, Original Size: %zu B, Encrypted Size: %zu B\n", level, plain_len, final_size);

    crypto_zeroize(plain, plain_len);
    crypto_zeroize(out_buf, final_size);
    free(plain);
    free(out_buf);
    return 0;
}

static int cmd_unpack(int argc, char **argv) {
    const char *key_str = NULL;
    const char *in_path = NULL;
    const char *out_path = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            key_str = argv[++i];
        } else if (!in_path) {
            in_path = argv[i];
        } else if (!out_path) {
            out_path = argv[i];
        }
    }

    if (!key_str || !in_path || !out_path) {
        print_usage(argv[0]);
        return 1;
    }

    size_t file_len = 0;
    uint8_t *container = read_file(in_path, &file_len);
    if (!container) {
        fprintf(stderr, "[-] Error: Could not read container '%s'\n", in_path);
        return 1;
    }

    pdoc_header hdr;
    if (!pdoc_is_valid_header(container, file_len, &hdr)) {
        fprintf(stderr, "[-] Error: Invalid PDOC container format in '%s'\n", in_path);
        free(container);
        return 1;
    }

    uint8_t key_buf[64];
    memset(key_buf, 0, sizeof(key_buf));
    size_t key_len = strlen(key_str);
    if (key_len >= sizeof(key_buf)) key_len = sizeof(key_buf) - 1;
    if (hdr.cipher_level == PDOC_LEVEL_CESAR) {
        key_buf[0] = (uint8_t)atoi(key_str);
        key_len = 1;
    } else if (hdr.cipher_level == PDOC_LEVEL_VIGENERE) {
        memcpy(key_buf, key_str, key_len);
    } else {
        key_len = parse_combo_tokens(key_str, key_buf, sizeof(key_buf));
    }

    uint8_t *plain = (uint8_t *)malloc(hdr.plain_len + 32);
    if (!plain) {
        crypto_zeroize(key_buf, sizeof(key_buf));
        free(container);
        return 1;
    }
    size_t plain_len = 0;

    int res = pdoc_verify_decrypt(container, file_len, key_buf, key_len, plain, &plain_len, &hdr);
    crypto_zeroize(key_buf, sizeof(key_buf));
    if (res != PDOC_OK) {
        if (res == PDOC_ERR_AUTH_FAILED) {
            fprintf(stderr, "[-] Authentication Failed: Wrong key or tampered file!\n");
        } else {
            fprintf(stderr, "[-] Decryption Failed: Error code %d\n", res);
        }
        crypto_zeroize(plain, hdr.plain_len + 32);
        free(container);
        free(plain);
        return 1;
    }

    if (write_file(out_path, plain, plain_len) != 0) {
        fprintf(stderr, "[-] Error: Could not write output '%s'\n", out_path);
        crypto_zeroize(plain, plain_len);
        free(container);
        free(plain);
        return 1;
    }

    printf("[+] Successfully verified and decrypted: %s -> %s\n", in_path, out_path);
    printf("    Plaintext Size: %zu B\n", plain_len);

    crypto_zeroize(plain, plain_len);
    free(container);
    free(plain);
    return 0;
}

static int cmd_inspect(const char *in_path) {
    size_t file_len = 0;
    uint8_t *container = read_file(in_path, &file_len);
    if (!container) {
        fprintf(stderr, "[-] Error: Could not read '%s'\n", in_path);
        return 1;
    }

    pdoc_header hdr;
    if (!pdoc_is_valid_header(container, file_len, &hdr)) {
        fprintf(stderr, "[-] Error: '%s' is not a valid PDOC container\n", in_path);
        free(container);
        return 1;
    }

    const char *lvl_name = "Unknown";
    switch (hdr.cipher_level) {
        case 1: lvl_name = "1 (Caesar / ROT-N)"; break;
        case 2: lvl_name = "2 (Vigenere)"; break;
        case 3: lvl_name = "3 (AES-256-CBC + HMAC-SHA256)"; break;
        case 4: lvl_name = "4 (ChaCha20-256 + HMAC-SHA256)"; break;
    }

    printf("====================================================\n");
    printf(" PDOC Container Header Inspection: %s\n", in_path);
    printf("====================================================\n");
    printf("Magic:              %.4s\n", hdr.magic);
    printf("Version:            %u\n", hdr.version);
    printf("Cipher Level:       %s\n", lvl_name);
    printf("Original Format:    %s\n", hdr.orig_format == PDOC_FMT_MD ? "Markdown (.md)" : "Text (.txt)");
    printf("Key Mode:           %u (0=Num, 1=Word, 2=Combo)\n", hdr.key_mode);
    printf("Key Hint Length:    %u\n", hdr.key_len_hint);
    printf("PBKDF2 Iterations:  %u\n", hdr.kdf_iters);
    printf("Plaintext Size:     %u bytes\n", hdr.plain_len);
    printf("Ciphertext Size:    %u bytes\n", hdr.cipher_len);
    printf("Total File Size:    %zu bytes\n", file_len);
    printf("Created UTC:        %llu\n", (unsigned long long)hdr.created_utc);
    printf("====================================================\n");

    free(container);
    return 0;
}

static int cmd_verify(const char *in_path, const char *key_str) {
    size_t file_len = 0;
    uint8_t *container = read_file(in_path, &file_len);
    if (!container) {
        fprintf(stderr, "[-] Error: Could not read '%s'\n", in_path);
        return 1;
    }

    pdoc_header hdr;
    if (!pdoc_is_valid_header(container, file_len, &hdr)) {
        fprintf(stderr, "[-] Error: Invalid container\n");
        free(container);
        return 1;
    }

    uint8_t key_buf[64];
    memset(key_buf, 0, sizeof(key_buf));
    size_t key_len = strlen(key_str);
    if (key_len >= sizeof(key_buf)) key_len = sizeof(key_buf) - 1;
    if (hdr.cipher_level == PDOC_LEVEL_CESAR) {
        key_buf[0] = (uint8_t)atoi(key_str);
        key_len = 1;
    } else if (hdr.cipher_level == PDOC_LEVEL_VIGENERE) {
        memcpy(key_buf, key_str, key_len);
    } else {
        key_len = parse_combo_tokens(key_str, key_buf, sizeof(key_buf));
    }

    uint8_t *dummy_plain = (uint8_t *)malloc(hdr.plain_len + 32);
    if (!dummy_plain) {
        crypto_zeroize(key_buf, sizeof(key_buf));
        free(container);
        return 1;
    }
    size_t out_len = 0;
    int res = pdoc_verify_decrypt(container, file_len, key_buf, key_len, dummy_plain, &out_len, &hdr);

    crypto_zeroize(key_buf, sizeof(key_buf));
    crypto_zeroize(dummy_plain, hdr.plain_len + 32);
    free(dummy_plain);
    free(container);

    if (res == PDOC_OK) {
        printf("[+] Verification SUCCESSFUL: Integrity intact and key is valid.\n");
        return 0;
    } else {
        printf("[-] Verification FAILED: Invalid key or file corrupted/tampered!\n");
        return 1;
    }
}

static int cmd_shred(const char *path) {
    FILE *f = fopen(path, "r+b");
    if (!f) {
        fprintf(stderr, "[-] Error opening '%s' for shredding\n", path);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        fclose(f);
        remove(path);
        printf("[+] Shredded and removed '%s'\n", path);
        return 0;
    }

    uint8_t block[4096];
    /* Pass 1: Random bytes */
    long remaining = sz;
    while (remaining > 0) {
        size_t chunk = (remaining > 4096) ? 4096 : remaining;
        fill_host_random(block, chunk);
        fwrite(block, 1, chunk, f);
        remaining -= chunk;
    }
    fflush(f);
    fseek(f, 0, SEEK_SET);

    /* Pass 2: Zeroes */
    memset(block, 0, sizeof(block));
    remaining = sz;
    while (remaining > 0) {
        size_t chunk = (remaining > 4096) ? 4096 : remaining;
        fwrite(block, 1, chunk, f);
        remaining -= chunk;
    }
    fflush(f);
    fclose(f);

    remove(path);
    printf("[+] Shredded (2-pass overwrite) and removed '%s'\n", path);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "pack") == 0) {
        return cmd_pack(argc, argv);
    } else if (strcmp(argv[1], "unpack") == 0) {
        return cmd_unpack(argc, argv);
    } else if (strcmp(argv[1], "inspect") == 0 && argc >= 3) {
        return cmd_inspect(argv[2]);
    } else if (strcmp(argv[1], "verify") == 0 && argc >= 5 && strcmp(argv[2], "--key") == 0) {
        return cmd_verify(argv[4], argv[3]);
    } else if (strcmp(argv[1], "shred") == 0 && argc >= 3) {
        return cmd_shred(argv[2]);
    }

    print_usage(argv[0]);
    return 1;
}
