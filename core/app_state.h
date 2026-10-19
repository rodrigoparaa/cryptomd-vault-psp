#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <stddef.h>

#include "pdoc_container.h"
#include "md_parser.h"
#include "scroll_physics.h"
#include "input_logic.h"

#define MAX_PATH_LEN 256
#define MAX_FILES_LIST 128

typedef enum {
    VIEW_EXPLORER = 0,
    VIEW_READER_PLAIN,
    VIEW_CRYPTO_WIZARD,
    VIEW_KEY_LEN_SELECT,
    VIEW_KEY_ENTRY,
    VIEW_VAULT_READER,
    VIEW_COMMIT_MODAL
} app_view;

typedef enum {
    THEME_CYBERPUNK = 0,
    THEME_SEPIA,
    THEME_OLED,
    THEME_COUNT
} app_theme;

typedef struct {
    char name[64];
    char full_path[MAX_PATH_LEN];
    uint32_t size;
    uint8_t is_dir;
    uint8_t is_pdoc;
    uint8_t cipher_level; /* 0 if plain, 1..4 if pdoc */
} file_item;

typedef struct {
    app_view current_view;
    app_view previous_view;
    app_theme current_theme;

    /* Explorer State */
    char current_dir[MAX_PATH_LEN];
    file_item files[MAX_FILES_LIST];
    size_t file_count;
    size_t selected_file_idx;
    int is_ms0_active; /* 0 = ef0:/, 1 = ms0:/ */

    /* Active File & Document State */
    char active_file_path[MAX_PATH_LEN];
    char active_file_name[64];
    uint8_t active_level;
    uint8_t active_format;
    pdoc_header active_header;

    /* Plaintext Work Buffer (RAM only, +32B headroom for PKCS#7 padding & NUL) */
    uint8_t plain_buf[PDOC_MAX_DOC_SIZE + 32];
    size_t plain_len;
    md_document doc_plain;

    /* Ciphertext Work Buffer (for Ray-X and previews) */
    uint8_t cipher_buf[PDOC_MAX_CONTAINER_SIZE];
    size_t cipher_len;
    md_document doc_cipher; /* Formatted ciphertext for L1/L2 */

    /* Scroll Engine */
    scroll_state scroll;

    /* Key Entry & Wizard Parameters */
    uint8_t wizard_target_level; /* 1..4 */
    uint8_t caesar_shift;        /* 1..25 */
    uint8_t selected_pin_len;    /* 2, 4, or 6 */
    pin_input_state pin_entry;
    drum_input_state drum_entry;
    int key_entry_for_encrypt;   /* 1 = creating key, 0 = unlocking */
    int auth_error;              /* 1 = last key attempt failed */

    /* Modal dialog status */
    int modal_active;
    int modal_action; /* 0 = Shred original, 1 = Keep original */
    char status_message[128];

    /* Input & Telemetry */
    input_tracker input;
} app_state;

void app_state_init(app_state *app);
void app_state_zeroize_vault(app_state *app);
void app_state_switch_view(app_state *app, app_view new_view);
void app_state_cycle_theme(app_state *app);

#endif /* APP_STATE_H */
