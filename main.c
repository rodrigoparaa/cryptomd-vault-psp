#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/crypto_primitives.h"
#include "core/cipher_classic.h"
#include "core/cipher_aes256.h"
#include "core/cipher_chacha20.h"
#include "core/pdoc_container.h"
#include "core/md_parser.h"
#include "core/scroll_physics.h"
#include "core/input_logic.h"
#include "core/app_state.h"

#include "platform/psp_entropy.h"
#include "platform/psp_fs.h"
#include "platform/psp_input.h"
#include "platform/psp_power.h"

#include "ui/graphics.h"
#include "ui/theme_engine.h"
#include "ui/screens.h"

#ifdef __PSP__
#include <pspkernel.h>
#include <pspctrl.h>

PSP_MODULE_INFO("CryptoMDVault", 0, 1, 1);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);
PSP_HEAP_SIZE_KB(20 * 1024);

static int s_exit_request = 0;

static int exit_callback(int arg1, int arg2, void *common) {
    (void)arg1; (void)arg2; (void)common;
    s_exit_request = 1;
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
}
#else
static int s_exit_request = 0;
static void setup_callbacks(void) {}
#endif

/* Global application state allocated in .bss (not stack) */
static app_state g_app;

static uint8_t detect_file_format(const char *filename) {
    if (!filename) return PDOC_FMT_TXT;
    const char *dot = strrchr(filename, '.');
    if (dot && (strcmp(dot, ".md") == 0 || strcmp(dot, ".MD") == 0)) {
        return PDOC_FMT_MD;
    }
    return PDOC_FMT_TXT;
}

static void strip_known_extensions(const char *filename, char *base_out, size_t max_len) {
    if (!filename || !base_out || max_len == 0) return;

    /* Skip legacy "restored_" prefix if present */
    if (strncmp(filename, "restored_", 9) == 0 && filename[9] != '\0') {
        filename += 9;
    }

    strncpy(base_out, filename, max_len - 1);
    base_out[max_len - 1] = '\0';

    /* Strip trailing .pdoc if present */
    size_t len = strlen(base_out);
    if (len > 5 && (strcmp(base_out + len - 5, ".pdoc") == 0 || strcmp(base_out + len - 5, ".PDOC") == 0)) {
        base_out[len - 5] = '\0';
        len -= 5;
    }

    /* Strip trailing .txt or .md if present so we have the clean stem */
    if (len > 4 && (strcmp(base_out + len - 4, ".txt") == 0 || strcmp(base_out + len - 4, ".TXT") == 0)) {
        base_out[len - 4] = '\0';
    } else if (len > 3 && (strcmp(base_out + len - 3, ".md") == 0 || strcmp(base_out + len - 3, ".MD") == 0)) {
        base_out[len - 3] = '\0';
    }
}

static void build_encrypted_path(const char *dir, const char *orig_name, char *out_path, size_t out_max) {
    char stem[64];
    strip_known_extensions(orig_name, stem, sizeof(stem));
    snprintf(out_path, out_max, "%s/%s.pdoc", dir, stem);
}

static void build_restored_path(const char *dir, const char *pdoc_name, uint8_t orig_format, char *out_path, size_t out_max) {
    /* Also detect if legacy filename had .md.pdoc before stripping */
    uint8_t fmt = orig_format;
    size_t nlen = strlen(pdoc_name);
    if (nlen > 8 && (strcmp(pdoc_name + nlen - 8, ".md.pdoc") == 0 || strcmp(pdoc_name + nlen - 8, ".MD.PDOC") == 0)) {
        fmt = PDOC_FMT_MD;
    }

    char stem[64];
    strip_known_extensions(pdoc_name, stem, sizeof(stem));
    const char *ext = (fmt == PDOC_FMT_MD) ? ".md" : ".txt";
    snprintf(out_path, out_max, "%s/%s%s", dir, stem, ext);
}

