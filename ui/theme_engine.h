#ifndef THEME_ENGINE_H
#define THEME_ENGINE_H

#include <stdint.h>
#include "../core/app_state.h"

/* 32-bit ABGR colors for PSP GU hardware (0xAABBGGRR) */
#define COLOR_RGBA(r, g, b, a) (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r))

typedef struct {
    const char *name;
    uint32_t bg_color;
    uint32_t text_color;
    uint32_t text_dim;
    uint32_t accent_primary;
    uint32_t accent_secondary;
    uint32_t container_bg;
    uint32_t code_text;
    uint32_t badge_pdoc;
    uint32_t badge_plain;
    uint32_t header_bg;
    uint32_t footer_bg;
    uint32_t alert_danger;
    uint32_t alert_success;
} theme_palette;

const theme_palette *theme_get(app_theme theme);

#endif /* THEME_ENGINE_H */
