#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "text.h"
#include "unicode.h"

#define FONT_MAGIC 0x746e4675
#define FONT_VERSION 1


typedef struct __attribute__((packed)) font_header_t {
    uint32_t magic;
    uint8_t version;
    uint8_t flags;
    uint16_t ascent;
    uint16_t descent;
} font_header_t;

typedef struct __attribute__((packed)) font_group_t {
    uint32_t first;
    uint32_t last;
    uint32_t offsets[];
} font_group_t;

typedef struct __attribute__((packed)) font_glyph_t {
    int16_t offset_x;
    int16_t offset_y;
    bitmap_t bitmap;
} font_glyph_t;

typedef struct text_state_t {
    text_config_t config;
    const void *font;
    uint16_t elide_width;
    uint16_t elide_count;
} text_state_t;

typedef struct glyph_t {
    uint16_t width;
    uint8_t c;
} glyph_t;

typedef struct line_t {
    uint16_t width;
    size_t consume;
    size_t discard;
    bool elide;
} line_t;

static bool text_validate_font(const void *font);
static const font_glyph_t *text_get_glyph(text_state_t *state, uint32_t cp);


void text_render(bitmap_t *dst, const text_config_t *config, const void *font,
        int xpos, int ypos, int width, int height, const char *s)
{
    if (!text_validate_font(font)) {
        return;
    }

    text_state_t state = { };
    if (config) {
        memcpy(&state.config, config, sizeof(state.config));
    }

    state.config.draw_fn = state.config.draw_fn ? state.config.draw_fn :
            bitmap_set_pixel;
    state.font = font;

    font_header_t *header = (font_header_t *) font;

    int line_height = header->ascent + header->descent;
    if (line_height > height) {
        return;
    }
    int max_lines = 1 + (height - line_height) /
            (state.config.line_spacing + line_height);

    if (state.config.elide_text) {
        for (size_t offset = 0; state.config.elide_text[offset];) {
            uint32_t cp;
            offset += utf8_cp(&state.config.elide_text[offset], &cp);
            const font_glyph_t *glyph = text_get_glyph(&state, cp);
            if (glyph) {
                state.elide_width += glyph->bitmap.width + glyph->offset_x +
                        state.config.kerning;
            }
            state.elide_count += 1;
        }
    }

    int glyph_count = utf8_len(s);
    glyph_t glyphs[glyph_count];
    memset(glyphs, 0, sizeof(glyphs));
    size_t s_offset = 0;
    int glyph_index = 0;
    while (glyph_index < glyph_count) {
        uint32_t cp;
        s_offset += utf8_cp(&s[s_offset], &cp);
        if (cp <= 0x7F) {
            glyphs[glyph_index].c = cp;
        }
        const font_glyph_t *glyph = text_get_glyph(&state, cp);
        if (glyph == NULL) {
            glyph = text_get_glyph(&state, '?');
        }
        if (glyph != NULL) {
            glyphs[glyph_index].width = glyph->bitmap.width + glyph->offset_x;
        }
        glyph_index++;
    }

    line_t lines[max_lines];
    memset(lines, 0, sizeof(lines));
    bool done = false;
    int line_index = 0;    
    glyph_index = 0;
    while (!done && line_index < max_lines) {
        while (glyph_index < glyph_count) {
            if (glyphs[glyph_index].c == '\n') {
                lines[line_index].discard++;
                glyph_index++;
                break;
            } else if (lines[line_index].width + glyphs[glyph_index].width <
                    width) {
                lines[line_index].width += glyphs[glyph_index].width;
                if (glyph_index >= 1) {
                    lines[line_index].width += state.config.kerning;
                }
                lines[line_index].consume++;
                glyph_index++;
            } else if (state.config.overflow == TEXT_OVERFLOW_BREAK_WORD) {
                while (lines[line_index].width + state.elide_width >
                        width) {
                    glyph_index--;
                    if (lines[line_index].consume <= 0 || glyph_index < 0) {
                        done = true;
                        goto done;
                    }
                    lines[line_index].width -= glyphs[glyph_index].width;
                    if (lines[line_index].consume > 1) {
                        lines[line_index].width -= state.config.kerning;
                    }
                    lines[line_index].consume--;
                } 
                lines[line_index].elide = true;
                lines[line_index].width += state.elide_width;
                done = true;
                goto done;
            } else if (state.config.overflow == TEXT_OVERFLOW_WORD) {
                while (lines[line_index].width + state.elide_width >
                        width) {
                    glyph_index--;
                    if (lines[line_index].consume <= 0 || glyph_index < 0) {
                        done = true;
                        goto done;
                    }
                    lines[line_index].width -= glyphs[glyph_index].width;
                    if (lines[line_index].consume > 1) {
                        lines[line_index].width -= state.config.kerning;
                    }
                    lines[line_index].consume--;
                } 
                while (lines[line_index].consume > 0 &&
                        glyphs[glyph_index].c != ' ') {
                    glyph_index--;
                    if (lines[line_index].consume <= 0 || glyph_index < 0) {
                        done = true;
                        goto done;
                    }
                    lines[line_index].width -= glyphs[glyph_index].width;
                    if (lines[line_index].consume > 1) {
                        lines[line_index].width -= state.config.kerning;
                    }
                    lines[line_index].consume--;
                }
                while (glyphs[glyph_index].c == ' ') {
                    lines[line_index].discard++;
                    glyph_index++;
                }
                lines[line_index].elide = true;
                lines[line_index].width += state.elide_width;
                done = true;
                goto done;
            } else if (state.config.overflow == TEXT_OVERFLOW_WRAP) {
                if (line_index + 1 >= max_lines) {
                    while (lines[line_index].width + state.elide_width >
                            width) {
                        glyph_index--;
                        if (lines[line_index].consume <= 0 || glyph_index < 0) {
                            done = true;
                            goto done;
                        }
                        lines[line_index].width -= glyphs[glyph_index].width;
                        if (lines[line_index].consume > 1) {
                            lines[line_index].width -= state.config.kerning;
                        }
                        lines[line_index].consume--;
                    } 
                    lines[line_index].elide = true;
                    lines[line_index].width += state.elide_width;
                }
                while (glyphs[glyph_index].c == ' ') {
                    lines[line_index].discard++;
                    glyph_index++;
                }
                break;
            } else if (state.config.overflow == TEXT_OVERFLOW_WRAP_WORD) {
                if (line_index + 1 >= max_lines) {
                    while (lines[line_index].width + state.elide_width >
                            width) {
                        glyph_index--;
                        if (lines[line_index].consume <= 0 || glyph_index < 0) {
                            done = true;
                            goto done;
                        }
                        lines[line_index].width -= glyphs[glyph_index].width;
                        if (lines[line_index].consume > 1) {
                            lines[line_index].width -= state.config.kerning;
                        }
                        lines[line_index].consume--;
                    } 
                    lines[line_index].elide = true;
                    lines[line_index].width += state.elide_width;
                }
                while (lines[line_index].consume > 0 &&
                        glyphs[glyph_index].c != ' ') {
                    glyph_index--;
                    if (lines[line_index].consume <= 0 || glyph_index < 0) {
                        done = true;
                        goto done;
                    }
                    lines[line_index].width -= glyphs[glyph_index].width;
                    if (lines[line_index].consume > 1) {
                        lines[line_index].width -= state.config.kerning;
                    }
                    lines[line_index].consume--;
                }
                while (glyphs[glyph_index].c == ' ') {
                    lines[line_index].discard++;
                    glyph_index++;
                }
                break;
            } else {
                glyph_index++;
                lines[line_index].discard++;
            }
        }
        if (lines[line_index].consume == 0 && lines[line_index].discard == 0) {
            done = true;
        }
done:
        line_index++;
    }
    int line_count = line_index;

    int offset_y = 0;
    if (state.config.valign == TEXT_VALIGN_MIDDLE) {
        offset_y = height / 2 - (line_height + (line_height +
                state.config.line_spacing) * (line_count - 1)) / 2;
    } else if (state.config.valign == TEXT_VALIGN_BOTTOM) {
        offset_y = height - (line_height + (line_height +
                state.config.line_spacing) * (line_count - 1));
    }

    int y = 0;
    line_index = 0;
    glyph_index = 0;
    s_offset = 0;
    while (line_index < line_count) {
        int offset_x = 0;
        int x = 0;
        if (state.config.align == TEXT_ALIGN_CENTER) {
            offset_x = width / 2 - lines[line_index].width / 2;
        } else if (state.config.align == TEXT_ALIGN_RIGHT) {
            offset_x = width - lines[line_index].width;
        }

        int consumed = 0;
        while (consumed < lines[line_index].consume) {
            uint32_t cp;
            s_offset += utf8_cp(&s[s_offset], &cp);
            const font_glyph_t *glyph = text_get_glyph(&state, cp);
            if (!glyph) {
                glyph = text_get_glyph(&state, '?');
            }
            if (glyph) {
                bitmap_blit2(dst, &glyph->bitmap, state.config.draw_fn,
                        xpos + offset_x + x + glyph->offset_x,
                        ypos + offset_y + y + glyph->offset_y, 0, 0, 0, 0);
                offset_x += glyph->bitmap.width + glyph->offset_x +
                        state.config.kerning;
            }
            glyph_index++;
            consumed++;            
        }
        int discarded = 0;
        while (discarded < lines[line_index].discard) {
            s_offset += utf8_cp(&s[s_offset], NULL);
            glyph_index++;
            discarded++;
        }

        if (lines[line_index].elide) {
            int elide_index = 0;
            int elide_offset = 0;
            while (elide_index < state.elide_count) {
                uint32_t cp;
                elide_offset += utf8_cp(&state.config.elide_text[elide_offset], &cp);
                const font_glyph_t *glyph = text_get_glyph(&state, cp);
                if (!glyph) {
                    glyph = text_get_glyph(&state, '?');
                }
                if (glyph) {
                    bitmap_blit2(dst, &glyph->bitmap, state.config.draw_fn,
                            xpos + offset_x + x + glyph->offset_x,
                            ypos + offset_y + y + glyph->offset_y, 0, 0, 0, 0);
                    offset_x += glyph->bitmap.width + glyph->offset_x +
                            state.config.kerning;
                }
                elide_index++;
            }
        }

        line_index += 1;
        y += state.config.line_spacing + line_height;
    }
}


static bool text_validate_font(const void *font)
{
    font_header_t *header = (font_header_t *)font;

    if (header->magic != FONT_MAGIC) {
        return false;
    }

    if (header->version != FONT_VERSION) {
        return false;
    }

    return true;
}


static const font_glyph_t *text_get_glyph(text_state_t *state, uint32_t cp)
{
    const font_group_t *group = (const font_group_t *)(state->font +
            sizeof(font_header_t));

    while (group->first <= group->last) {
        if (cp >= group->first && cp <= group->last) {
            break;
        }
        group += sizeof(font_group_t) + sizeof(void *) *
                (group->last - group->first + 1);
    }

    if (group->first > group->last) {
        return NULL;
    }
    
    size_t i = cp - group->first;
    return (const font_glyph_t *)(state->font + group->offsets[i]);
}
