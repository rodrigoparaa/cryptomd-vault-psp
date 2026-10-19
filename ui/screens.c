#include "screens.h"
#include "graphics.h"
#include "theme_engine.h"
#include "font_7seg.h"
#include "../core/cipher_classic.h"
#include "../platform/psp_power.h"
#include <stdio.h>
#include <string.h>

static void draw_header(const app_state *app, const theme_palette *pal, const char *title) {
    graphics_fill_rect(0, 0, SCREEN_WIDTH, 20, pal->header_bg);
    graphics_draw_line(0, 20, SCREEN_WIDTH, 20, pal->accent_primary);

    graphics_draw_string(title, 12, 6, pal->accent_primary, 1);

    char time_buf[16];
    psp_power_format_time(time_buf, sizeof(time_buf));
    graphics_draw_string(time_buf, 370, 6, pal->text_color, 1);

    int bat = psp_power_get_battery_percent();
    char bat_buf[16];
    snprintf(bat_buf, sizeof(bat_buf), "%d%%", bat);
    graphics_draw_string(bat_buf, 420, 6, (bat < 20) ? pal->alert_danger : pal->code_text, 1);
    (void)app;
}

static void draw_footer(const app_state *app, const theme_palette *pal, const char *shortcuts, const char *status) {
    graphics_fill_rect(0, 252, SCREEN_WIDTH, 20, pal->footer_bg);
    graphics_draw_line(0, 252, SCREEN_WIDTH, 252, pal->container_bg);

    if (status) {
        if (shortcuts) {
            graphics_draw_string_clipped(shortcuts, 12, 258, pal->text_dim, 1, 336);
        }
        graphics_draw_string(status, 344, 258, pal->accent_primary, 1);
    } else {
        if (shortcuts) {
            graphics_draw_string_clipped(shortcuts, 12, 258, pal->text_dim, 1, 468);
        }
    }
    (void)app;
}

static void render_explorer(const app_state *app, const theme_palette *pal) {
    draw_header(app, pal, "EXPLORADOR DOCUMENTAL");

    char vol_info[320];
    snprintf(vol_info, sizeof(vol_info), "Directorio: %s (%zu archivos)", app->current_dir, app->file_count);
    graphics_draw_string_clipped(vol_info, 16, 26, pal->text_dim, 1, 464);

    int start_y = 44;
    int line_h = 16;
    size_t visible_count = 12;

    size_t scroll_offset = 0;
    if (app->selected_file_idx >= visible_count) {
        scroll_offset = app->selected_file_idx - visible_count + 1;
    }

    if (app->file_count == 0) {
        graphics_draw_string("No se encontraron documentos en esta carpeta.", 30, 100, pal->text_dim, 1);
        graphics_draw_string("Conecte la PSP por USB y copie archivos a ef0:/DOCS o ms0:/DOCS", 30, 120, pal->text_color, 1);
    } else {
        for (size_t i = 0; i < visible_count && (scroll_offset + i) < app->file_count; i++) {
            size_t idx = scroll_offset + i;
            const file_item *item = &app->files[idx];
            int y = start_y + (int)i * line_h;

            if (idx == app->selected_file_idx) {
                graphics_fill_rect(12, y - 2, 456, line_h, pal->container_bg);
                graphics_draw_rect(12, y - 2, 456, line_h, pal->accent_primary);
            }

            /* Badge */
            const char *badge = "[FILE]";
            uint32_t badge_col = pal->text_dim;
            if (item->is_pdoc) {
                switch (item->cipher_level) {
                    case 1: badge = "[N1-CESAR]"; badge_col = pal->badge_pdoc; break;
                    case 2: badge = "[N2-VIG]";   badge_col = pal->badge_pdoc; break;
                    case 3: badge = "[N3-AES]";   badge_col = pal->alert_success; break;
                    case 4: badge = "[N4-CHACHA]";badge_col = pal->accent_primary; break;
                    default: badge = "[PDOC]";     badge_col = pal->badge_pdoc; break;
                }
            } else {
                const char *dot = strrchr(item->name, '.');
                if (dot && strcmp(dot, ".md") == 0) {
                    badge = "[MD]";
                    badge_col = pal->badge_plain;
                } else {
                    badge = "[TXT]";
                    badge_col = pal->text_dim;
                }
            }

            graphics_draw_string(badge, 18, y + 2, badge_col, 1);
            graphics_draw_string_clipped(item->name, 110, y + 2,
                                         (idx == app->selected_file_idx) ? pal->accent_primary : pal->text_color,
                                         1, 396);

            char sz_buf[16];
            if (item->size > 1024) {
                snprintf(sz_buf, sizeof(sz_buf), "%lu KB", (unsigned long)(item->size / 1024));
            } else {
                snprintf(sz_buf, sizeof(sz_buf), "%lu B", (unsigned long)item->size);
            }
            graphics_draw_string(sz_buf, 410, y + 2, pal->text_dim, 1);
        }
    }

    draw_footer(app, pal, "[X] Abrir/Descifrar  [/\\] Cifrar  [SELECT] Cambiar ef0/ms0", NULL);
}

