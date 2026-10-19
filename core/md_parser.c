#include "md_parser.h"
#include <string.h>
#include <ctype.h>

size_t md_sanitize_utf8(const char *src, size_t src_len, char *dst, size_t dst_max) {
    if (!src || src_len == 0 || !dst || dst_max == 0) return 0;
    size_t r = 0;
    size_t w = 0;

    while (r < src_len && w + 1 < dst_max) {
        unsigned char b = (unsigned char)src[r];

        if (b < 0x80) {
            /* Standard ASCII (except carriage return \r) */
            if (b != '\r') {
                dst[w++] = (char)b;
            }
            r++;
        } else if (b == 0xC3 && r + 1 < src_len) {
            /* 2-byte UTF-8 Spanish Accents and Ñ */
            unsigned char b2 = (unsigned char)src[r + 1];
            char mapped = 0;
            switch (b2) {
                case 0xA1: mapped = (char)0x80; break; /* á */
                case 0xA9: mapped = (char)0x81; break; /* é */
                case 0xAD: mapped = (char)0x82; break; /* í */
                case 0xB3: mapped = (char)0x83; break; /* ó */
                case 0xBA: mapped = (char)0x84; break; /* ú */
                case 0xB1: mapped = (char)0x85; break; /* ñ */
                case 0x81: mapped = (char)0x86; break; /* Á */
                case 0x89: mapped = (char)0x87; break; /* É */
                case 0x8D: mapped = (char)0x88; break; /* Í */
                case 0x93: mapped = (char)0x89; break; /* Ó */
                case 0x9A: mapped = (char)0x8A; break; /* Ú */
                case 0x91: mapped = (char)0x8B; break; /* Ñ */
                case 0xBC: mapped = (char)0x8C; break; /* ü */
                case 0x9C: mapped = (char)0x8D; break; /* Ü */
                default: mapped = '?'; break;
            }
            dst[w++] = mapped;
            r += 2;
        } else if (b == 0xC2 && r + 1 < src_len) {
            /* 2-byte UTF-8 ¿ and ¡ */
            unsigned char b2 = (unsigned char)src[r + 1];
            if (b2 == 0xBF) dst[w++] = (char)0x8E;      /* ¿ */
            else if (b2 == 0xA1) dst[w++] = (char)0x8F; /* ¡ */
            else dst[w++] = '?';
            r += 2;
        } else if ((b & 0xF0) == 0xE0) {
            /* 3-byte UTF-8: map common punctuation/arrows, discard emojis/icons */
            if (r + 2 < src_len) {
                unsigned char b2 = (unsigned char)src[r + 1];
                unsigned char b3 = (unsigned char)src[r + 2];
                if (b == 0xE2 && b2 == 0x80) {
                    if (b3 == 0x93 || b3 == 0x94) { dst[w++] = '-'; }
                    else if (b3 == 0x98 || b3 == 0x99) { dst[w++] = '\''; }
                    else if (b3 == 0x9C || b3 == 0x9D) { dst[w++] = '"'; }
                    else if (b3 == 0xA6 && w + 3 < dst_max) {
                        dst[w++] = '.'; dst[w++] = '.'; dst[w++] = '.';
                    }
                } else if (b == 0xE2 && b2 == 0x86) {
                    if (b3 == 0x92 && w + 2 < dst_max) { dst[w++] = '-'; dst[w++] = '>'; }
                    else if (b3 == 0x90 && w + 2 < dst_max) { dst[w++] = '<'; dst[w++] = '-'; }
                }
                r += 3;
            } else {
                r = src_len;
            }
        } else if ((b & 0xF8) == 0xF0) {
            /* 4-byte UTF-8 (emojis): discard */
            r += (r + 3 < src_len) ? 4 : (src_len - r);
        } else {
            /* Other multibyte or corrupt: skip 1 */
            r++;
        }
    }
    dst[w] = '\0';
    return w;
}

static int is_word_char(unsigned char c) {
    return isalnum(c) || c >= 0x80;
}

