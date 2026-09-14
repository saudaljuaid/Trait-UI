/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_FONT_H_API
#define TRAIT_FONT_H_API

#include <stdint.h>

#include <trait/surface.h>

/*
 * Text, as coverage.
 *
 * The glyphs are rasterised ahead of time by tools/c/make-font.py - there
 * is no font server behind a framebuffer - and drawn by tinting that
 * coverage, so one bitmap serves every colour the shell wants text in.
 */
uint32_t trait_font_width(const char *text);
uint32_t trait_font_line_height(void);

/* `baseline` is the text baseline, not the top of the cell, because that
 * is what every metric a font ships is measured from. */
void trait_font_draw(struct trait_surface *surface, struct trait_rect clip,
    uint32_t x, uint32_t baseline, const char *text, uint32_t colour);

#endif /* TRAIT_FONT_H_API */
