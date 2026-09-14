/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_SHELL_H
#define TRAIT_SHELL_H

#include <stdbool.h>
#include <stdint.h>

#include <trait/input.h>
#include <trait/surface.h>
#include <trait/window.h>

/*
 * THE SHELL: what owns the windows and routes the events.
 *
 * Every module below it draws and models; none of them knows another
 * exists.  This is the one place that knows there is more than one
 * window, which is why it is the one place that can say what a click on
 * a given pixel means.
 *
 * The rule it enforces is the one the whole desktop is built on: a click
 * lands on the TOPMOST thing under it, and that thing does what it is
 * drawn as.  A control that is covered does not receive the click that
 * looks like it landed on the thing above it.
 */

#define TRAIT_SHELL_MAX_WINDOWS 8U

enum trait_shell_app {
    TRAIT_APP_FILES = 0,
    TRAIT_APP_TERMINAL,
    TRAIT_APP_TASKMGR,
    TRAIT_APP_SETTINGS,
    TRAIT_APP_PACKAGES,
    TRAIT_APP_COUNT
};

void trait_shell_reset(struct trait_surface *surface);

/* Returns the slot, or TRAIT_SHELL_MAX_WINDOWS if there is no room. */
uint32_t trait_shell_open(enum trait_shell_app app, struct trait_rect at);
bool trait_shell_close(uint32_t slot);
uint32_t trait_shell_window_count(void);
struct trait_window *trait_shell_window(uint32_t slot);
enum trait_shell_app trait_shell_app_of(uint32_t slot);

/* The window under a point, topmost first, or TRAIT_SHELL_MAX_WINDOWS. */
uint32_t trait_shell_at(uint32_t x, uint32_t y);
uint32_t trait_shell_focused(void);
void trait_shell_focus(uint32_t slot);

/* Returns true if the event changed anything, so a caller can redraw
 * only when it must. */
bool trait_shell_handle(const struct trait_event *event);

void trait_shell_draw(void);

bool trait_shell_self_test(void);

#endif /* TRAIT_SHELL_H */