/* Strips inline Markdown markers (**bold**, *italic*, `code`, [label](url), badges) */
static void clean_inline_markdown(const char *src, char *dst, size_t dst_max) {
    if (!src || !dst || dst_max == 0) return;
    size_t r = 0;
    size_t w = 0;
    size_t len = strlen(src);

    while (r < len && w + 1 < dst_max) {
        /* 1. Badge link: [![alt](img_url)](link_url) -> skip completely */
        if (src[r] == '[' && r + 2 < len && src[r + 1] == '!' && src[r + 2] == '[') {
            size_t p = r + 3;
            while (p < len && src[p] != ']') p++;
            if (p + 1 < len && src[p] == ']' && src[p + 1] == '(') {
                p += 2;
                while (p < len && src[p] != ')') p++;
                if (p + 1 < len && src[p] == ')' && src[p + 1] == ']') {
                    p += 2;
                    if (p < len && src[p] == '(') {
                        p++;
                        while (p < len && src[p] != ')') p++;
                        if (p < len && src[p] == ')') p++;
                    }
                    r = p;
                    continue;
                }
            }
        }

        /* 2. Inline image: ![alt](url) -> skip */
        if (src[r] == '!' && r + 1 < len && src[r + 1] == '[') {
            size_t p = r + 2;
            while (p < len && src[p] != ']') p++;
            if (p + 1 < len && src[p] == ']' && src[p + 1] == '(') {
                p += 2;
                while (p < len && src[p] != ')') p++;
                if (p < len && src[p] == ')') p++;
                r = p;
                continue;
            }
        }

        /* 3. Markdown link: [label](url) -> keep label, skip (url) */
        if (src[r] == '[') {
            size_t close_br = r + 1;
            while (close_br < len && src[close_br] != ']' && src[close_br] != '[') {
                close_br++;
            }
            if (close_br + 1 < len && src[close_br] == ']' && src[close_br + 1] == '(') {
                size_t close_par = close_br + 2;
                while (close_par < len && src[close_par] != ')') {
                    close_par++;
                }
                if (close_par < len && src[close_par] == ')') {
                    /* Copy inner label (stripping backticks/asterisks inside label) */
                    for (size_t k = r + 1; k < close_br && w + 1 < dst_max; k++) {
                        if (src[k] != '*' && src[k] != '`') {
                            dst[w++] = src[k];
                        }
                    }
                    r = close_par + 1;
                    continue;
                }
            }
        }

        /* 4. Bold / Italic / Code markers */
        if (src[r] == '*' && r + 1 < len && src[r + 1] == '*') {
            r += 2;
            continue;
        }
        if (src[r] == '_' && r + 1 < len && src[r + 1] == '_') {
            r += 2;
            continue;
        }
        if (src[r] == '*' || src[r] == '`') {
            r++;
            continue;
        }
        if (src[r] == '_') {
            int prev_w = (w > 0 && is_word_char((unsigned char)dst[w - 1]));
            int next_w = (r + 1 < len && is_word_char((unsigned char)src[r + 1]));
            if (!prev_w || !next_w) {
                r++;
                continue;
            }
        }

        dst[w++] = src[r++];
    }
    dst[w] = '\0';
}

static void add_line(md_document *doc, md_line_type type, const char *text, uint8_t height) {
    if (doc->line_count >= MD_MAX_LINES) return;
    md_render_line *line = &doc->lines[doc->line_count];
    line->type = type;
    line->y_offset = (uint16_t)doc->total_height;
    line->height = height;

    size_t len = strlen(text);
    if (len >= MD_LINE_MAX_CHARS) len = MD_LINE_MAX_CHARS - 1;
    memcpy(line->text, text, len);
    line->text[len] = '\0';

    doc->line_count++;
    doc->total_height += height;
}

static void wrap_and_add_ext(md_document *doc, md_line_type first_type, md_line_type cont_type,
                             const char *text, size_t max_chars, uint8_t height) {
    while (*text == ' ') text++;
    size_t len = strlen(text);
    if (len == 0) {
        add_line(doc, first_type, "", height);
        return;
    }

    int is_first = 1;
    size_t start = 0;
    while (start < len) {
        md_line_type cur_type = is_first ? first_type : cont_type;
        size_t remaining = len - start;
        if (remaining <= max_chars) {
            add_line(doc, cur_type, text + start, height);
            break;
        }

        /* Look for last space within max_chars */
        size_t split = max_chars;
        while (split > 0 && text[start + split] != ' ') {
            split--;
        }

        if (split == 0) {
            /* Hard break on long word */
            split = max_chars;
        }

        char temp[MD_LINE_MAX_CHARS];
        size_t copy_len = (split < MD_LINE_MAX_CHARS) ? split : (MD_LINE_MAX_CHARS - 1);
        memcpy(temp, text + start, copy_len);
        temp[copy_len] = '\0';

        add_line(doc, cur_type, temp, height);
        is_first = 0;

        start += split;
        while (start < len && text[start] == ' ') {
            start++; /* Skip space */
        }
    }
}

static void wrap_and_add(md_document *doc, md_line_type type, const char *text, size_t max_chars, uint8_t height) {
    wrap_and_add_ext(doc, type, type, text, max_chars, height);
}