static void render_document_viewport(const md_document *doc, float scroll_y, const theme_palette *pal, int is_xray) {
    int vp_top = 22;
    int vp_bottom = 250;

    for (size_t i = 0; i < doc->line_count; i++) {
        const md_render_line *line = &doc->lines[i];
        int draw_y = vp_top + (int)line->y_offset - (int)scroll_y;

        if (draw_y + line->height < vp_top || draw_y > vp_bottom) {
            continue; /* Viewport clipping */
        }

        switch (line->type) {
            case MD_LINE_H1:
                graphics_fill_rect(16, draw_y, 448, line->height, pal->container_bg);
                graphics_draw_string(line->text, 20, draw_y + 2, pal->accent_primary, 2);
                break;
            case MD_LINE_H2:
                graphics_draw_string(line->text, 18, draw_y + 1, pal->accent_secondary, 2);
                break;
            case MD_LINE_H3:
                graphics_draw_string(line->text, 18, draw_y + 2, pal->accent_primary, 1);
                break;
            case MD_LINE_CODE:
                graphics_fill_rect(16, draw_y, 448, line->height, pal->container_bg);
                graphics_draw_string(line->text, 24, draw_y + 2, pal->code_text, 1);
                break;
            case MD_LINE_QUOTE:
                graphics_fill_rect(16, draw_y, 3, line->height, pal->accent_primary);
                graphics_draw_string(line->text, 26, draw_y + 2, pal->text_dim, 1);
                break;
            case MD_LINE_BULLET:
                graphics_fill_rect(20, draw_y + 4, 4, 4, pal->accent_primary);
                graphics_draw_string(line->text, 32, draw_y + 2, pal->text_color, 1);
                break;
            case MD_LINE_BULLET_CONT:
                graphics_draw_string(line->text, 32, draw_y + 2, pal->text_color, 1);
                break;
            case MD_LINE_HR:
                graphics_draw_line(16, draw_y + 5, 464, draw_y + 5, pal->container_bg);
                break;
            case MD_LINE_EMPTY:
                break;
            case MD_LINE_TEXT:
            default:
                graphics_draw_string(line->text, 18, draw_y + 2, is_xray ? pal->badge_pdoc : pal->text_color, 1);
                break;
        }
    }
}

static void render_reader_plain(const app_state *app, const theme_palette *pal) {
    draw_header(app, pal, app->active_file_name);
    render_document_viewport(&app->doc_plain, app->scroll.scroll_y, pal, 0);
    draw_footer(app, pal, "[O] Volver  [START] Cifrar  [L/R] Pagina  [[]] Tema", NULL);
}

static void render_vault_reader(const app_state *app, const theme_palette *pal) {
    if (app->input.xray_active) {
        draw_header(app, pal, "RAYOS X: VISTA CIFRADA RAW");
        if (app->doc_cipher.line_count > 0) {
            render_document_viewport(&app->doc_cipher, app->scroll.scroll_y, pal, 1);
        } else {
            /* Raw hex dump */
            for (size_t row = 0; row < 12 && (row * 16) < app->cipher_len; row++) {
                char hex_str[64];
                size_t offset = row * 16;
                snprintf(hex_str, sizeof(hex_str), "%04zX: %02X %02X %02X %02X %02X %02X %02X %02X ...",
                         offset,
                         app->cipher_buf[offset], app->cipher_buf[offset+1],
                         app->cipher_buf[offset+2], app->cipher_buf[offset+3],
                         app->cipher_buf[offset+4], app->cipher_buf[offset+5],
                         app->cipher_buf[offset+6], app->cipher_buf[offset+7]);
                graphics_draw_string(hex_str, 20, 36 + (int)row * 16, pal->code_text, 1);
            }
        }
        draw_footer(app, pal, "Soltar SELECT para volver a texto descifrado", "*** RAYOS X ***");
    } else {
        char title[80];
        snprintf(title, sizeof(title), "BOVEDA RAM: %s", app->active_file_name);
        draw_header(app, pal, title);
        render_document_viewport(&app->doc_plain, app->scroll.scroll_y, pal, 0);
        draw_footer(app, pal, "[O] Salir  [START] Guardar Descifrado  [HOLD SELECT] Rayos X", NULL);
    }
}

