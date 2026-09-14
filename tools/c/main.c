/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * The host harness for the C desktop.
 *
 * The shell draws into a plain surface and knows nothing about files;
 * this allocates one, loads the wallpaper, asks the panel to draw, and
 * writes PNGs.  On real hardware the surface is the framebuffer and this
 * program does not exist.
 *
 *     make -C tools/c run
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <trait/font.h>
#include <trait/panel.h>
#include <trait/settings.h>
#include <trait/taskmgr.h>
#include <trait/theme.h>
#include <trait/window.h>
#include <trait/surface.h>

#include "png.h"

#define SCREEN_WIDTH 1280U
#define SCREEN_HEIGHT 800U

static uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static struct trait_surface screen = {
    framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT
};

static struct trait_rect whole(void)
{
    return (struct trait_rect){ 0U, 0U, SCREEN_WIDTH, SCREEN_HEIGHT };
}

/*
 * The wallpaper, as the raw RGB24 dump tools/c/make-wallpaper.py writes.
 * There is no image decoder here on purpose: a desktop that needs a PNG
 * decoder to put up a background needs one in the kernel.
 */
static int load_wallpaper(const char *path)
{
    FILE *file = fopen(path, "rb");
    size_t want = (size_t)SCREEN_WIDTH * SCREEN_HEIGHT * 3U;
    uint8_t *bytes;
    size_t got;

    if (file == NULL) {
        return 0;
    }
    bytes = malloc(want);
    if (bytes == NULL) {
        (void)fclose(file);
        return 0;
    }
    got = fread(bytes, 1U, want, file);
    (void)fclose(file);
    if (got != want) {
        free(bytes);
        return 0;
    }
    for (uint32_t y = 0U; y < SCREEN_HEIGHT; ++y) {
        for (uint32_t x = 0U; x < SCREEN_WIDTH; ++x) {
            size_t at = ((size_t)y * SCREEN_WIDTH + x) * 3U;

            trait_surface_plot(&screen, whole(), x, y,
                ((uint32_t)bytes[at] << 16) |
                ((uint32_t)bytes[at + 1U] << 8) | bytes[at + 2U]);
        }
    }
    free(bytes);
    return 1;
}

static void flat(uint32_t colour)
{
    trait_surface_fill(&screen, whole(), whole(), colour);
}

static int emit(const char *directory, const char *name,
    struct trait_rect area)
{
    char path[512];
    uint32_t *cut;
    int ok;

