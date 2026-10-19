#include "font_7seg.h"
#include "graphics.h"
#include "theme_engine.h"
#include <stdio.h>

void font_7seg_draw_char(char c, int x, int y, int w, int h, uint32_t color) {
    if (w < 4 || h < 8) return;
    int t = h / 8;
    if (t < 1) t = 1;
    int mid_y = y + (h - t) / 2;
    int bot_y = y + h - t;
    int right_x = x + w - t;
    int half_h = (h - 3 * t) / 2;

    uint8_t seg = 0; /* bit 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G */

    switch (c) {
        case '0': seg = 0x3F; break;
        case '1': seg = 0x06; break;
        case '2': seg = 0x5B; break;
        case '3': seg = 0x4F; break;
        case '4': seg = 0x66; break;
        case '5': seg = 0x6D; break;
        case '6': seg = 0x7D; break;
        case '7': seg = 0x07; break;
        case '8': seg = 0x7F; break;
        case '9': seg = 0x6F; break;
        case '-': seg = 0x40; break;
        case '+':
            /* Special cross symbol */
            graphics_fill_rect(x + (w - t) / 2, y + t, t, h - 2 * t, color);
            graphics_fill_rect(x + t, mid_y, w - 2 * t, t, color);
            return;
        default: return;
    }

    uint32_t dim_color = COLOR_RGBA(255, 255, 255, 22);

    /* Segment A: top horizontal */
    graphics_fill_rect(x + t, y, w - 2 * t, t, (seg & (1 << 0)) ? color : dim_color);
    /* Segment B: top-right vertical */
    graphics_fill_rect(right_x, y + t, t, half_h, (seg & (1 << 1)) ? color : dim_color);
    /* Segment C: bottom-right vertical */
    graphics_fill_rect(right_x, mid_y + t, t, half_h, (seg & (1 << 2)) ? color : dim_color);
    /* Segment D: bottom horizontal */
    graphics_fill_rect(x + t, bot_y, w - 2 * t, t, (seg & (1 << 3)) ? color : dim_color);
    /* Segment E: bottom-left vertical */
    graphics_fill_rect(x, mid_y + t, t, half_h, (seg & (1 << 4)) ? color : dim_color);
    /* Segment F: top-left vertical */
    graphics_fill_rect(x, y + t, t, half_h, (seg & (1 << 5)) ? color : dim_color);
    /* Segment G: middle horizontal */
    graphics_fill_rect(x + t, mid_y, w - 2 * t, t, (seg & (1 << 6)) ? color : dim_color);
}

void font_7seg_draw_number(int num, int x, int y, int digit_w, int h, int spacing, uint32_t color) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d", num);

    int cur_x = x;
    for (int i = 0; buf[i] != '\0'; i++) {
        font_7seg_draw_char(buf[i], cur_x, y, digit_w, h, color);
        cur_x += digit_w + spacing;
    }
}