static void render_crypto_wizard(const app_state *app, const theme_palette *pal) {
    draw_header(app, pal, "ASISTENTE DE CIFRADO: ELEGIR NIVEL");

    graphics_draw_string("Selecciona el algoritmo de cifrado para este documento:", 20, 26, pal->text_color, 1);

    const char *names[4] = {
        "Nivel 1: Cesar (ROT-N Estructural)",
        "Nivel 2: Vigenere Autonomo (Palabra Clave)",
        "Nivel 3: AES-256-CBC (Estandar PC / OpenSSL)",
        "Nivel 4: ChaCha20-256 (Grado Militar MIPS ARX)"
    };
    const char *descs[4] = {
        "Ofuscacion clasica por rotacion de letras. Conserva formato Markdown.",
        "Cifrado polialfabetico con palabra clave. Resiste analisis simple.",
        "Seguridad corporativa estandar con relleno PKCS#7 y firma HMAC.",
        "Maximo rendimiento y robustez de 32 bits. Verify-then-Decrypt."
    };

    int card_y = 44;
    int card_h = 46;

    for (int lvl = 1; lvl <= 4; lvl++) {
        int y = card_y + (lvl - 1) * (card_h + 4);
        int is_sel = (app->wizard_target_level == lvl);

        graphics_fill_rect(20, y, 440, card_h, pal->container_bg);
        graphics_draw_rect(20, y, 440, card_h, is_sel ? pal->accent_primary : pal->container_bg);

        if (is_sel) {
            graphics_fill_rect(20, y, 6, card_h, pal->accent_primary);
        }

        graphics_draw_string(names[lvl - 1], 34, y + 6, is_sel ? pal->accent_primary : pal->text_color, 1);
        graphics_draw_string_clipped(descs[lvl - 1], 34, y + 24, pal->text_dim, 1, 450);
    }

    draw_footer(app, pal, "[Arriba/Abajo] Elegir  [X] Siguiente  [O] Cancelar", NULL);
}

static void render_key_len_select(const app_state *app, const theme_palette *pal) {
    draw_header(app, pal, "LONGITUD DE LA CLAVE DE BOTONES");

    graphics_draw_string("Elige cuantas teclas tendra tu combinacion secreta:", 20, 30, pal->text_color, 1);

    const uint8_t lengths[3] = { 2, 4, 6 };
    const char *titles[3] = {
        "2 Teclas (Combinacion Corta)",
        "4 Teclas (Combinacion Equilibrada)",
        "6 Teclas (Combinacion de Maxima Seguridad)"
    };
    const char *subtitles[3] = {
        "Acceso rapido con 2 pulsaciones fisicas.",
        "Balance ideal entre agilidad y proteccion criptografica.",
        "Alta entropia fisica derivada con 10.000 rondas PBKDF2-SHA256."
    };

    int card_y = 54;
    int card_h = 54;

    for (int i = 0; i < 3; i++) {
        int y = card_y + i * (card_h + 8);
        int is_sel = (app->selected_pin_len == lengths[i]);

        graphics_fill_rect(24, y, 432, card_h, pal->container_bg);
        graphics_draw_rect(24, y, 432, card_h, is_sel ? pal->accent_primary : pal->text_dim);

        if (is_sel) {
            graphics_fill_rect(24, y, 6, card_h, pal->accent_primary);
        }

        graphics_draw_string(titles[i], 40, y + 10, is_sel ? pal->accent_primary : pal->text_color, 1);
        graphics_draw_string(subtitles[i], 40, y + 30, pal->text_dim, 1);
    }

    draw_footer(app, pal, "[Arriba/Abajo] Elegir 2, 4 o 6 Teclas  [X] Continuar  [O] Volver", NULL);
}

