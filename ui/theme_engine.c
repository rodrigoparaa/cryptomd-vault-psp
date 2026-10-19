#include "theme_engine.h"

/* Palettes defined in ABGR (0xAABBGGRR) for PSP Hardware */
static const theme_palette s_themes[THEME_COUNT] = {
    /* 1. Dark Cyberpunk */
    {
        .name = "Dark Cyberpunk",
        .bg_color         = COLOR_RGBA(13, 17, 23, 255),    /* #0D1117 */
        .text_color       = COLOR_RGBA(201, 209, 217, 255),/* #C9D1D9 */
        .text_dim         = COLOR_RGBA(139, 148, 158, 255),/* #8B949E */
        .accent_primary   = COLOR_RGBA(88, 166, 255, 255), /* #58A6FF */
        .accent_secondary = COLOR_RGBA(56, 139, 253, 255), /* #388BFD */
        .container_bg     = COLOR_RGBA(22, 27, 34, 255),   /* #161B22 */
        .code_text        = COLOR_RGBA(126, 231, 135, 255),/* #7EE787 */
        .badge_pdoc       = COLOR_RGBA(210, 153, 34, 255), /* Gold/Amber */
        .badge_plain      = COLOR_RGBA(56, 139, 253, 255), /* Blue */
        .header_bg        = COLOR_RGBA(22, 27, 34, 255),
        .footer_bg        = COLOR_RGBA(22, 27, 34, 255),
        .alert_danger     = COLOR_RGBA(248, 81, 73, 255),  /* #F85149 */
        .alert_success    = COLOR_RGBA(46, 160, 67, 255)   /* #2EA043 */
    },
    /* 2. Sepia Paper */
    {
        .name = "Sepia Paper",
        .bg_color         = COLOR_RGBA(251, 240, 217, 255),/* #FBF0D9 */
        .text_color       = COLOR_RGBA(67, 52, 34, 255),   /* #433422 */
        .text_dim         = COLOR_RGBA(138, 117, 92, 255), /* Muted Brown */
        .accent_primary   = COLOR_RGBA(158, 93, 42, 255),  /* #9E5D2A */
        .accent_secondary = COLOR_RGBA(194, 114, 52, 255),
        .container_bg     = COLOR_RGBA(240, 226, 196, 255),/* #F0E2C4 */
        .code_text        = COLOR_RGBA(46, 117, 89, 255),
        .badge_pdoc       = COLOR_RGBA(158, 93, 42, 255),
        .badge_plain      = COLOR_RGBA(90, 120, 140, 255),
        .header_bg        = COLOR_RGBA(235, 218, 185, 255),
        .footer_bg        = COLOR_RGBA(235, 218, 185, 255),
        .alert_danger     = COLOR_RGBA(180, 50, 40, 255),
        .alert_success    = COLOR_RGBA(46, 130, 67, 255)
    },
    /* 3. OLED Stealth */
    {
        .name = "OLED Stealth",
        .bg_color         = COLOR_RGBA(0, 0, 0, 255),        /* #000000 */
        .text_color       = COLOR_RGBA(230, 237, 243, 255),/* #E6EDF3 */
        .text_dim         = COLOR_RGBA(110, 118, 129, 255),
        .accent_primary   = COLOR_RGBA(242, 204, 96, 255), /* #F2CC60 (Gold) */
        .accent_secondary = COLOR_RGBA(218, 165, 32, 255),
        .container_bg     = COLOR_RGBA(18, 18, 18, 255),   /* #121212 */
        .code_text        = COLOR_RGBA(80, 250, 123, 255), /* Neon green */
        .badge_pdoc       = COLOR_RGBA(242, 204, 96, 255),
        .badge_plain      = COLOR_RGBA(100, 150, 255, 255),
        .header_bg        = COLOR_RGBA(15, 15, 15, 255),
        .footer_bg        = COLOR_RGBA(15, 15, 15, 255),
        .alert_danger     = COLOR_RGBA(255, 85, 85, 255),
        .alert_success    = COLOR_RGBA(80, 250, 123, 255)
    }
};

const theme_palette *theme_get(app_theme theme) {
    if (theme >= THEME_COUNT) theme = THEME_CYBERPUNK;
    return &s_themes[theme];
}
