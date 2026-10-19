#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../core/crypto_primitives.h"
#include "../core/cipher_classic.h"
#include "../core/cipher_aes256.h"
#include "../core/cipher_chacha20.h"
#include "../core/pdoc_container.h"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        fprintf(stderr, "[-] FAILED: %s (line %d)\n", msg, __LINE__); \
    } \
} while (0)

static void hex_to_bytes(const char *hex, uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned int b;
        sscanf(hex + i * 2, "%02x", &b);
        bytes[i] = (uint8_t)b;
    }
}

/* 1. SHA-256 Official NIST Vectors */
static void test_sha256(void) {
    printf("[*] Testing SHA-256 (FIPS 180-4)...\n");
    uint8_t digest[32];

    /* Test vector 1: "abc" */
    sha256_hash((const uint8_t *)"abc", 3, digest);
    uint8_t expected_abc[32];
    hex_to_bytes("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", expected_abc, 32);
    TEST_ASSERT(memcmp(digest, expected_abc, 32) == 0, "SHA-256('abc')");

    /* Test vector 2: empty string */
    sha256_hash((const uint8_t *)"", 0, digest);
    uint8_t expected_empty[32];
    hex_to_bytes("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", expected_empty, 32);
    TEST_ASSERT(memcmp(digest, expected_empty, 32) == 0, "SHA-256('')");
}

/* 2. HMAC-SHA256 (RFC 4231 Test Case 1 & 2) */
static void test_hmac_sha256(void) {
    printf("[*] Testing HMAC-SHA256 (RFC 4231)...\n");
    uint8_t out[32];

    /* RFC 4231 Test Case 1:
     * Key = 20 bytes of 0x0b
     * Data = "Hi There"
     * Digest = b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7 */
    uint8_t key1[20];
    memset(key1, 0x0b, 20);
    hmac_sha256_calc(key1, 20, (const uint8_t *)"Hi There", 8, out);
    uint8_t exp1[32];
    hex_to_bytes("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", exp1, 32);
    TEST_ASSERT(memcmp(out, exp1, 32) == 0, "HMAC-SHA256 RFC 4231 Case 1");

    /* Test Case 2:
     * Key = "Jefe"
     * Data = "what do ya want for nothing?"
     * Digest = 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843 */
    hmac_sha256_calc((const uint8_t *)"Jefe", 4, (const uint8_t *)"what do ya want for nothing?", 28, out);
    uint8_t exp2[32];
    hex_to_bytes("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", exp2, 32);
    TEST_ASSERT(memcmp(out, exp2, 32) == 0, "HMAC-SHA256 RFC 4231 Case 2");
}

/* 3. PBKDF2-HMAC-SHA256 (RFC 6070 Test 1 & 2) */
static void test_pbkdf2(void) {
    printf("[*] Testing PBKDF2-HMAC-SHA256 (RFC 6070)...\n");
    uint8_t dk[32];

    /* PBKDF2-HMAC-SHA256 Test 1: P = "password", S = "salt", c = 1, dkLen = 32 */
    pbkdf2_hmac_sha256((const uint8_t *)"password", 8, (const uint8_t *)"salt", 4, 1, dk, 32);
    uint8_t exp1[32];
    hex_to_bytes("120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b", exp1, 32);
    TEST_ASSERT(memcmp(dk, exp1, 32) == 0, "PBKDF2-SHA256 (c=1)");

    /* PBKDF2-HMAC-SHA256 Test 2: P = "password", S = "salt", c = 2, dkLen = 32 */
    pbkdf2_hmac_sha256((const uint8_t *)"password", 8, (const uint8_t *)"salt", 4, 2, dk, 32);
    uint8_t exp2[32];
    hex_to_bytes("ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43", exp2, 32);
    TEST_ASSERT(memcmp(dk, exp2, 32) == 0, "PBKDF2-SHA256 (c=2)");
}