static void execute_encryption(app_state *app) {
    uint8_t salt[16];
    uint8_t iv_nonce[16];
    psp_entropy_gather(salt, 16);
    psp_entropy_gather(iv_nonce, 16);

    uint8_t key_data[64];
    size_t key_len = 0;
    uint8_t key_mode = PDOC_KEYMODE_COMBO;

    if (app->wizard_target_level == PDOC_LEVEL_CESAR) {
        key_mode = PDOC_KEYMODE_NUMBER;
        key_data[0] = app->caesar_shift;
        key_len = 1;
    } else if (app->wizard_target_level == PDOC_LEVEL_VIGENERE) {
        key_mode = PDOC_KEYMODE_WORD;
        key_len = app->drum_entry.length;
        memcpy(key_data, app->drum_entry.word, key_len);
    } else {
        key_mode = PDOC_KEYMODE_COMBO;
        key_len = app->pin_entry.count;
        memcpy(key_data, app->pin_entry.buttons, key_len);
    }

    size_t out_len = 0;
    int pack_res = pdoc_pack_encrypt(app->wizard_target_level, app->active_format, key_mode,
                                     key_data, key_len, salt, iv_nonce,
                                     psp_power_get_utc_seconds(),
                                     app->plain_buf, app->plain_len,
                                     app->cipher_buf, &out_len);

    crypto_zeroize(key_data, sizeof(key_data));
    crypto_zeroize(salt, sizeof(salt));
    crypto_zeroize(iv_nonce, sizeof(iv_nonce));

    if (pack_res == PDOC_OK) {
        char target_pdoc[512];
        build_encrypted_path(app->current_dir, app->active_file_name, target_pdoc, sizeof(target_pdoc));
        psp_fs_write_atomic(target_pdoc, app->cipher_buf, out_len);
        app_state_switch_view(app, VIEW_COMMIT_MODAL);
    } else {
        app_state_switch_view(app, VIEW_EXPLORER);
    }
}

static void execute_decryption(app_state *app) {
    uint8_t key_data[64];
    size_t key_len = 0;

    if (app->active_header.cipher_level == PDOC_LEVEL_CESAR) {
        key_data[0] = app->caesar_shift;
        key_len = 1;
    } else if (app->active_header.cipher_level == PDOC_LEVEL_VIGENERE) {
        key_len = app->drum_entry.length;
        memcpy(key_data, app->drum_entry.word, key_len);
    } else {
        key_len = app->pin_entry.count;
        memcpy(key_data, app->pin_entry.buttons, key_len);
    }

    size_t plain_out_len = 0;
    int dec_res = pdoc_verify_decrypt(app->cipher_buf, app->cipher_len,
                                      key_data, key_len,
                                      app->plain_buf, &plain_out_len,
                                      &app->active_header);

    crypto_zeroize(key_data, sizeof(key_data));

    if (dec_res == PDOC_OK) {
        app->auth_error = 0;
        app->plain_len = plain_out_len;
        app->plain_buf[plain_out_len] = '\0';
        md_parse_document((const char *)app->plain_buf, plain_out_len, &app->doc_plain);

        /* Format cipher view for Rayos X mode */
        if (app->active_header.cipher_level <= PDOC_LEVEL_VIGENERE) {
            md_parse_document((const char *)app->cipher_buf + PDOC_HEADER_SIZE,
                              app->active_header.cipher_len, &app->doc_cipher);
        } else {
            app->doc_cipher.line_count = 0;
            app->doc_cipher.total_height = 0;
        }

        scroll_init(&app->scroll, (float)app->doc_plain.total_height, MD_VIEWPORT_HEIGHT);
        app_state_switch_view(app, VIEW_VAULT_READER);
    } else {
        /* Failed authentication / wrong key: stay in key entry and alert user */
        app->auth_error = 1;
        if (app->active_header.cipher_level >= PDOC_LEVEL_AES256) {
            pin_input_clear(&app->pin_entry);
        }
    }
}

