/* SPDX-License-Identifier: GPL-3.0-only */
#include <trait/theme.h>

/*
 * THREE THEMES, AND ALL THREE ARE REAL ONES.
 *
 * Clearlooks and Adwaita are the palettes Debian's own gtkrc files carry;
 * Adwaita-dark is the dark variant's.  Nothing here is a colour somebody
 * liked - inventing a fourth "theme" would make the list longer and the
 * claim that these are the installed themes false.
 */
static const struct trait_palette PALETTES[] = {
    /* Clearlooks: gtk2-engines, usr/share/themes/Clearlooks/gtk-2.0/gtkrc */
    {
        0xEDECEBU, 0xF2F1F0U, 0xD5D4D3U, 0xFFFFFFU, 0xE1E0DFU,
        0x000000U, 0x1A1A1AU, 0x86ABD9U, 0xFFFFFFU, 0xB6B3AFU, 0xFAFAF9U,
        0x5B6472U, 0x38404CU, 0x3B3F46U, 0x2B2F35U, 0xD9DDE3U, 0x9AA0A8U
    },
    /* Adwaita's GTK2 palette, from the gtkrc gnome-themes-extra-data
     * ships beside Clearlooks', with the same shade() factors applied. */
    {
        0xEDEDEDU, 0xF2F2F2U, 0xD5D5D5U, 0xFFFFFFU, 0xE1E1E1U,
        0x2E3436U, 0x2E3436U, 0x4A90D9U, 0xFFFFFFU, 0xC3C3C3U, 0xFBFBFBU,
        0x5C616CU, 0x393F45U, 0x3C4048U, 0x2D3036U, 0xDEDEDEU, 0x93999FU
    },
    /* Adwaita-dark. The frame goes darker with it - a dark theme with a
     * light title bar is two themes at once. */
    {
        0x3C3C3CU, 0x464646U, 0x2E2E2EU, 0x2B2B2BU, 0x353535U,
        0xEEEEECU, 0xEEEEECU, 0x215D9CU, 0xFFFFFFU, 0x1B1B1BU, 0x4A4A4AU,
        0x2F2F2FU, 0x1E1E1EU, 0x262626U, 0x1A1A1AU, 0xD8D8D6U, 0x8A8A88U
    }
};

static const char *const NAMES[] = {
    "Clearlooks", "Adwaita", "Adwaita-dark"
};

#define THEME_COUNT (sizeof(PALETTES) / sizeof(PALETTES[0]))

static uint32_t current;

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