static void draw_preview_box(const app_state *app, const theme_palette *pal, int is_vigenere) {
    graphics_draw_string("Previsualizacion en vivo del texto cifrado:", 24, 114, pal->accent_secondary, 1);
    graphics_fill_rect(24, 130, 432, 96, pal->container_bg);
    graphics_draw_rect(24, 130, 432, 96, pal->accent_primary);

    if (app->plain_len == 0) return;

    char clean_src[256];
    size_t raw_take = (app->plain_len > 220) ? 220 : app->plain_len;
    size_t clean_len = md_sanitize_utf8((const char *)app->plain_buf, raw_take, clean_src, sizeof(clean_src));

    /* Replace newlines/tabs with spaces for compact 3-line preview */
    for (size_t i = 0; i < clean_len; i++) {
        if (clean_src[i] == '\n' || clean_src[i] == '\r' || clean_src[i] == '\t') {
            clean_src[i] = ' ';
        }
    }

    char enc_buf[256];
    if (is_vigenere) {
        vigenere_encrypt(clean_src, clean_len, app->drum_entry.word, enc_buf);
    } else {
        caesar_encrypt(clean_src, clean_len, app->caesar_shift, enc_buf);
    }
    enc_buf[clean_len] = '\0';

    /* Draw up to 4 wrapped lines of 50 chars inside the preview box */
    size_t pos = 0;
    for (int line = 0; line < 4 && pos < clean_len; line++) {
        while (pos < clean_len && enc_buf[pos] == ' ') pos++;
        if (pos >= clean_len) break;

        char row_str[52];
        size_t rem = clean_len - pos;
        size_t take = (rem > 50) ? 50 : rem;
        memcpy(row_str, enc_buf + pos, take);
        row_str[take] = '\0';

        graphics_draw_string(row_str, 34, 140 + line * 18, pal->code_text, 1);
        pos += take;
    }
}