int main(void) {
    setup_callbacks();
    psp_power_init();
    psp_input_init();
    graphics_init();

    app_state_init(&g_app);

    /* Initial scan of ef0:/DOCS, fallback to ms0:/DOCS */
    if (psp_fs_scan_directory(&g_app, "ef0:/DOCS") != 0) {
        psp_fs_scan_directory(&g_app, "ms0:/DOCS");
        g_app.is_ms0_active = 1;
    }

    int8_t analog_x = 0;
    int8_t analog_y = 0;

    while (!s_exit_request) {
        psp_power_tick();
        psp_input_poll(&g_app, &analog_x, &analog_y);

        /* Global Theme Cycling: Square in Explorer */
        if (g_app.current_view == VIEW_EXPLORER && (g_app.input.pressed_buttons & BTN_SQUARE)) {
            app_state_cycle_theme(&g_app);
        }

        /* View-specific state logic */
        switch (g_app.current_view) {
            case VIEW_EXPLORER:
                if (g_app.input.pressed_buttons & BTN_UP) {
                    if (g_app.selected_file_idx > 0) g_app.selected_file_idx--;
                } else if (g_app.input.pressed_buttons & BTN_DOWN) {
                    if (g_app.selected_file_idx + 1 < g_app.file_count) g_app.selected_file_idx++;
                } else if (g_app.input.pressed_buttons & BTN_SELECT) {
                    g_app.is_ms0_active = !g_app.is_ms0_active;
                    psp_fs_scan_directory(&g_app, g_app.is_ms0_active ? "ms0:/DOCS" : "ef0:/DOCS");
                } else if (g_app.input.pressed_buttons & BTN_CROSS) {
                    if (g_app.file_count > 0) {
                        file_item *item = &g_app.files[g_app.selected_file_idx];
                        strncpy(g_app.active_file_path, item->full_path, sizeof(g_app.active_file_path) - 1);
                        g_app.active_file_path[sizeof(g_app.active_file_path) - 1] = '\0';
                        strncpy(g_app.active_file_name, item->name, sizeof(g_app.active_file_name) - 1);
                        g_app.active_file_name[sizeof(g_app.active_file_name) - 1] = '\0';

                        if (item->is_pdoc) {
                            /* Encrypted file: read container and prompt key challenge */
                            size_t rd = 0;
                            if (psp_fs_read_file(item->full_path, g_app.cipher_buf, sizeof(g_app.cipher_buf), &rd) == 0) {
                                g_app.cipher_len = rd;
                                pdoc_header hdr;
                                if (pdoc_is_valid_header(g_app.cipher_buf, rd, &hdr)) {
                                    g_app.active_header = hdr;
                                    g_app.active_format = hdr.orig_format;
                                    g_app.wizard_target_level = hdr.cipher_level;
                                    g_app.key_entry_for_encrypt = 0;
                                    g_app.auth_error = 0;
                                    g_app.caesar_shift = 1;
                                    size_t hint_len = (hdr.key_len_hint >= 2) ? hdr.key_len_hint : 4;
                                    pin_input_init(&g_app.pin_entry, hint_len);
                                    drum_input_init(&g_app.drum_entry, (hdr.key_len_hint >= 1) ? hdr.key_len_hint : 6);
                                    app_state_switch_view(&g_app, VIEW_KEY_ENTRY);
                                }
                            }
                        } else {
                            /* Plain text: read and open in plain reader */
                            size_t rd = 0;
                            if (psp_fs_read_file(item->full_path, g_app.plain_buf, PDOC_MAX_DOC_SIZE, &rd) == 0) {
                                g_app.plain_len = rd;
                                g_app.plain_buf[rd] = '\0';
                                g_app.active_format = detect_file_format(item->name);
                                md_parse_document((const char *)g_app.plain_buf, rd, &g_app.doc_plain);
                                scroll_init(&g_app.scroll, (float)g_app.doc_plain.total_height, MD_VIEWPORT_HEIGHT);
                                app_state_switch_view(&g_app, VIEW_READER_PLAIN);
                            }
                        }
                    }
                } else if (g_app.input.pressed_buttons & BTN_TRIANGLE) {
                    /* Encrypt selected file */
                    if (g_app.file_count > 0 && !g_app.files[g_app.selected_file_idx].is_pdoc) {
                        file_item *item = &g_app.files[g_app.selected_file_idx];
                        strncpy(g_app.active_file_path, item->full_path, sizeof(g_app.active_file_path) - 1);
                        g_app.active_file_path[sizeof(g_app.active_file_path) - 1] = '\0';
                        strncpy(g_app.active_file_name, item->name, sizeof(g_app.active_file_name) - 1);
                        g_app.active_file_name[sizeof(g_app.active_file_name) - 1] = '\0';
                        g_app.active_format = detect_file_format(item->name);
                        size_t rd = 0;
                        if (psp_fs_read_file(item->full_path, g_app.plain_buf, PDOC_MAX_DOC_SIZE, &rd) == 0) {
                            g_app.plain_len = rd;
                            g_app.plain_buf[rd] = '\0';
                            app_state_switch_view(&g_app, VIEW_CRYPTO_WIZARD);
                        }
                    }
                }
                break;

            case VIEW_READER_PLAIN:
                scroll_update(&g_app.scroll, analog_y);
                if (g_app.input.pressed_buttons & BTN_UP) scroll_step(&g_app.scroll, -18.0f);
                if (g_app.input.pressed_buttons & BTN_DOWN) scroll_step(&g_app.scroll, 18.0f);
                if (g_app.input.pressed_buttons & BTN_LTRIGGER) scroll_page_up(&g_app.scroll);
                if (g_app.input.pressed_buttons & BTN_RTRIGGER) scroll_page_down(&g_app.scroll);
                if (g_app.input.pressed_buttons & BTN_SQUARE) app_state_cycle_theme(&g_app);
                if (g_app.input.pressed_buttons & BTN_CIRCLE) app_state_switch_view(&g_app, VIEW_EXPLORER);
                if (g_app.input.pressed_buttons & BTN_START) app_state_switch_view(&g_app, VIEW_CRYPTO_WIZARD);
                break;

            case VIEW_VAULT_READER:
                /* Panic lock detector or auto-lock timeout */
                if (g_app.input.panic_triggered || g_app.input.inactivity_frames >= AUTO_LOCK_FRAMES) {
                    app_state_zeroize_vault(&g_app);
                    app_state_switch_view(&g_app, VIEW_EXPLORER);
                    break;
                }
                scroll_update(&g_app.scroll, analog_y);
                if (g_app.input.pressed_buttons & BTN_UP) scroll_step(&g_app.scroll, -18.0f);
                if (g_app.input.pressed_buttons & BTN_DOWN) scroll_step(&g_app.scroll, 18.0f);
                if (g_app.input.pressed_buttons & BTN_LTRIGGER) scroll_page_up(&g_app.scroll);
                if (g_app.input.pressed_buttons & BTN_RTRIGGER) scroll_page_down(&g_app.scroll);
                if (g_app.input.pressed_buttons & BTN_SQUARE) app_state_cycle_theme(&g_app);
                if (g_app.input.pressed_buttons & BTN_CIRCLE) {
                    app_state_zeroize_vault(&g_app);
                    app_state_switch_view(&g_app, VIEW_EXPLORER);
                }
                if (g_app.input.pressed_buttons & BTN_START) {
                    /* Restore decrypted file with original name and original extension (.md / .txt),
                     * and delete the encrypted .pdoc file so files do not accumulate */
                    char out_path[512];
                    build_restored_path(g_app.current_dir, g_app.active_file_name,
                                        g_app.active_header.orig_format, out_path, sizeof(out_path));
                    if (psp_fs_write_atomic(out_path, g_app.plain_buf, g_app.plain_len) == 0) {
                        if (strcmp(out_path, g_app.active_file_path) != 0) {
                            psp_fs_shred_file(g_app.active_file_path);
                        }
                    }
                    app_state_zeroize_vault(&g_app);
                    app_state_switch_view(&g_app, VIEW_EXPLORER);
                    psp_fs_scan_directory(&g_app, g_app.current_dir);
                }
                break;

            case VIEW_CRYPTO_WIZARD:
                if (g_app.input.pressed_buttons & BTN_UP) {
                    if (g_app.wizard_target_level > 1) g_app.wizard_target_level--;
                } else if (g_app.input.pressed_buttons & BTN_DOWN) {
                    if (g_app.wizard_target_level < 4) g_app.wizard_target_level++;
                } else if (g_app.input.pressed_buttons & BTN_CROSS) {
                    g_app.key_entry_for_encrypt = 1;
                    g_app.auth_error = 0;
                    g_app.caesar_shift = 1;
                    drum_input_init(&g_app.drum_entry, 6);
                    if (g_app.wizard_target_level >= PDOC_LEVEL_AES256) {
                        /* Ask user to select key length: 2, 4, or 6 buttons */
                        if (g_app.selected_pin_len != 2 && g_app.selected_pin_len != 4 && g_app.selected_pin_len != 6) {
                            g_app.selected_pin_len = 4;
                        }
                        app_state_switch_view(&g_app, VIEW_KEY_LEN_SELECT);
                    } else {
                        app_state_switch_view(&g_app, VIEW_KEY_ENTRY);
                    }
                } else if (g_app.input.pressed_buttons & BTN_CIRCLE) {
                    app_state_switch_view(&g_app, VIEW_EXPLORER);
                }
                break;

            case VIEW_KEY_LEN_SELECT:
                if (g_app.input.pressed_buttons & (BTN_UP | BTN_LEFT)) {
                    if (g_app.selected_pin_len == 6) g_app.selected_pin_len = 4;
                    else if (g_app.selected_pin_len == 4) g_app.selected_pin_len = 2;
                    else g_app.selected_pin_len = 6;
                } else if (g_app.input.pressed_buttons & (BTN_DOWN | BTN_RIGHT)) {
                    if (g_app.selected_pin_len == 2) g_app.selected_pin_len = 4;
                    else if (g_app.selected_pin_len == 4) g_app.selected_pin_len = 6;
                    else g_app.selected_pin_len = 2;
                } else if (g_app.input.pressed_buttons & BTN_CROSS) {
                    pin_input_init(&g_app.pin_entry, g_app.selected_pin_len);
                    g_app.auth_error = 0;
                    app_state_switch_view(&g_app, VIEW_KEY_ENTRY);
                } else if (g_app.input.pressed_buttons & BTN_CIRCLE) {
                    app_state_switch_view(&g_app, VIEW_CRYPTO_WIZARD);
                }
                break;

            case VIEW_KEY_ENTRY:
                if (g_app.wizard_target_level == PDOC_LEVEL_CESAR) {
                    if (g_app.input.pressed_buttons & (BTN_RIGHT | BTN_RTRIGGER)) {
                        g_app.caesar_shift = (g_app.caesar_shift % 25) + 1;
                        g_app.auth_error = 0;
                    } else if (g_app.input.pressed_buttons & (BTN_LEFT | BTN_LTRIGGER)) {
                        g_app.caesar_shift = (g_app.caesar_shift == 1) ? 25 : (g_app.caesar_shift - 1);
                        g_app.auth_error = 0;
                    } else if (g_app.input.pressed_buttons & BTN_CROSS) {
                        if (g_app.key_entry_for_encrypt) execute_encryption(&g_app);
                        else execute_decryption(&g_app);
                    } else if (g_app.input.pressed_buttons & BTN_CIRCLE) {
                        app_state_switch_view(&g_app, g_app.key_entry_for_encrypt ? VIEW_CRYPTO_WIZARD : VIEW_EXPLORER);
                    }
                } else if (g_app.wizard_target_level == PDOC_LEVEL_VIGENERE) {
                    if (g_app.input.pressed_buttons & BTN_UP) {
                        drum_input_cycle_char(&g_app.drum_entry, 1);
                        g_app.auth_error = 0;
                    } else if (g_app.input.pressed_buttons & BTN_DOWN) {
                        drum_input_cycle_char(&g_app.drum_entry, -1);
                        g_app.auth_error = 0;
                    } else if (g_app.input.pressed_buttons & BTN_RIGHT) {
                        drum_input_move_cursor(&g_app.drum_entry, 1);
                    } else if (g_app.input.pressed_buttons & BTN_LEFT) {
                        drum_input_move_cursor(&g_app.drum_entry, -1);
                    } else if (g_app.input.pressed_buttons & BTN_CROSS) {
                        if (g_app.key_entry_for_encrypt) execute_encryption(&g_app);
                        else execute_decryption(&g_app);
                    } else if (g_app.input.pressed_buttons & BTN_CIRCLE) {
                        app_state_switch_view(&g_app, g_app.key_entry_for_encrypt ? VIEW_CRYPTO_WIZARD : VIEW_EXPLORER);
                    }
                } else {
                    /* Button Combo PIN (Levels 3 & 4) */
                    if (g_app.input.pressed_buttons) {
                        if (g_app.input.pressed_buttons & BTN_CIRCLE) {
                            if (g_app.pin_entry.count > 0) {
                                pin_input_backspace(&g_app.pin_entry);
                                g_app.auth_error = 0;
                            } else {
                                app_state_switch_view(&g_app, g_app.key_entry_for_encrypt ? VIEW_KEY_LEN_SELECT : VIEW_EXPLORER);
                            }
                        } else if (g_app.input.pressed_buttons & (BTN_SELECT | BTN_START)) {
                            /* Confirm / Save combination with SELECT (or START) */
                            if (g_app.pin_entry.count == g_app.pin_entry.max_len) {
                                if (g_app.key_entry_for_encrypt) execute_encryption(&g_app);
                                else execute_decryption(&g_app);
                            }
                        } else {
                            /* Capture physical button ID */
                            uint8_t code = input_mask_to_code(g_app.input.pressed_buttons);
                            if (code != BTN_CODE_NONE && g_app.pin_entry.count < g_app.pin_entry.max_len) {
                                g_app.auth_error = 0;
                                pin_input_add(&g_app.pin_entry, code);
                            }
                        }
                    }
                }
                break;

            case VIEW_COMMIT_MODAL:
                if (g_app.input.pressed_buttons & BTN_CROSS) {
                    /* Destroy original plaintext with secure shredder */
                    psp_fs_shred_file(g_app.active_file_path);
                    psp_fs_scan_directory(&g_app, g_app.current_dir);
                    app_state_switch_view(&g_app, VIEW_EXPLORER);
                } else if (g_app.input.pressed_buttons & (BTN_TRIANGLE | BTN_CIRCLE)) {
                    /* Keep original */
                    psp_fs_scan_directory(&g_app, g_app.current_dir);
                    app_state_switch_view(&g_app, VIEW_EXPLORER);
                }
                break;
        }

        /* Render Frame */
        graphics_start_frame();
        screens_render(&g_app);
        graphics_end_frame();
    }

    app_state_zeroize_vault(&g_app);
    graphics_term();

#ifdef __PSP__
    sceKernelExitGame();
#endif
    return 0;
}
