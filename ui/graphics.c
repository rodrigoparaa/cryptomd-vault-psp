#include "graphics.h"
#include "font_8x8.h"
#include <string.h>

#ifdef __PSP__
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <stdint.h>

#define BUF_WIDTH  512
#define SCR_WIDTH  480
#define SCR_HEIGHT 272

static unsigned int __attribute__((aligned(16))) s_dlist[1024];
static uint32_t *s_vram_draw = (uint32_t *)0x44000000;

void graphics_init(void) {
    sceGuInit();
    sceGuStart(GU_DIRECT, s_dlist);

    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void *)0x88000, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    s_vram_draw = (uint32_t *)0x44000000;
}

void graphics_term(void) {
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
}

void graphics_start_frame(void) {
    /* Direct uncached VRAM rendering requires no per-frame GU list setup */
}

void graphics_end_frame(void) {
    sceDisplayWaitVblankStart();
    void *new_draw = sceGuSwapBuffers();
    s_vram_draw = (uint32_t *)((uintptr_t)new_draw | 0x44000000);
}

void graphics_clear(uint32_t color) {
    uint32_t *ptr = s_vram_draw;
    for (int y = 0; y < SCR_HEIGHT; y++) {
        for (int x = 0; x < SCR_WIDTH; x++) {
            ptr[x] = color;
        }
        ptr += BUF_WIDTH;
    }
}

void graphics_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    if (x >= SCR_WIDTH || y >= SCR_HEIGHT) return;
    if (x + w <= 0 || y + h <= 0) return;

    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCR_WIDTH) ? SCR_WIDTH : (x + w);
    int y1 = (y + h > SCR_HEIGHT) ? SCR_HEIGHT : (y + h);

    uint8_t alpha = (uint8_t)((color >> 24) & 0xFF);
    if (alpha == 0) return;

    if (alpha == 255) {
        for (int r = y0; r < y1; r++) {
            uint32_t *row = &s_vram_draw[r * BUF_WIDTH + x0];
            for (int c = x0; c < x1; c++) {
                *row++ = color;
            }
        }
    } else {
        /* Alpha blend for modal dimming */
        uint32_t sr = color & 0xFF;
        uint32_t sg = (color >> 8) & 0xFF;
        uint32_t sb = (color >> 16) & 0xFF;
        uint32_t inv_a = 255 - alpha;
        for (int r = y0; r < y1; r++) {
            uint32_t *row = &s_vram_draw[r * BUF_WIDTH + x0];
            for (int c = x0; c < x1; c++) {
                uint32_t dst = *row;
                uint32_t dr = dst & 0xFF;
                uint32_t dg = (dst >> 8) & 0xFF;
                uint32_t db = (dst >> 16) & 0xFF;
                uint32_t nr = (sr * alpha + dr * inv_a) >> 8;
                uint32_t ng = (sg * alpha + dg * inv_a) >> 8;
                uint32_t nb = (sb * alpha + db * inv_a) >> 8;
                *row++ = 0xFF000000U | (nb << 16) | (ng << 8) | nr;
            }
        }
    }
}

void graphics_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    if (y0 == y1) {
        int xs = (x0 < x1) ? x0 : x1;
        int xe = (x0 > x1) ? x0 : x1;
        graphics_fill_rect(xs, y0, xe - xs + 1, 1, color);
        return;
    }
    if (x0 == x1) {
        int ys = (y0 < y1) ? y0 : y1;
        int ye = (y0 > y1) ? y0 : y1;
        graphics_fill_rect(x0, ys, 1, ye - ys + 1, color);
        return;
    }
    /* Bresenham fallback */
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while (1) {
        graphics_fill_rect(x0, y0, 1, 1, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

#else
/* Host Stub Implementation for testing/compilation */
void graphics_init(void) {}
void graphics_term(void) {}
void graphics_start_frame(void) {}
void graphics_end_frame(void) {}
void graphics_clear(uint32_t color) { (void)color; }
void graphics_fill_rect(int x, int y, int w, int h, uint32_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}
void graphics_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}
#endif

void graphics_draw_rect(int x, int y, int w, int h, uint32_t color) {
    graphics_fill_rect(x, y, w, 1, color);             /* Top */
    graphics_fill_rect(x, y + h - 1, w, 1, color);     /* Bottom */
    graphics_fill_rect(x, y, 1, h, color);             /* Left */
    graphics_fill_rect(x + w - 1, y, 1, h, color);     /* Right */
}

void graphics_draw_char(char c, int x, int y, uint32_t color, int scale) {
    if (scale < 1) scale = 1;
    if (x + 8 * scale < 0 || x >= SCREEN_WIDTH) return;
    if (y + 8 * scale < 0 || y >= SCREEN_HEIGHT) return;

    const uint8_t *glyph = font_8x8_get_glyph((unsigned char)c);
    for (int row = 0; row < 8; row++) {
        uint8_t row_bits = glyph[row];
        if (row_bits == 0) continue;
        for (int col = 0; col < 8; col++) {
            if (row_bits & (1 << (7 - col))) {
                graphics_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void graphics_draw_string(const char *str, int x, int y, uint32_t color, int scale) {
    if (!str) return;
    int cur_x = x;
    int step = 8 * scale;
    for (size_t i = 0; str[i] != '\0'; i++) {
        graphics_draw_char(str[i], cur_x, y, color, scale);
        cur_x += step;
    }
}

void graphics_draw_string_clipped(const char *str, int x, int y, uint32_t color, int scale, int max_x) {
    if (!str) return;
    int cur_x = x;
    int step = 8 * scale;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (cur_x + step > max_x) {
            /* Draw ellipsis ... if clipped */
            if (cur_x + 3 * step <= max_x + step) {
                graphics_draw_string("...", cur_x, y, color, scale);
            }
            break;
        }
        graphics_draw_char(str[i], cur_x, y, color, scale);
        cur_x += step;
    }
}