static void render_key_entry(const app_state *app, const theme_palette *pal) {
    if (app->wizard_target_level == PDOC_LEVEL_CESAR) {
        if (app->key_entry_for_encrypt) {
            draw_header(app, pal, "CIFRADO CESAR: ELEGIR CLAVE");
            graphics_draw_string("Selecciona el numero de clave para cifrar (1 a 25):", 24, 28, pal->text_color, 1);

            /* Centered 7-Segment dial above preview */
            int dial_w = 2 * 26 + 8;
            int dial_x = (SCREEN_WIDTH - dial_w) / 2;
            graphics_fill_rect(dial_x - 18, 44, dial_w + 36, 60, pal->container_bg);
            graphics_draw_rect(dial_x - 18, 44, dial_w + 36, 60, pal->accent_primary);
            font_7seg_draw_number(app->caesar_shift, dial_x, 50, 26, 48, 8, pal->accent_primary);

            draw_preview_box(app, pal, 0);
            draw_footer(app, pal, "[Izq/Der o L/R] Cambiar Numero  [X] Cifrar  [O] Volver", NULL);
        } else {
            /* Dedicated Caesar Decryption Screen: Giant Number in Center of Screen */
            draw_header(app, pal, "DESCIFRAR DOCUMENTO (CESAR)");

            const char *q_msg = "\x8E" "Que numero de clave quieres utilizar para descifrarlo?";
            graphics_draw_string(q_msg, 20, 34, pal->text_color, 1);
            graphics_draw_string("Usa Izquierda/Derecha o L/R para elegir el numero (1 a 25):", 20, 52, pal->text_dim, 1);

            /* Giant centered dial card in the middle of the screen */
            int digit_w = 48;
            int digit_h = 92;
            int spacing = 14;
            int total_num_w = 2 * digit_w + spacing;
            int num_x = (SCREEN_WIDTH - total_num_w) / 2;
            int num_y = 92;

            int box_w = 220;
            int box_h = 120;
            int box_x = (SCREEN_WIDTH - box_w) / 2;
            int box_y = 78;

            graphics_fill_rect(box_x, box_y, box_w, box_h, pal->container_bg);
            graphics_draw_rect(box_x, box_y, box_w, box_h, pal->accent_primary);
            graphics_draw_rect(box_x + 2, box_y + 2, box_w - 4, box_h - 4, pal->container_bg);

            /* Left / Right indicator arrows */
            graphics_draw_string("<", box_x + 14, box_y + 52, pal->accent_secondary, 2);
            graphics_draw_string(">", box_x + box_w - 30, box_y + 52, pal->accent_secondary, 2);

            font_7seg_draw_number(app->caesar_shift, num_x, num_y, digit_w, digit_h, spacing, pal->accent_primary);

            if (app->auth_error) {
                graphics_draw_string("\x8F" "Clave incorrecta! Prueba con otro numero de desplazamiento.", 20, 216, pal->alert_danger, 1);
            } else {
                graphics_draw_string("Pulsa [X] para descifrar el archivo con este numero.", 32, 216, pal->accent_secondary, 1);
            }

            draw_footer(app, pal, "[Izq/Der o L/R] Elegir Numero  [X] Descifrar  [O] Volver", NULL);
        }
    }
    else if (app->wizard_target_level == PDOC_LEVEL_VIGENERE) {
        draw_header(app, pal, app->key_entry_for_encrypt ? "CIFRADO VIGENERE: PALABRA CLAVE" : "DESCIFRAR DOCUMENTO (VIGENERE)");
        graphics_draw_string("Introduce la palabra clave alfabetica con el tambor:", 24, 30, pal->text_color, 1);

        /* Drum slots centered */
        int total_w = (int)app->drum_entry.length * 28 - 4;
        int start_x = (SCREEN_WIDTH - total_w) / 2;
        int drum_y = app->key_entry_for_encrypt ? 50 : 90;

        for (size_t i = 0; i < app->drum_entry.length; i++) {
            int slot_x = start_x + (int)i * 28;
            int is_cur = (app->drum_entry.cursor == i);

            graphics_fill_rect(slot_x, drum_y, 24, 36, pal->container_bg);
            graphics_draw_rect(slot_x, drum_y, 24, 36, is_cur ? pal->accent_primary : pal->text_dim);

            char ch[2] = { app->drum_entry.word[i], '\0' };
            graphics_draw_string(ch, slot_x + 4, drum_y + 10, is_cur ? pal->accent_primary : pal->text_color, 2);
        }

        if (app->key_entry_for_encrypt) {
            draw_preview_box(app, pal, 1);
        } else if (app->auth_error) {
            graphics_draw_string("\x8F" "Palabra clave incorrecta! Verifica las letras e intentalo de nuevo.", 16, 160, pal->alert_danger, 1);
        } else {
            graphics_draw_string("Ajusta cada letra y pulsa [X] para descifrar el documento.", 20, 160, pal->accent_secondary, 1);
        }

        draw_footer(app, pal, "[Arriba/Abajo] Letra  [Izq/Der] Casilla  [X] Confirmar  [O] Volver", NULL);
    }
    else {
        /* Level 3 & 4: Button Combo PIN */
        draw_header(app, pal, app->key_entry_for_encrypt ? "CONFIGURAR CLAVE DE BOTONES" : "DESBLOQUEAR DOCUMENTO");

        char prompt_buf[80];
        if (app->key_entry_for_encrypt) {
            snprintf(prompt_buf, sizeof(prompt_buf), "Introduce una combinacion secreta de %zu teclas:", app->pin_entry.max_len);
        } else {
            snprintf(prompt_buf, sizeof(prompt_buf), "Introduce la secuencia fisica de %zu teclas para desbloquear:", app->pin_entry.max_len);
        }
        graphics_draw_string(prompt_buf, 20, 30, pal->text_color, 1);

        /* Center the PIN boxes according to max_len (2, 4, or 6) */
        int total_slots_w = (int)app->pin_entry.max_len * 40 - 8;
        int start_x = (SCREEN_WIDTH - total_slots_w) / 2;

        for (size_t i = 0; i < app->pin_entry.max_len; i++) {
            int slot_x = start_x + (int)i * 40;
            int filled = (i < app->pin_entry.count);

            graphics_fill_rect(slot_x, 56, 32, 32, pal->container_bg);
            graphics_draw_rect(slot_x, 56, 32, 32, filled ? pal->accent_primary : pal->text_dim);

            if (filled) {
                graphics_fill_rect(slot_x + 9, 65, 14, 14, pal->accent_primary);
            }
        }

        graphics_draw_string("Controles admitidos: L, R, Cruceta y Botones geometricos", 20, 102, pal->text_dim, 1);

        /* Prominent SELECT instruction box */
        int is_complete = (app->pin_entry.count == app->pin_entry.max_len);
        uint32_t box_border = is_complete ? pal->alert_success : pal->accent_primary;
        graphics_fill_rect(20, 124, 440, 56, pal->container_bg);
        graphics_draw_rect(20, 124, 440, 56, box_border);

        if (is_complete) {
            graphics_fill_rect(20, 124, 6, 56, pal->alert_success);
            if (app->key_entry_for_encrypt) {
                graphics_draw_string("\x8F" "Combinacion completa! Pulsa [SELECT] para guardar la combinacion", 34, 136, pal->alert_success, 1);
            } else {
                graphics_draw_string("\x8F" "Combinacion completa! Pulsa [SELECT] para desbloquear", 34, 136, pal->alert_success, 1);
            }
            graphics_draw_string("O pulsa [O] si deseas borrar y corregir la ultima tecla.", 34, 156, pal->text_color, 1);
        } else {
            char prog_buf[80];
            snprintf(prog_buf, sizeof(prog_buf), "Teclas introducidas: %zu de %zu", app->pin_entry.count, app->pin_entry.max_len);
            graphics_draw_string(prog_buf, 34, 134, pal->accent_primary, 1);
            graphics_draw_string("Una vez introducida la combinacion, pulsa [SELECT] para guardar", 34, 154, pal->text_color, 1);
        }

        if (app->auth_error) {
            graphics_draw_string("\x8F" "Combinacion incorrecta! Intentalo de nuevo.", 20, 196, pal->alert_danger, 1);
        } else {
            graphics_draw_string("Clave procesada en RAM con PBKDF2-SHA256 y firma HMAC.", 20, 196, pal->text_dim, 1);
        }

        draw_footer(app, pal, "[Botones] Ingresar  [O] Borrar/Salir  [SELECT] Guardar Combinacion", NULL);
    }
}

