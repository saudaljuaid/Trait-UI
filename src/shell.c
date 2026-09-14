/* SPDX-License-Identifier: GPL-3.0-only */
#include <trait/shell.h>

#include <trait/files.h>
#include <trait/packages.h>
#include <trait/panel.h>
#include <trait/settings.h>
#include <trait/taskmgr.h>
#include <trait/terminal.h>

static struct trait_surface *canvas;
static struct trait_window windows[TRAIT_SHELL_MAX_WINDOWS];
static enum trait_shell_app apps[TRAIT_SHELL_MAX_WINDOWS];
static bool used[TRAIT_SHELL_MAX_WINDOWS];

/*
 * THE STACK, TOP LAST.  Drawing walks it forwards and hit-testing walks
 * it backwards, which is the whole of why a click lands on the window you
 * can see rather than the one underneath it.  Holding the order in one
 * array rather than as a z field per window means the two can never
 * disagree.
 */
static uint32_t stack[TRAIT_SHELL_MAX_WINDOWS];
static uint32_t stack_depth;

/* Where a drag started, and what it is moving. */
static bool dragging;
static uint32_t drag_slot;
static uint32_t drag_dx;
static uint32_t drag_dy;

static const char *const TITLES[TRAIT_APP_COUNT] = {
    "user", "user@trait: ~", "Task Manager", "Desktop Preferences",
    "Package Manager"
};

static void set_title(struct trait_window *window, const char *text)
{
    trait_window_set_title(window, text);
}

static void stack_remove(uint32_t slot)
{
    uint32_t at;

    for (at = 0U; at < stack_depth; ++at) {
        if (stack[at] != slot) {
            continue;
        }
        for (; at + 1U < stack_depth; ++at) {
            stack[at] = stack[at + 1U];
        }
        --stack_depth;
        return;
    }
}

static void stack_raise(uint32_t slot)
{
    stack_remove(slot);
    if (stack_depth < TRAIT_SHELL_MAX_WINDOWS) {
        stack[stack_depth++] = slot;
    }
}

void trait_shell_reset(struct trait_surface *surface)
{
    uint32_t at;

    canvas = surface;
    stack_depth = 0U;
    dragging = false;
    for (at = 0U; at < TRAIT_SHELL_MAX_WINDOWS; ++at) {
        used[at] = false;
    }
}

uint32_t trait_shell_open(enum trait_shell_app app, struct trait_rect at)
{
    uint32_t slot;

    if ((uint32_t)app >= TRAIT_APP_COUNT) {
        return TRAIT_SHELL_MAX_WINDOWS;
    }
    for (slot = 0U; slot < TRAIT_SHELL_MAX_WINDOWS; ++slot) {
        if (!used[slot]) {
            break;
        }
    }
    if (slot == TRAIT_SHELL_MAX_WINDOWS) {
        return TRAIT_SHELL_MAX_WINDOWS;
    }
    used[slot] = true;
    apps[slot] = app;
    windows[slot].frame = at;
    windows[slot].active = false;
    set_title(&windows[slot], TITLES[app]);
    stack_raise(slot);
    trait_shell_focus(slot);
    return slot;
}

bool trait_shell_close(uint32_t slot)
{
    if (slot >= TRAIT_SHELL_MAX_WINDOWS || !used[slot]) {
        return false;
    }
    used[slot] = false;
    stack_remove(slot);
    /* Focus falls to whatever is now on top, not to nothing: a desktop
     * with windows open and none focused is a state nobody asked for. */
    if (stack_depth != 0U) {
        trait_shell_focus(stack[stack_depth - 1U]);
    }
    return true;
}

uint32_t trait_shell_window_count(void)
{
    return stack_depth;
}

struct trait_window *trait_shell_window(uint32_t slot)
{
    if (slot >= TRAIT_SHELL_MAX_WINDOWS || !used[slot]) {
        return NULL;
    }
    return &windows[slot];
}

enum trait_shell_app trait_shell_app_of(uint32_t slot)
{
    if (slot >= TRAIT_SHELL_MAX_WINDOWS || !used[slot]) {
        return TRAIT_APP_COUNT;
    }
    return apps[slot];
}

uint32_t trait_shell_at(uint32_t x, uint32_t y)
{
    uint32_t at = stack_depth;

    /* BACKWARDS: topmost first. */
    while (at != 0U) {
        uint32_t slot = stack[--at];

        if (trait_rect_contains(windows[slot].frame, x, y)) {
            return slot;
        }
    }
    return TRAIT_SHELL_MAX_WINDOWS;
}