/* 4. AES-256 (FIPS 197 Single Block & CBC) */
static void test_aes256(void) {
    printf("[*] Testing AES-256 (FIPS 197)...\n");

    /* NIST FIPS 197 Appendix C.3 AES-256 Known Answer Test */
    uint8_t key[32];
    hex_to_bytes("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", key, 32);
    uint8_t pt[16];
    hex_to_bytes("00112233445566778899aabbccddeeff", pt, 16);

    aes256_ctx ctx;
    aes256_set_encrypt_key(&ctx, key);
    uint8_t ct[16];
    aes256_encrypt_block(&ctx, pt, ct);

    uint8_t exp_ct[16];
    hex_to_bytes("8ea2b7ca516745bfeafc49904b496089", exp_ct, 16);
    TEST_ASSERT(memcmp(ct, exp_ct, 16) == 0, "AES-256 Block Encrypt (NIST FIPS 197)");

    aes256_set_decrypt_key(&ctx, key);
    uint8_t decrypted[16];
    aes256_decrypt_block(&ctx, ct, decrypted);
    TEST_ASSERT(memcmp(decrypted, pt, 16) == 0, "AES-256 Block Decrypt");

    /* Test CBC with PKCS#7 */
    const char *msg = "Top Secret Document for Sony PSP Go with Markdown formatting # H1";
    size_t msg_len = strlen(msg);
    uint8_t iv[16];
    memset(iv, 0x5A, 16);

    uint8_t cbc_cipher[256];
    size_t cbc_cipher_len = 0;
    int enc_res = aes256_cbc_encrypt(key, iv, (const uint8_t *)msg, msg_len, cbc_cipher, &cbc_cipher_len);
    TEST_ASSERT(enc_res == 0, "AES-256-CBC Encrypt");
    TEST_ASSERT(cbc_cipher_len % 16 == 0, "AES-256-CBC Padding Multiple of 16");

    uint8_t cbc_plain[256];
    size_t cbc_plain_len = 0;
    int dec_res = aes256_cbc_decrypt(key, iv, cbc_cipher, cbc_cipher_len, cbc_plain, &cbc_plain_len);
    TEST_ASSERT(dec_res == 0, "AES-256-CBC Decrypt");
    TEST_ASSERT(cbc_plain_len == msg_len, "AES-256-CBC Decrypted Length");
    TEST_ASSERT(memcmp(cbc_plain, msg, msg_len) == 0, "AES-256-CBC Content Match");
}

/* 5. ChaCha20 (RFC 8439 §2.4.2 Test Vector) */
static void test_chacha20(void) {
    printf("[*] Testing ChaCha20 (RFC 8439 §2.4.2)...\n");

    uint8_t key[32];
    hex_to_bytes("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", key, 32);

    uint8_t nonce[12];
    hex_to_bytes("000000000000004a00000000", nonce, 12);

    const char *sun_quote = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    size_t len = strlen(sun_quote);

    uint8_t out[256];
    chacha20_crypt(key, nonce, 1, (const uint8_t *)sun_quote, out, len);

    uint8_t exp_head[16];
    hex_to_bytes("6e2e359a2568f98041ba0728dd0d6981", exp_head, 16);
    TEST_ASSERT(memcmp(out, exp_head, 16) == 0, "ChaCha20 RFC 8439 Head Vector");

    /* Roundtrip */
    uint8_t recovered[256];
    chacha20_crypt(key, nonce, 1, out, recovered, len);
    TEST_ASSERT(memcmp(recovered, sun_quote, len) == 0, "ChaCha20 Roundtrip Decrypt");
}

/* 6. Classical Ciphers: Caesar & Vigenère */
static void test_classical_ciphers(void) {
    printf("[*] Testing Caesar & Vigenere...\n");

    /* Caesar */
    const char *plain = "# Encabezado H1: Hola Mundo! 123";
    size_t len = strlen(plain);
    char cipher[128];
    char roundtrip[128];

    caesar_encrypt(plain, len, 3, cipher);
    /* '#' and ':' and '!' and ' ' and '123' must be unchanged */
    TEST_ASSERT(cipher[0] == '#', "Caesar preserves markdown #");
    TEST_ASSERT(cipher[1] == ' ', "Caesar preserves space");
    TEST_ASSERT(cipher[2] == 'H', "Caesar shifts 'E' by 3 to 'H'");

    caesar_decrypt(cipher, len, 3, roundtrip);
    TEST_ASSERT(memcmp(roundtrip, plain, len) == 0, "Caesar roundtrip match");

    /* Spanish folding */
    char spanish_txt[128];
    strcpy(spanish_txt, "Canción del año");
    size_t folded_len = classic_fold_spanish_utf8(spanish_txt, strlen(spanish_txt));
    spanish_txt[folded_len] = '\0';
    TEST_ASSERT(strcmp(spanish_txt, "Cancion del ano") == 0, "Spanish UTF-8 folding (canción -> cancion)");

    /* Vigenère */
    const char *vig_plain = "ATTACK AT DAWN #secret";
    char vig_cipher[128];
    char vig_back[128];
    vigenere_encrypt(vig_plain, strlen(vig_plain), "LEMON", vig_cipher);
    vigenere_decrypt(vig_cipher, strlen(vig_plain), "LEMON", vig_back);
    TEST_ASSERT(memcmp(vig_back, vig_plain, strlen(vig_plain)) == 0, "Vigenere roundtrip match");
    TEST_ASSERT(vig_cipher[15] == '#', "Vigenere preserves markdown #");
}

