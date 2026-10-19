#ifndef FONT_7SEG_H
#define FONT_7SEG_H

#include <stdint.h>

/* Draws a 7-segment digit (0..9, minus '-', plus '+') at (x, y) with specified width, height and color */
void font_7seg_draw_char(char c, int x, int y, int w, int h, uint32_t color);

/* Draws an integer value (e.g. +3, -12, 25) using 7-segment digits */
void font_7seg_draw_number(int num, int x, int y, int digit_w, int h, int spacing, uint32_t color);

#endif /* FONT_7SEG_H */