    snprintf(path, sizeof(path), "%s/%s", directory, name);
    if (area.width == SCREEN_WIDTH && area.height == SCREEN_HEIGHT) {
        ok = png_write(path, framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    } else {
        cut = malloc((size_t)area.width * area.height * sizeof(*cut));
        if (cut == NULL) {
            return 0;
        }
        for (uint32_t y = 0U; y < area.height; ++y) {
            for (uint32_t x = 0U; x < area.width; ++x) {
                cut[(size_t)y * area.width + x] =
                    trait_surface_read(&screen, area.x + x, area.y + y);
            }
        }
        ok = png_write(path, cut, area.width, area.height);
        free(cut);
    }
    if (!ok) {
        fprintf(stderr, "trait: could not write %s\n", path);
        return 0;
    }
    printf("wrote %s (%ux%u)\n", path, area.width, area.height);
    return 1;
}

static void populate(void)
{
    static const char *const NAMES[4] = {
        "user", "user@trait: ~", "Task Manager", "Settings"
    };
    static const char *const ICONS[4] = {
        "file-manager", "terminal", "gtk-preferences", "gtk-preferences"
    };
    /* A shape rather than a flat load, so the graph reads as a graph. */
    static const uint32_t LOAD[TRAIT_PANEL_CPU_COLUMNS] = {
        4U, 6U, 5U, 7U, 6U, 8U, 7U, 9U, 12U, 18U, 26U, 35U,
        44U, 52U, 61U, 70U, 78U, 85U, 91U, 94U, 96U, 92U, 84U, 73U,
        61U, 50U, 41U, 33U, 27U, 22U, 18U, 15U, 12U, 10U, 8U, 7U
    };
    struct trait_panel_task task;

    for (uint32_t at = 0U; at < TRAIT_PANEL_CPU_COLUMNS; ++at) {
        (void)trait_panel_push_cpu(LOAD[at]);
    }
    for (uint32_t at = 0U; at < 4U; ++at) {
        memset(&task, 0, sizeof(task));
        task.icon = ICONS[at];
        task.active = at == 1U;
        task.minimised = at == 3U;
        task.desktop = 0U;
        (void)snprintf(task.label, TRAIT_PANEL_LABEL_BYTES, "%s",
                       NAMES[at]);
        (void)trait_panel_set_task(at, &task);
    }
    (void)trait_panel_set_clock("15:43");
    (void)trait_panel_set_desktop(0U, 2U);
    (void)trait_panel_set_volume(65U, false);
}


/* The Task Manager's rows.  They are the desktop's own processes, which
 * is what lxtask lists - a COMMAND, not a window title. */
static void populate_taskmgr(void)
{
    static const struct {
        const char *command;
        uint32_t cpu;
        uint32_t rss;
        uint32_t pid;
    } TASKS[6] = {
        { "trait-session", 28U, 2458U, 1U },
        { "pcmanfm", 12U, 3810U, 2U },
        { "lxterminal", 41U, 5122U, 3U },
        { "lxtask", 8U, 1904U, 4U },
        { "openbox", 3U, 2201U, 5U },
        { "lxpanel", 19U, 3355U, 6U }
    };
    struct trait_taskmgr_row row;

    trait_taskmgr_reset();
    for (uint32_t at = 0U; at < 6U; ++at) {
        memset(&row, 0, sizeof(row));
        (void)snprintf(row.command, TRAIT_TASKMGR_NAME_BYTES, "%s",
                       TASKS[at].command);
        (void)snprintf(row.user, TRAIT_TASKMGR_NAME_BYTES, "user");
        row.cpu_tenths = TASKS[at].cpu;
        row.rss_kib = TASKS[at].rss;
        row.pid = TASKS[at].pid;
        (void)trait_taskmgr_add(&row);
    }
}

/* Settings, as the three LXDE programs it stands in for. */
static void populate_settings(void)
{
    struct trait_settings_row row;

    trait_settings_reset();
    (void)trait_settings_add_page("Widget");
    (void)trait_settings_add_page("Desktop");
    (void)trait_settings_add_page("Panel");
    (void)trait_settings_add_page("Keyboard");

    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_CHOICE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Widget theme");
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES, "Clearlooks");
    (void)trait_settings_add_row(0U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Font size");
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES, "Normal (11)");
    (void)trait_settings_add_row(0U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Icon theme");
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES, "nuoveXT2");
    (void)trait_settings_add_row(0U, &row);
    row.kind = TRAIT_SETTINGS_NOTE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "One theme, because one is installed.");
    (void)trait_settings_add_row(0U, &row);

    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_CHOICE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Wallpaper mode");
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES, "Crop to fit");
    (void)trait_settings_add_row(1U, &row);
    row.kind = TRAIT_SETTINGS_SWITCH;
    row.on = true;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Show icons on the desktop");
    (void)trait_settings_add_row(1U, &row);

    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_CHOICE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Position");
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES, "Bottom");
    (void)trait_settings_add_row(2U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Height");
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES, "26 (default)");
    (void)trait_settings_add_row(2U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Clock format");
    /* %%R, because this string goes through printf and the
     * profile's own ClockFmt is literally "%R". */
    (void)snprintf(row.value, TRAIT_SETTINGS_TEXT_BYTES,
                   "%%R - 15:43");
    (void)trait_settings_add_row(2U, &row);

    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_NOTE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "W-e   Open the file manager");
    (void)trait_settings_add_row(3U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "W-r   Open the Run box");
    (void)trait_settings_add_row(3U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "C-A-t Open a terminal");
    (void)trait_settings_add_row(3U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "A-F4  Close the focused window");
    (void)trait_settings_add_row(3U, &row);
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : "build/c";
    struct trait_rect bar;

    if (!trait_panel_self_test()) {
        fprintf(stderr, "trait: panel self-test failed\n");
        return 1;
    }
    if (trait_panel_attach(&screen) != TRAIT_PANEL_STATUS_OK ||
            trait_panel_initialize() != TRAIT_PANEL_STATUS_OK) {
        fprintf(stderr, "trait: panel refused to start\n");
        return 1;
    }
    populate();

    if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
        /* The ground the wallpaper is drawn on, so a missing dump is a
         * plain backdrop rather than whatever was in memory. */
        flat(0x212121U);
        fprintf(stderr, "trait: no wallpaper dump, using a flat ground\n");
    }
    if (trait_panel_draw(whole()) != TRAIT_PANEL_STATUS_OK) {
        fprintf(stderr, "trait: panel refused to draw\n");
        return 1;
    }
    bar = trait_panel_bounds(whole());
    if (!emit(out, "desktop.png", whole()) ||
            !emit(out, "panel.png", bar)) {
        return 1;
    }

    /* The two windows, over the same desktop, so they are seen sitting on
     * something rather than on a flat grey. */
    if (!trait_taskmgr_self_test()) {
        fprintf(stderr, "trait: task manager self-test failed\n");
        return 1;
    }
    if (!trait_settings_self_test()) {
        fprintf(stderr, "trait: settings self-test failed\n");
        return 1;
    }
    populate_taskmgr();
    populate_settings();
    {
        struct trait_window taskmgr;
        struct trait_window settings;

        memset(&taskmgr, 0, sizeof(taskmgr));
        taskmgr.frame = (struct trait_rect){ 120U, 120U, 520U, 340U };
        taskmgr.active = false;
        trait_window_set_title(&taskmgr, "Task Manager");

        memset(&settings, 0, sizeof(settings));
        settings.frame = (struct trait_rect){ 470U, 290U, 520U, 300U };
        settings.active = true;
        trait_window_set_title(&settings, "Desktop Preferences");

        /* Sorted by CPU, descending, which is what anybody opens a task
         * manager to see. */
        trait_taskmgr_sort(TRAIT_TASKMGR_CPU);
        trait_taskmgr_sort(TRAIT_TASKMGR_CPU);
        trait_settings_select(2U);

        /*
         * Each window gets its own crop BEFORE the other is drawn over
         * it.  Cropping the finished screen gave a Task Manager with the
         * Settings window sitting in the corner of it - a true picture of
         * the screen and a useless picture of the window.
         */
        trait_window_draw(&screen, &taskmgr);
        trait_taskmgr_draw(&screen, &taskmgr);
        if (!emit(out, "taskmgr.png", taskmgr.frame)) {
            return 1;
        }
        trait_window_draw(&screen, &settings);
        trait_settings_draw(&screen, &settings);
        if (!emit(out, "settings.png", settings.frame)) {
            return 1;
        }
        if (trait_panel_draw(whole()) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        if (!emit(out, "windows.png", whole())) {
            return 1;
        }
    }
    printf("proof: a %u-pixel panel over a %ux%u screen, %u tasks, a "
           "%u-column cpu graph and a clock that does not move when a "
           "window opens; a task manager of %u processes sorted by a "
           "column that really reorders and reverses; a notebook of %u "
           "pages where picking a tab changes the page\n",
           TRAIT_PANEL_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT,
           trait_panel_task_count(), TRAIT_PANEL_CPU_COLUMNS,
           trait_taskmgr_count(), trait_settings_page_count());
    return 0;
}
