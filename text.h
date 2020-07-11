#pragma once

#include <stdbool.h>

#include "bitmap.h"


#define FONT_FLAG_MONOSPACE (1 << 0)

typedef enum text_align_t {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_JUSTIFY,
} text_align_t;

typedef enum text_valign_t {
    TEXT_VALIGN_TOP,
    TEXT_VALIGN_MIDDLE,
    TEXT_VALIGN_BOTTOM,
} text_valign_t;

typedef enum text_overflow_t {
    TEXT_OVERFLOW_BREAK_WORD,
    TEXT_OVERFLOW_WORD,
    TEXT_OVERFLOW_WRAP,
    TEXT_OVERFLOW_WRAP_WORD,
} text_overflow_t;

typedef struct text_config_t {
    bitmap_draw_fn draw_fn;
    text_align_t align;
    text_valign_t valign;
    text_overflow_t overflow;
    char *elide_text;
    int8_t kerning;
    int8_t line_spacing;
} text_config_t;


void text_render(bitmap_t *dst, const text_config_t *config, const void *font,
        int xpos, int ypos, int width, int height, const char *s);