void md_parse_document(const char *raw_text, size_t raw_len, md_document *doc) {
    if (!doc) return;
    doc->line_count = 0;
    doc->total_height = 0;

    if (!raw_text || raw_len == 0) return;

    /* First pass: sanitize UTF-8 using static buffer in .bss */
    static char s_clean_buf[MD_MAX_DOC_SIZE];
    size_t clean_len = md_sanitize_utf8(raw_text, raw_len, s_clean_buf, sizeof(s_clean_buf));

    int in_code_block = 0;
    int last_was_empty = 0;
    size_t i = 0;

    while (i < clean_len) {
        /* Read one line */
        size_t line_start = i;
        while (i < clean_len && s_clean_buf[i] != '\n') {
            i++;
        }
        size_t line_len = i - line_start;
        if (i < clean_len && s_clean_buf[i] == '\n') {
            i++; /* Consume newline */
        }

        char cur_line[512];
        size_t copy_len = (line_len < sizeof(cur_line) - 1) ? line_len : (sizeof(cur_line) - 1);
        memcpy(cur_line, s_clean_buf + line_start, copy_len);
        cur_line[copy_len] = '\0';

        /* Trim trailing whitespace */
        while (copy_len > 0 && isspace((unsigned char)cur_line[copy_len - 1])) {
            cur_line[--copy_len] = '\0';
        }

        const char *trimmed = cur_line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        /* Check for code block toggle: ``` */
        if (strncmp(trimmed, "```", 3) == 0) {
            in_code_block = !in_code_block;
            add_line(doc, MD_LINE_EMPTY, "", in_code_block ? 4 : 6);
            last_was_empty = 0;
            continue;
        }

        if (in_code_block) {
            wrap_and_add(doc, MD_LINE_CODE, cur_line, 52, 11);
            last_was_empty = 0;
            continue;
        }

        if (*trimmed == '\0') {
            if (!last_was_empty && doc->line_count > 0) {
                add_line(doc, MD_LINE_EMPTY, "", 8);
                last_was_empty = 1;
            }
            continue;
        }

        /* Check horizontal rule: --- or *** */
        if (strcmp(trimmed, "---") == 0 || strcmp(trimmed, "***") == 0) {
            add_line(doc, MD_LINE_HR, "", 12);
            last_was_empty = 0;
            continue;
        }

        char clean_line[512];

        /* Headers */
        if (strncmp(trimmed, "# ", 2) == 0) {
            clean_inline_markdown(trimmed + 2, clean_line, sizeof(clean_line));
            add_line(doc, MD_LINE_EMPTY, "", 10);
            wrap_and_add(doc, MD_LINE_H1, clean_line, 26, 20);
            add_line(doc, MD_LINE_EMPTY, "", 6);
            last_was_empty = 0;
            continue;
        }
        if (strncmp(trimmed, "## ", 3) == 0) {
            clean_inline_markdown(trimmed + 3, clean_line, sizeof(clean_line));
            add_line(doc, MD_LINE_EMPTY, "", 8);
            wrap_and_add(doc, MD_LINE_H2, clean_line, 26, 18);
            add_line(doc, MD_LINE_EMPTY, "", 4);
            last_was_empty = 0;
            continue;
        }
        if (strncmp(trimmed, "### ", 4) == 0 || strncmp(trimmed, "#### ", 5) == 0) {
            const char *h_src = (trimmed[3] == '#') ? (trimmed + 5) : (trimmed + 4);
            clean_inline_markdown(h_src, clean_line, sizeof(clean_line));
            add_line(doc, MD_LINE_EMPTY, "", 6);
            wrap_and_add(doc, MD_LINE_H3, clean_line, 52, 13);
            last_was_empty = 0;
            continue;
        }

        /* Blockquote: > */
        if (trimmed[0] == '>' && (trimmed[1] == ' ' || trimmed[1] == '\0')) {
            const char *q_text = (trimmed[1] == ' ') ? (trimmed + 2) : (trimmed + 1);
            clean_inline_markdown(q_text, clean_line, sizeof(clean_line));
            wrap_and_add(doc, MD_LINE_QUOTE, clean_line, 50, 12);
            last_was_empty = 0;
            continue;
        }

        /* Bullet lists: -, *, + */
        if ((trimmed[0] == '-' || trimmed[0] == '*' || trimmed[0] == '+') && trimmed[1] == ' ') {
            clean_inline_markdown(trimmed + 2, clean_line, sizeof(clean_line));
            wrap_and_add_ext(doc, MD_LINE_BULLET, MD_LINE_BULLET_CONT, clean_line, 50, 12);
            last_was_empty = 0;
            continue;
        }

        /* Normal paragraph text */
        clean_inline_markdown(trimmed, clean_line, sizeof(clean_line));
        const char *p_clean = clean_line;
        while (*p_clean == ' ') p_clean++;
        if (*p_clean == '\0') {
            /* Line contained only stripped badges/images */
            continue;
        }
        wrap_and_add(doc, MD_LINE_TEXT, p_clean, 54, 12);
        last_was_empty = 0;
    }

    /* Bottom scroll padding so last lines are clearly visible above footer */
    add_line(doc, MD_LINE_EMPTY, "", 48);
}