uint32_t trait_shell_focused(void)
{
    if (stack_depth == 0U) {
        return TRAIT_SHELL_MAX_WINDOWS;
    }
    return stack[stack_depth - 1U];
}

void trait_shell_focus(uint32_t slot)
{
    uint32_t at;

    if (slot >= TRAIT_SHELL_MAX_WINDOWS || !used[slot]) {
        return;
    }
    stack_raise(slot);
    for (at = 0U; at < TRAIT_SHELL_MAX_WINDOWS; ++at) {
        windows[at].active = used[at] && at == slot;
    }
}

/* The close button's box, which is where the title bar's cross is drawn -
 * one definition, so the thing that is drawn and the thing that answers
 * a click cannot drift apart. */
static bool close_box(uint32_t slot, struct trait_rect *out)
{
    struct trait_rect title = trait_window_title(&windows[slot]);

    if (title.width < 90U) {
        return false;
    }
    out->x = title.x + title.width - 6U - 8U - 2U;
    out->y = title.y + (title.height - 8U) / 2U - 2U;
    out->width = 12U;
    out->height = 12U;
    return true;
}

static bool handle_client(uint32_t slot, const struct trait_event *event)
{
    struct trait_rect client = trait_window_client(&windows[slot]);
    uint32_t at;

    switch (apps[slot]) {
    case TRAIT_APP_TASKMGR:
        /* A press on a column header sorts by it. */
        for (at = 0U; at < TRAIT_TASKMGR_COLUMNS; ++at) {
            struct trait_rect head;

            if (!trait_taskmgr_header_bounds(&windows[slot],
                    (enum trait_taskmgr_column)at, &head)) {
                continue;
            }
            if (trait_rect_contains(head, event->x, event->y)) {
                trait_taskmgr_sort((enum trait_taskmgr_column)at);
                return true;
            }
        }
        return false;
    case TRAIT_APP_SETTINGS:
        for (at = 0U; at < trait_settings_page_count(); ++at) {
            struct trait_rect tab;

            if (!trait_settings_tab_bounds(&windows[slot], at, &tab)) {
                continue;
            }
            if (trait_rect_contains(tab, event->x, event->y)) {
                trait_settings_select(at);
                return true;
            }
        }
        return false;
    case TRAIT_APP_FILES:
        for (at = 0U; at < trait_files_child_count(trait_files_here());
                ++at) {
            struct trait_rect cell;
            uint32_t node;

            if (!trait_files_entry_bounds(&windows[slot], at, &cell)) {
                continue;
            }
            if (!trait_rect_contains(cell, event->x, event->y)) {
                continue;
            }
            node = trait_files_child(trait_files_here(), at);
            if (event->double_click) {
                /* Opening a FILE is not opening a folder, and pretending
                 * it is would be the file manager lying about what it
                 * did.  Only a folder opens. */
                (void)trait_files_open(node);
                return true;
            }
            trait_files_select(node,
                (event->modifiers & TRAIT_MOD_CTRL) != 0U);
            return true;
        }
        if (trait_rect_contains(client, event->x, event->y)) {
            trait_files_clear_selection();
            return true;
        }
        return false;
    case TRAIT_APP_PACKAGES:
        for (at = 0U; at < trait_packages_count(); ++at) {
            struct trait_rect row;

            row.x = client.x;
            row.y = client.y + 30U + at * 19U;
            row.width = client.width;
            row.height = 19U;
            if (trait_rect_contains(row, event->x, event->y)) {
                trait_packages_select(at);
                return true;
            }
        }
        return false;
    case TRAIT_APP_TERMINAL:
    default:
        return false;
    }
}

