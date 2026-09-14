/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_THEME_H
#define TRAIT_THEME_H

#include <stdint.h>

/*
 * THE PALETTE, IN ONE PLACE.
 *
 * Clearlooks' own, out of its gtkrc in Debian's gtk2-engines
 * (usr/share/themes/Clearlooks/gtk-2.0/gtkrc).  GTK computes three of
 * these at runtime with shade(); a framebuffer has no runtime to compute
 * them in, so they are precomputed here and the factor is named beside
 * each one.
 */
#define TRAIT_BG 0xEDECEBU           /* bg_color */
#define TRAIT_BG_PRELIGHT 0xF2F1F0U  /* shade(1.02, bg) */
#define TRAIT_BG_ACTIVE 0xD5D4D3U    /* shade(0.90, bg) */
#define TRAIT_BASE 0xFFFFFFU         /* base_color */
#define TRAIT_BASE_PRELIGHT 0xE1E0DFU/* shade(0.95, bg) */
#define TRAIT_FG 0x000000U           /* fg_color */
#define TRAIT_TEXT 0x1A1A1AU         /* text_color */
#define TRAIT_SEL_BG 0x86ABD9U       /* selected_bg_color */
#define TRAIT_SEL_FG 0xFFFFFFU       /* selected_fg_color */
#define TRAIT_LINE 0xB6B3AFU
#define TRAIT_LINE_LIGHT 0xFAFAF9U

/*
 * The window frame.  Openbox draws the title bar as a vertical ramp, so
 * these are its two ends: an active window is lighter and its text is
 * brighter, which is the whole of how a focused window says so.
 */
#define TRAIT_FRAME_ACTIVE_TOP 0x5B6472U
#define TRAIT_FRAME_ACTIVE_BOTTOM 0x38404CU
#define TRAIT_FRAME_IDLE_TOP 0x3B3F46U
#define TRAIT_FRAME_IDLE_BOTTOM 0x2B2F35U
#define TRAIT_FRAME_INK 0xD9DDE3U
#define TRAIT_FRAME_INK_DIM 0x9AA0A8U

#endif /* TRAIT_THEME_H */