/* 7. PDOC Container (Packing, Verify-then-Decrypt, Tamper Resistance) */
static void test_pdoc_container(void) {
    printf("[*] Testing PDOC Container (Levels 1-4 & Tamper Resistance)...\n");

    const char *doc = "# Diario Secreto en PSP Go\n- Tarea 1: Proteger notas.\n- Tarea 2: 60 FPS.\n";
    size_t doc_len = strlen(doc);

    uint8_t salt[16];
    memset(salt, 0x11, 16);
    uint8_t iv[16];
    memset(iv, 0x22, 16);

    uint8_t container[4096];
    size_t container_len = 0;

    uint8_t recovered[4096];
    size_t recovered_len = 0;
    pdoc_header hdr;

    /* Level 1: Caesar */
    uint8_t key_l1[1] = { 7 };
    int pack1 = pdoc_pack_encrypt(PDOC_LEVEL_CESAR, PDOC_FMT_MD, PDOC_KEYMODE_NUMBER,
                                  key_l1, 1, salt, iv, 1700000000ULL,
                                  (const uint8_t *)doc, doc_len,
                                  container, &container_len);
    TEST_ASSERT(pack1 == PDOC_OK, "PDOC Pack Level 1 (Caesar)");

    int dec1 = pdoc_verify_decrypt(container, container_len, key_l1, 1, recovered, &recovered_len, &hdr);
    TEST_ASSERT(dec1 == PDOC_OK, "PDOC Decrypt Level 1 (Caesar)");
    TEST_ASSERT(recovered_len == doc_len, "PDOC Decrypt Level 1 Length");
    TEST_ASSERT(memcmp(recovered, doc, doc_len) == 0, "PDOC Decrypt Level 1 Content Match");

    /* Level 2: Vigenère */
    const char *key_l2 = "RETRO";
    int pack2 = pdoc_pack_encrypt(PDOC_LEVEL_VIGENERE, PDOC_FMT_MD, PDOC_KEYMODE_WORD,
                                  (const uint8_t *)key_l2, strlen(key_l2), salt, iv, 1700000000ULL,
                                  (const uint8_t *)doc, doc_len,
                                  container, &container_len);
    TEST_ASSERT(pack2 == PDOC_OK, "PDOC Pack Level 2 (Vigenere)");

    int dec2 = pdoc_verify_decrypt(container, container_len, (const uint8_t *)key_l2, strlen(key_l2),
                                   recovered, &recovered_len, &hdr);
    TEST_ASSERT(dec2 == PDOC_OK, "PDOC Decrypt Level 2 (Vigenere)");
    TEST_ASSERT(memcmp(recovered, doc, doc_len) == 0, "PDOC Decrypt Level 2 Content Match");

    /* Level 3: AES-256 */
    const char *key_l3 = "LRXOTriangle";
    int pack3 = pdoc_pack_encrypt(PDOC_LEVEL_AES256, PDOC_FMT_MD, PDOC_KEYMODE_COMBO,
                                  (const uint8_t *)key_l3, strlen(key_l3), salt, iv, 1700000000ULL,
                                  (const uint8_t *)doc, doc_len,
                                  container, &container_len);
    TEST_ASSERT(pack3 == PDOC_OK, "PDOC Pack Level 3 (AES-256)");

    int dec3 = pdoc_verify_decrypt(container, container_len, (const uint8_t *)key_l3, strlen(key_l3),
                                   recovered, &recovered_len, &hdr);
    TEST_ASSERT(dec3 == PDOC_OK, "PDOC Decrypt Level 3 (AES-256)");
    TEST_ASSERT(recovered_len == doc_len, "PDOC Decrypt Level 3 Content Match");

    /* Level 4: ChaCha20 */
    const char *key_l4 = "UpUpUpDownLeftRight";
    int pack4 = pdoc_pack_encrypt(PDOC_LEVEL_CHACHA20, PDOC_FMT_MD, PDOC_KEYMODE_COMBO,
                                  (const uint8_t *)key_l4, strlen(key_l4), salt, iv, 1700000000ULL,
                                  (const uint8_t *)doc, doc_len,
                                  container, &container_len);
    TEST_ASSERT(pack4 == PDOC_OK, "PDOC Pack Level 4 (ChaCha20)");

    int dec4 = pdoc_verify_decrypt(container, container_len, (const uint8_t *)key_l4, strlen(key_l4),
                                   recovered, &recovered_len, &hdr);
    TEST_ASSERT(dec4 == PDOC_OK, "PDOC Decrypt Level 4 (ChaCha20)");
    TEST_ASSERT(recovered_len == doc_len, "PDOC Decrypt Level 4 Content Match");

    /* Tamper Resistance Test: flip 1 bit in ciphertext */
    container[PDOC_HEADER_SIZE + 5] ^= 0x01;
    int dec_tamper = pdoc_verify_decrypt(container, container_len, (const uint8_t *)key_l4, strlen(key_l4),
                                         recovered, &recovered_len, &hdr);
    TEST_ASSERT(dec_tamper == PDOC_ERR_AUTH_FAILED, "Verify-then-Decrypt rejects tampered ciphertext");

    /* Wrong key test */
    container[PDOC_HEADER_SIZE + 5] ^= 0x01; /* restore bit */
    int dec_wrong = pdoc_verify_decrypt(container, container_len, (const uint8_t *)"WRONG_KEY", 9,
                                        recovered, &recovered_len, &hdr);
    TEST_ASSERT(dec_wrong == PDOC_ERR_AUTH_FAILED, "Verify-then-Decrypt rejects wrong key");

    /* Test Visible Header */
    char vis_hdr[128];
    pdoc_create_visible_header(vis_hdr, sizeof(vis_hdr), 1, 1024, 0x1234ABCD, 7);
    uint8_t v_lvl, v_shift;
    size_t v_len;
    uint32_t v_crc;
    int parse_ok = pdoc_parse_visible_header(vis_hdr, strlen(vis_hdr), &v_lvl, &v_len, &v_crc, &v_shift);
    TEST_ASSERT(parse_ok == 1, "Visible header parsed successfully");
    TEST_ASSERT(v_lvl == 1 && v_len == 1024 && v_crc == 0x1234ABCD && v_shift == 7, "Visible header values match");

    /* Security Hardening: Reject crafted headers (integer overflow, KDF downgrade, size mismatch) */
    pdoc_header bad_hdr;
    memcpy(&bad_hdr, container, sizeof(pdoc_header));
    bad_hdr.cipher_len = 0xFFFFFFA0U; /* Integer overflow attempt on 32-bit size_t */
    TEST_ASSERT(pdoc_is_valid_header((const uint8_t *)&bad_hdr, sizeof(bad_hdr), NULL) == 0,
                "Rejects crafted header with oversized cipher_len (overflow defense)");

    memcpy(&bad_hdr, container, sizeof(pdoc_header));
    bad_hdr.kdf_iters = 1; /* KDF downgrade attack on Level 4 */
    TEST_ASSERT(pdoc_is_valid_header((const uint8_t *)&bad_hdr, sizeof(bad_hdr), NULL) == 0,
                "Rejects crafted header with downgraded kdf_iters < 1000");

    memcpy(&bad_hdr, container, sizeof(pdoc_header));
    bad_hdr.kdf_iters = 9999999U; /* CPU DoS attack on 333 MHz MIPS */
    TEST_ASSERT(pdoc_is_valid_header((const uint8_t *)&bad_hdr, sizeof(bad_hdr), NULL) == 0,
                "Rejects crafted header with excessive kdf_iters > 100000 (DoS defense)");
}

