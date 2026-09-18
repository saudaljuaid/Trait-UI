/* SPDX-License-Identifier: GPL-3.0-only */
#include <trait/theme.h>

#include "trait_relief.h"

/*
 * FIVE THEMES, AND EVERY COLOUR IN THEM IS TAKEN FROM SOMEWHERE.
 *
 * Clearlooks and Adwaita are the palettes Debian's own gtkrc files carry;
 * Adwaita-dark is the dark variant's.  Those three are the GTK themes on
 * the machine, and nothing in them is a colour somebody liked.
 *
 * The fourth is NOT a GTK theme and does not pretend to be one - it is
 * this desktop's own, and it is here because the desktop has an identity
 * that the LXDE defaults do not carry.  Its colours are not invented
 * either: they are SAMPLED from the owner's artwork.  The selection gold
 * is #B08020, one of the onion's own ambers; the ground is the
 * wallpaper's #212121 charcoal, read out of the corner of the PNG.  A
 * theme whose accent is guessed at sits next to the wallpaper and argues
 * with it.
 */
static const struct trait_palette PALETTES[] = {
    /* Clearlooks: gtk2-engines, usr/share/themes/Clearlooks/gtk-2.0/gtkrc */
    {
        0xEDECEBU, 0xF2F1F0U, 0xD5D4D3U, 0xFFFFFFU, 0xE1E0DFU,
        0x000000U, 0x1A1A1AU, 0x86ABD9U, 0xFFFFFFU, 0xB6B3AFU, 0xFAFAF9U,
        TRAIT_RELIEF_CLEARLOOKS_ACTIVE,
        TRAIT_RELIEF_CLEARLOOKS_IDLE, 0xD9DDE3U, 0x9AA0A8U
    },
    /* Adwaita's GTK2 palette, from the gtkrc gnome-themes-extra-data
     * ships beside Clearlooks', with the same shade() factors applied. */
    {
        0xEDEDEDU, 0xF2F2F2U, 0xD5D5D5U, 0xFFFFFFU, 0xE1E1E1U,
        0x2E3436U, 0x2E3436U, 0x4A90D9U, 0xFFFFFFU, 0xC3C3C3U, 0xFBFBFBU,
        TRAIT_RELIEF_ADWAITA_ACTIVE,
        TRAIT_RELIEF_ADWAITA_IDLE, 0xDEDEDEU, 0x93999FU
    },
    /* Adwaita-dark. The frame goes darker with it - a dark theme with a
     * light title bar is two themes at once. */
    {
        0x3C3C3CU, 0x464646U, 0x2E2E2EU, 0x2B2B2BU, 0x353535U,
        0xEEEEECU, 0xEEEEECU, 0x215D9CU, 0xFFFFFFU, 0x1B1B1BU, 0x4A4A4AU,
        TRAIT_RELIEF_ADWAITA_DARK_ACTIVE,
        TRAIT_RELIEF_ADWAITA_DARK_IDLE, 0xD8D8D6U, 0x8A8A88U
    },
    /*
     * Trait.  Sampled, not chosen:
     *   #212121  the wallpaper's ground, from the corner of the PNG
     *   #B08020  the onion's amber, its sixth most common opaque colour
     *   #503000  the onion's darkest amber, which the title ramp ends on
     * The selected foreground is dark because the selection is GOLD and
     * white on gold is the one pairing this palette could get wrong.
     */
    {
        0x2B2B2BU, 0x333333U, 0x212121U, 0x232323U, 0x2A2A2AU,
        0xE8E6E0U, 0xE0DED6U, 0xB08020U, 0x1A1400U, 0x151515U, 0x3A3A3AU,
        TRAIT_RELIEF_TRAIT_ACTIVE,
        TRAIT_RELIEF_TRAIT_IDLE, 0xE8E6E0U, 0x8A8780U
    },
    /*
     * OpenRFS.  Sixteen colours and nothing between them.
     *
     * The palette is the IBM text one, which is what TempleOS drew in
     * and what tools/render.c paints the console and the installer
     * with - the same table, so the three do not end up three slightly
     * different reds.  Red is the brand's #9E1B1B rather than IBM's
     * #AA0000, for the same reason.
     *
     * Every pair here is two of those sixteen. Nothing is shaded, no
     * value is computed from another, and the two frame ends are equal
     * because a gradient needs colours that are not in the palette to
     * get from one end to the other.
     */
    {
        0xAAAAAAU,   /* bg          light grey */
        0xAAAAAAU,   /* prelight    the same: there is nothing to shade */
        0x555555U,   /* active      dark grey */
        0xFFFFFFU,   /* base        white */
        0xFFFFFFU,   /* base pre    white */
        0x000000U,   /* fg          black */
        0x000000U,   /* text        black */
        0x0000AAU,   /* sel bg      blue */
        0xFFFFFFU,   /* sel fg      white */
        0x000000U,   /* line        black */
        0xFFFFFFU,   /* line light  white, for the lit side of a bevel */
        /*
         * The frame is fvwm's, not this project's.  OpenBSD's
         * system.fvwmrc says HilightColor #bebebe blue for the focused
         * window and Color #bebebe/darkred for every other one, and
         * that is what these two lines are - ground, lit side, shadow
         * side, the last two computed by fvwm's own GetHilite() and
         * GetShadow().  The brand red is not here: a title bar in
         * #9E1B1B is this desktop wearing fvwm's shape in its own
         * colours, which is the thing it was asked not to be.
         */
        TRAIT_RELIEF_OPENRFS_ACTIVE,
        TRAIT_RELIEF_OPENRFS_IDLE,
        0xBEBEBEU,   /* frame ink       fvwm's #bebebe, both states */
        0xBEBEBEU    /* frame ink dim   the same: fvwm dims the GROUND */
    }
};

static const char *const NAMES[] = {
    "Clearlooks", "Adwaita", "Adwaita-dark", "Trait", "OpenRFS"
};

#define THEME_COUNT (sizeof(PALETTES) / sizeof(PALETTES[0]))

/* OpenRFS.  The LXDE themes are still in the list, one press of the
 * Widget row away, because they are installed and a list that hides
 * what is installed is a shorter list than the machine has. */
static uint32_t current = TRAIT_THEME_DEFAULT;

const struct trait_palette *trait_theme(void)
{
    return &PALETTES[current];
}

uint32_t trait_theme_count(void)
{
    return (uint32_t)THEME_COUNT;
}

const char *trait_theme_name(uint32_t at)
{
    if (at >= THEME_COUNT) {
        return "";
    }
    return NAMES[at];
}

bool trait_theme_select(uint32_t at)
{
    if (at >= THEME_COUNT) {
        return false;
    }
    current = at;
    return true;
}

uint32_t trait_theme_selected(void)
{
    return current;
}

bool trait_theme_select_named(const char *name)
{
    uint32_t at;

    if (name == NULL) {
        return false;
    }
    for (at = 0U; at < THEME_COUNT; ++at) {
        uint32_t byte = 0U;

        while (NAMES[at][byte] != '\0' && name[byte] != '\0' &&
                NAMES[at][byte] == name[byte]) {
            ++byte;
        }
        if (NAMES[at][byte] == '\0' && name[byte] == '\0') {
            current = at;
            return true;
        }
    }
    return false;
}