bool trait_shell_handle(const struct trait_event *event)
{
    uint32_t slot;
    struct trait_rect title;
    struct trait_rect close;

    if (event == NULL) {
        return false;
    }
    if (event->kind == TRAIT_EVENT_KEY) {
        slot = trait_shell_focused();

        if (slot >= TRAIT_SHELL_MAX_WINDOWS) {
            return false;
        }
        /* A-F4 closes the FOCUSED window, which is the one the keyboard
         * is talking to - not the one under the pointer. */
        if (event->special == TRAIT_KEY_F4 &&
                (event->modifiers & TRAIT_MOD_ALT) != 0U) {
            return trait_shell_close(slot);
        }
        if (apps[slot] == TRAIT_APP_TERMINAL &&
                event->special == TRAIT_KEY_ENTER) {
            trait_terminal_run("");
            return true;
        }
        return false;
    }

    if (event->kind == TRAIT_EVENT_POINTER_MOVE) {
        if (!dragging) {
            return false;
        }
        windows[drag_slot].frame.x = event->x > drag_dx ?
            event->x - drag_dx : 0U;
        windows[drag_slot].frame.y = event->y > drag_dy ?
            event->y - drag_dy : 0U;
        return true;
    }

    if (event->kind == TRAIT_EVENT_POINTER_UP) {
        bool was = dragging;

        dragging = false;
        return was;
    }

    slot = trait_shell_at(event->x, event->y);
    if (slot >= TRAIT_SHELL_MAX_WINDOWS) {
        return false;
    }
    /* Whatever else the press does, it RAISES: that is what clicking a
     * window means, and doing it before anything else means the rest of
     * this function is always talking about the window on top. */
    trait_shell_focus(slot);

    if (close_box(slot, &close) &&
            trait_rect_contains(close, event->x, event->y)) {
        return trait_shell_close(slot);
    }
    title = trait_window_title(&windows[slot]);
    if (trait_rect_contains(title, event->x, event->y)) {
        dragging = true;
        drag_slot = slot;
        drag_dx = event->x - windows[slot].frame.x;
        drag_dy = event->y - windows[slot].frame.y;
        return true;
    }
    (void)handle_client(slot, event);
    return true;
}

void trait_shell_draw(void)
{
    uint32_t at;

    if (!trait_surface_valid(canvas)) {
        return;
    }
    /* FORWARDS: bottom first, so the top window is drawn last and covers
     * what it is over.  The same order the hit test walks backwards. */
    for (at = 0U; at < stack_depth; ++at) {
        uint32_t slot = stack[at];

        trait_window_draw(canvas, &windows[slot]);
        switch (apps[slot]) {
        case TRAIT_APP_FILES:
            trait_files_draw(canvas, &windows[slot]);
            break;
        case TRAIT_APP_TERMINAL:
            trait_terminal_draw(canvas, &windows[slot]);
            break;
        case TRAIT_APP_TASKMGR:
            trait_taskmgr_draw(canvas, &windows[slot]);
            break;
        case TRAIT_APP_SETTINGS:
            trait_settings_draw(canvas, &windows[slot]);
            break;
        case TRAIT_APP_PACKAGES:
            trait_packages_draw(canvas, &windows[slot]);
            break;
        default:
            break;
        }
    }
}

/*
 * The self test asks the three things a window manager is FOR, and that
 * a pile of independently-correct modules still gets wrong:
 *
 *   - does a click land on the window you can SEE, not the one under it?
 *   - does clicking a window raise it, so the next click lands there?
 *   - when a window closes, does focus go somewhere real?
 */
bool trait_shell_self_test(void)
{
    struct trait_event press;
    uint32_t lower;
    uint32_t upper;

    trait_shell_reset(canvas);
    lower = trait_shell_open(TRAIT_APP_TASKMGR,
        (struct trait_rect){ 100U, 100U, 300U, 200U });
    upper = trait_shell_open(TRAIT_APP_TERMINAL,
        (struct trait_rect){ 200U, 150U, 300U, 200U });
    if (lower >= TRAIT_SHELL_MAX_WINDOWS ||
            upper >= TRAIT_SHELL_MAX_WINDOWS) {
        return false;
    }
    /* Opened second, so it is on top and focused. */
    if (trait_shell_focused() != upper) {
        return false;
    }
    /* A point inside BOTH frames belongs to the upper one. */
    if (trait_shell_at(250U, 200U) != upper) {
        return false;
    }
    /* A point inside only the lower one belongs to it. */
    if (trait_shell_at(120U, 120U) != lower) {
        return false;
    }
    /* Clicking the lower one raises it, and then the overlap is ITS. */
    press.kind = TRAIT_EVENT_POINTER_DOWN;
    press.x = 120U;
    press.y = 120U;
    press.modifiers = 0U;
    press.key = 0;
    press.special = 0U;
    press.double_click = false;
    if (!trait_shell_handle(&press)) {
        return false;
    }
    if (trait_shell_focused() != lower) {
        return false;
    }
    if (trait_shell_at(250U, 200U) != lower) {
        return false;
    }
    /* Closing the focused one leaves focus on something real. */
    if (!trait_shell_close(lower)) {
        return false;
    }
    if (trait_shell_focused() != upper) {
        return false;
    }
    if (trait_shell_window_count() != 1U) {
        return false;
    }
    /* Closing the last one leaves nothing focused, and says so rather
     * than returning a slot that is not open. */
    if (!trait_shell_close(upper)) {
        return false;
    }
    if (trait_shell_focused() != TRAIT_SHELL_MAX_WINDOWS) {
        return false;
    }
    if (trait_shell_window(upper) != NULL) {
        return false;
    }
    trait_shell_reset(canvas);
    return true;
}