/* 8. Markdown Parser & Word-Wrap */
#include "../core/md_parser.h"
#include "../core/scroll_physics.h"
#include "../core/input_logic.h"
#include "../core/app_state.h"

static void test_md_parser_and_scroll(void) {
    printf("[*] Testing Markdown Layout & Scroll Physics...\n");

    const char *md_content = 
        "# Titulo Principal\n## Subtitulo\n> Cita importante\n"
        "- Item 1 con texto largo que debe partirse en lineas legibles para la pantalla de la PSP\n"
        "- Item 2 con mas texto para rellenar la altura del documento\n"
        "- Item 3 con explicacion detallada del cifrado\n"
        "```\nint main() {\n    printf(\"Hello PSP!\");\n    return 0;\n}\n```\n"
        "Este es un parrafo largo de prueba que ocupa varias lineas en pantalla y permite verificar que el scroll analogico funciona correctamente cuando el documento excede el tamano del viewport util de 226 pixeles de alto.\n"
        "Otro parrafo adicional para asegurar que la altura total supera ampliamente los 226 pixeles del viewport.\n";
    md_document doc;
    md_parse_document(md_content, strlen(md_content), &doc);

    TEST_ASSERT(doc.line_count > 10, "Markdown generates multiple layout lines");
    TEST_ASSERT(doc.total_height > 250, "Markdown computes total height exceeding viewport (226px)");
    TEST_ASSERT(doc.lines[1].type == MD_LINE_H1, "Markdown line identified as H1");

    /* Scroll physics */
    scroll_state sc;
    scroll_init(&sc, (float)doc.total_height, MD_VIEWPORT_HEIGHT);
    TEST_ASSERT(sc.scroll_y == 0.0f, "Scroll starts at 0.0");
    TEST_ASSERT(sc.max_scroll_y > 0.0f, "Max scroll is positive for large document");

    /* Analog deflection down (Y = +60) */
    scroll_update(&sc, 60);
    TEST_ASSERT(sc.velocity_y > 0.0f, "Analog downward deflection generates positive velocity");
    TEST_ASSERT(sc.scroll_y > 0.0f, "Scroll position advances");

    /* Page stepping */
    scroll_page_down(&sc);
    TEST_ASSERT(sc.scroll_y > 0.0f && sc.scroll_y <= sc.max_scroll_y, "Page down advances within max scroll bounds");

    /* Inline markdown & badge/link stripping */
    const char *rich_md =
        "# 📖 Titulo con Emoji\n"
        "[![Badge](https://img.shields.io/badge/test.svg)](https://example.com)\n"
        "- **4 Niveles de Cifrado:** explicacion muy larga que debe envolverse en una segunda linea de continuacion sin repetir el icono de vineta\n"
        "Enlace a [02_MarkdownReader](./Proyectos/MarkdownReader/) con `codigo`.\n";
    md_document rich_doc;
    md_parse_document(rich_md, strlen(rich_md), &rich_doc);
    TEST_ASSERT(strcmp(rich_doc.lines[1].text, "Titulo con Emoji") == 0, "Emoji and leading space stripped from H1");
    TEST_ASSERT(rich_doc.lines[3].type == MD_LINE_BULLET, "First line of bullet has MD_LINE_BULLET");
    TEST_ASSERT(strncmp(rich_doc.lines[3].text, "4 Niveles de Cifrado:", 21) == 0, "Bold ** stripped from bullet text");
    TEST_ASSERT(rich_doc.lines[4].type == MD_LINE_BULLET_CONT, "Wrapped line of bullet has MD_LINE_BULLET_CONT");
}

