/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_WINDOW_H
#define TRAIT_WINDOW_H

#include <stdbool.h>
#include <stdint.h>

#include <trait/surface.h>

/*
 * An Openbox frame: a title bar with the window's name and its three
 * buttons, and a one-pixel border round the client area.
 *
 * The frame owns nothing inside it.  trait_window_client() says where the
 * application may draw and the application draws there; that is the whole
 * contract, and it is why the Task Manager and Settings below can be
 * written without either of them knowing what a title bar looks like.
 */

#define TRAIT_TITLE_HEIGHT 22U
#define TRAIT_BORDER 1U
#define TRAIT_TITLE_BYTES 48U

struct trait_window {
    char title[TRAIT_TITLE_BYTES];
    struct trait_rect frame;
    bool active;
};

struct trait_rect trait_window_client(const struct trait_window *window);
struct trait_rect trait_window_title(const struct trait_window *window);
void trait_window_draw(struct trait_surface *surface,
    const struct trait_window *window);
void trait_window_set_title(struct trait_window *window, const char *text);

#endif /* TRAIT_WINDOW_H */
