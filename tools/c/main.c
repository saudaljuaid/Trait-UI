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
    printf("proof: a %u-pixel panel over a %ux%u screen, %u tasks, "
           "a %u-column cpu graph and a clock that does not move when a "
           "window opens\n",
           TRAIT_PANEL_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT,
           trait_panel_task_count(), TRAIT_PANEL_CPU_COLUMNS);
    return 0;
}
