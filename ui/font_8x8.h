#ifndef FONT_8X8_H
#define FONT_8X8_H

#include <stdint.h>
#include <stddef.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 8

/* Returns 8-byte bitmap glyph row data for character c (ASCII or Spanish 0x80..0x8F) */
const uint8_t *font_8x8_get_glyph(unsigned char c);

#endif /* FONT_8X8_H */
