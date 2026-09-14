/* SPDX-License-Identifier: GPL-3.0-only */
#include <trait/font.h>

#include "trait_font.h"

#define TRAIT_FONT_COUNT (sizeof(trait_font) / sizeof(trait_font[0]))

static const struct trait_font_glyph *glyph_for(char ch)
{
    uint32_t code = (uint32_t)(unsigned char)ch;

    if (code < TRAIT_FONT_FIRST || code > TRAIT_FONT_LAST) {
        return NULL;
    }
    return &trait_font[code - TRAIT_FONT_FIRST];
}

uint32_t trait_font_width(const char *text)
{
    uint32_t total = 0U;
    uint32_t at;

    if (text == NULL) {
        return 0U;
    }
    for (at = 0U; text[at] != '\0'; ++at) {
        const struct trait_font_glyph *glyph = glyph_for(text[at]);

        if (glyph != NULL) {
            total += glyph->advance;
        }
    }
    return total;
}

uint32_t trait_font_line_height(void)
{
    return TRAIT_FONT_HEIGHT;
}

void trait_font_draw(struct trait_surface *surface, struct trait_rect clip,
    uint32_t x, uint32_t baseline, const char *text, uint32_t colour)
{
    uint32_t pen = x;
    uint32_t at;

    if (text == NULL || baseline < TRAIT_FONT_ASCENT) {
        return;
    }
    for (at = 0U; text[at] != '\0'; ++at) {
        const struct trait_font_glyph *glyph = glyph_for(text[at]);
        uint32_t top = baseline - TRAIT_FONT_ASCENT;
        uint32_t row;
        uint32_t column;

        if (glyph == NULL) {
            /* Nothing, rather than a box: a glyph this font does not
             * carry is a gap the caller can see, and a box is a picture
             * of a missing character pretending to be a character. */
            continue;
        }
        for (row = 0U; row < TRAIT_FONT_HEIGHT; ++row) {
            for (column = 0U; column < glyph->width; ++column) {
                uint32_t alpha =
                    glyph->coverage[row * glyph->width + column];
                uint32_t under;

                if (alpha == 0U) {
                    continue;
                }
                under = trait_surface_read(surface, pen + column,
                                           top + row);
                trait_surface_plot(surface, clip, pen + column, top + row,
                                   trait_blend(under, colour, alpha));
            }
        }
        pen += glyph->advance;
    }
}
