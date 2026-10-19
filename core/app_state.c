#include "app_state.h"
#include "crypto_primitives.h"
#include <string.h>

void app_state_init(app_state *app) {
    if (!app) return;
    memset(app, 0, sizeof(app_state));

    app->current_view = VIEW_EXPLORER;
    app->previous_view = VIEW_EXPLORER;
    app->current_theme = THEME_CYBERPUNK;

    strcpy(app->current_dir, "ef0:/DOCS");
    app->is_ms0_active = 0;
    app->file_count = 0;
    app->selected_file_idx = 0;

    app->wizard_target_level = PDOC_LEVEL_CHACHA20;
    app->caesar_shift = 1;
    app->selected_pin_len = 4;
    app->auth_error = 0;
    pin_input_init(&app->pin_entry, 4);
    drum_input_init(&app->drum_entry, 6);

    input_tracker_init(&app->input);
    scroll_init(&app->scroll, 0, MD_VIEWPORT_HEIGHT);
}

void app_state_zeroize_vault(app_state *app) {
    if (!app) return;
    /* Purge decrypted text */
    crypto_zeroize(app->plain_buf, sizeof(app->plain_buf));
    app->plain_len = 0;

    /* Purge parsed lines */
    crypto_zeroize(&app->doc_plain, sizeof(md_document));
    crypto_zeroize(app->cipher_buf, sizeof(app->cipher_buf));
    app->cipher_len = 0;
    crypto_zeroize(&app->doc_cipher, sizeof(md_document));

    /* Purge entered keys */
    pin_input_clear(&app->pin_entry);
    drum_input_init(&app->drum_entry, 6);
}

void app_state_switch_view(app_state *app, app_view new_view) {
    if (!app) return;
    if (app->current_view == VIEW_VAULT_READER && new_view != VIEW_VAULT_READER) {
        /* Exiting vault view: purge sensitive RAM buffers immediately */
        app_state_zeroize_vault(app);
    }
    app->previous_view = app->current_view;
    app->current_view = new_view;
    input_reset_inactivity(&app->input);
}

void app_state_cycle_theme(app_state *app) {
    if (!app) return;
    app->current_theme = (app_theme)((app->current_theme + 1) % THEME_COUNT);
}
