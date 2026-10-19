#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define BUFFER_WIDTH  512

void graphics_init(void);
void graphics_term(void);

void graphics_start_frame(void);
void graphics_end_frame(void);

void graphics_clear(uint32_t color);

void graphics_fill_rect(int x, int y, int w, int h, uint32_t color);
void graphics_draw_rect(int x, int y, int w, int h, uint32_t color);
void graphics_draw_line(int x0, int y0, int x1, int y1, uint32_t color);

void graphics_draw_char(char c, int x, int y, uint32_t color, int scale);
void graphics_draw_string(const char *str, int x, int y, uint32_t color, int scale);
void graphics_draw_string_clipped(const char *str, int x, int y, uint32_t color, int scale, int max_x);

#endif /* GRAPHICS_H */
