/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_MENU_H
#define TRAIT_MENU_H

#include <stdbool.h>
#include <stdint.h>

#include <trait/surface.h>

/*
 * The panel's applications menu.
 *
 * The profile's menu plugin lists its contents literally:
 *
 *     system { }  |  separator  |  item{command=run}
 *
 * `system` is the application menu, which LXDE builds from the .desktop
 * files on the machine and groups by their freedesktop category.  A
 * category with nothing in it is not drawn, because LXDE does not draw
 * one either.
 *
 * IT OPENS UPWARDS off the button, because the panel is at the foot of
 * the screen - a menu that dropped down from a bottom panel would go off
 * the bottom of the display.
 *
 * WHAT IS IN IT IS NOT A CHOICE MADE HERE: an entry is a package, and a
 * package that is not installed is not in the menu.  That is what makes
 * the package manager a package manager rather than a shop window.
 */

#define TRAIT_MENU_MAX_ROWS 16U
#define TRAIT_MENU_TEXT_BYTES 32U

struct trait_menu_row {
    char label[TRAIT_MENU_TEXT_BYTES];
    bool category;     /* has a submenu arrow */
    bool rule;         /* a separator, and the label is ignored */
};

void trait_menu_reset(void);
bool trait_menu_add(const char *label, bool category, bool rule);
uint32_t trait_menu_row_count(void);

/* Where the menu sits given the button it hangs off and the screen. */
struct trait_rect trait_menu_bounds(struct trait_rect screen,
    struct trait_rect button);

void trait_menu_draw(struct trait_surface *surface,
    struct trait_rect screen, struct trait_rect button);

bool trait_menu_self_test(void);

#endif /* TRAIT_MENU_H */
