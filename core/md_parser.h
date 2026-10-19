#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <stdint.h>
#include <stddef.h>

#define MD_MAX_LINES 4096
#define MD_LINE_MAX_CHARS 96
#define MD_VIEWPORT_WIDTH 448
#define MD_VIEWPORT_HEIGHT 226
#define MD_MAX_DOC_SIZE (128 * 1024)

typedef enum {
    MD_LINE_TEXT = 0,
    MD_LINE_H1,
    MD_LINE_H2,
    MD_LINE_H3,
    MD_LINE_CODE,
    MD_LINE_QUOTE,
    MD_LINE_BULLET,
    MD_LINE_BULLET_CONT,
    MD_LINE_HR,
    MD_LINE_EMPTY
} md_line_type;

typedef struct {
    md_line_type type;
    uint16_t y_offset;     /* Relative Y offset from top of document */
    uint8_t height;        /* Line height in pixels (e.g. 10, 18, 22) */
    char text[MD_LINE_MAX_CHARS];
} md_render_line;

typedef struct {
    md_render_line lines[MD_MAX_LINES];
    size_t line_count;
    uint32_t total_height;
} md_document;

/* Sanitizes Spanish UTF-8 multibyte characters into single-byte extended font indices.
 * Removes 3-byte and 4-byte emojis. Returns sanitized length. */
size_t md_sanitize_utf8(const char *src, size_t src_len, char *dst, size_t dst_max);

/* Tokenizes and layouts a plaintext/markdown buffer into visual render lines.
 * Handles headers, code fences, blockquotes, bullets, and word-wrapping. */
void md_parse_document(const char *raw_text, size_t raw_len, md_document *doc);

#endif /* MD_PARSER_H */