static void render_commit_modal(const app_state *app, const theme_palette *pal) {
    /* Dim background */
    graphics_fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_RGBA(0, 0, 0, 180));

    /* Wider Modal dialog box (448px wide) so all text fits with generous margin */
    int mx = 16, my = 46, mw = 448, mh = 176;
    graphics_fill_rect(mx, my, mw, mh, pal->container_bg);
    graphics_draw_rect(mx, my, mw, mh, pal->accent_primary);

    graphics_draw_string("CONFIRMAR OPERACION EN DISCO", mx + 16, my + 16, pal->accent_primary, 1);
    graphics_draw_line(mx + 16, my + 30, mx + mw - 16, my + 30, pal->text_dim);

    graphics_draw_string("El documento ha sido cifrado con exito.", mx + 16, my + 44, pal->text_color, 1);
    graphics_draw_string("\x8E" "Que accion deseas aplicar sobre el archivo original?", mx + 16, my + 62, pal->text_dim, 1);

    /* Options */
    graphics_draw_string("[X] Destruir Original (Shredder seguro FAT32)", mx + 20, my + 92, pal->alert_danger, 1);
    graphics_draw_string("[Triangulo] Conservar Copia en Claro", mx + 20, my + 116, pal->accent_secondary, 1);
    graphics_draw_string("[O] Volver al Explorador", mx + 20, my + 140, pal->text_dim, 1);
    (void)app;
}

void screens_render(const app_state *app) {
    if (!app) return;
    const theme_palette *pal = theme_get(app->current_theme);

    graphics_clear(pal->bg_color);

    switch (app->current_view) {
        case VIEW_EXPLORER:
            render_explorer(app, pal);
            break;
        case VIEW_READER_PLAIN:
            render_reader_plain(app, pal);
            break;
        case VIEW_VAULT_READER:
            render_vault_reader(app, pal);
            break;
        case VIEW_CRYPTO_WIZARD:
            render_crypto_wizard(app, pal);
            break;
        case VIEW_KEY_LEN_SELECT:
            render_key_len_select(app, pal);
            break;
        case VIEW_KEY_ENTRY:
            render_key_entry(app, pal);
            break;
        case VIEW_COMMIT_MODAL:
            render_commit_modal(app, pal);
            break;
    }
}