/* 9. Input Logic & App State */
static void test_input_and_app_state(void) {
    printf("[*] Testing Input Logic & Application FSM...\n");

    app_state app;
    app_state_init(&app);
    TEST_ASSERT(app.current_view == VIEW_EXPLORER, "Initial view is VIEW_EXPLORER");

    /* Pin entry test */
    pin_input_add(&app.pin_entry, 1); /* L */
    pin_input_add(&app.pin_entry, 2); /* R */
    pin_input_add(&app.pin_entry, 3); /* Cross */
    TEST_ASSERT(app.pin_entry.count == 3, "PIN records 3 button presses");

    pin_input_backspace(&app.pin_entry);
    TEST_ASSERT(app.pin_entry.count == 2, "PIN backspace removes last button");

    /* Panic chord detector (L + R) */
    input_tracker tr;
    input_tracker_init(&tr);
    uint32_t lr_pressed = BTN_LTRIGGER | BTN_RTRIGGER;
    for (int frame = 0; frame < PANIC_CHORD_FRAMES; frame++) {
        input_tracker_update(&tr, lr_pressed);
    }
    TEST_ASSERT(tr.panic_triggered == 1, "Panic chord triggers after 60 frames (1s)");

    /* Zeroize on view switch */
    strcpy((char *)app.plain_buf, "SuperSecretData");
    app.plain_len = 15;
    app.current_view = VIEW_VAULT_READER;
    app_state_switch_view(&app, VIEW_EXPLORER);

    TEST_ASSERT(app.plain_len == 0, "Exiting vault reader zeroizes plain_len");
    TEST_ASSERT(app.plain_buf[0] == 0, "Exiting vault reader wipes plain_buf memory");
}

int main(void) {
    printf("====================================================\n");
    printf("   05_CryptoMD_Vault: Comprehensive Test Suite     \n");
    printf("====================================================\n");

    test_sha256();
    test_hmac_sha256();
    test_pbkdf2();
    test_aes256();
    test_chacha20();
    test_classical_ciphers();
    test_pdoc_container();
    test_md_parser_and_scroll();
    test_input_and_app_state();

    printf("\n====================================================\n");
    printf("Results: %d PASSED, %d FAILED\n", g_tests_passed, g_tests_failed);
    printf("====================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
