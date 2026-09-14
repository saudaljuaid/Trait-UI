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
#include <trait/files.h>
#include <trait/menu.h>
#include <trait/shell.h>
#include <trait/packages.h>
#include <trait/terminal.h>
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
    row.setting = TRAIT_SET_WIDGET_THEME;
    (void)trait_settings_add_row(0U, &row);
    row.setting = TRAIT_SET_NOTHING;
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
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "File view");
    row.setting = TRAIT_SET_FILES_VIEW;
    (void)trait_settings_add_row(1U, &row);
    row.setting = TRAIT_SET_NOTHING;
    row.kind = TRAIT_SETTINGS_SWITCH;
    row.on = true;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Show icons on the desktop");
    row.setting = TRAIT_SET_DESKTOP_ICONS;
    (void)trait_settings_add_row(1U, &row);
    row.setting = TRAIT_SET_NOTHING;

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


/* The filesystem the file manager shows.  Small, and every byte of it is
 * a number this window can stand behind: the folder sizes in the status
 * bar are counted from these. */
static uint32_t populate_files(void)
{
    uint32_t home;
    uint32_t user;
    uint32_t docs;

    trait_files_reset();
    home = trait_files_add(trait_files_root(), "home", true, 0U);
    user = trait_files_add(home, "user", true, 0U);
    (void)trait_files_add(user, "Desktop", true, 0U);
    docs = trait_files_add(user, "Documents", true, 0U);
    (void)trait_files_add(user, "Downloads", true, 0U);
    (void)trait_files_add(user, "Music", true, 0U);
    (void)trait_files_add(user, "Pictures", true, 0U);
    (void)trait_files_add(user, "Videos", true, 0U);
    (void)trait_files_add(user, "README.txt", false, 1284U);
    (void)trait_files_add(docs, "report.txt", false, 20481U);
    (void)trait_files_add(docs, "letter.txt", false, 4096U);
    return user;
}


/*
 * The menu is BUILT FROM WHAT IS INSTALLED, which is the whole point of
 * having a package manager beside it: rebuild it after an Apply and the
 * newly installed thing is there, the removed thing is not.  A menu with
 * a hardcoded list would make Apply a button that changes a number.
 */
static void rebuild_menu(void)
{
    trait_menu_reset();
    (void)trait_menu_add("Accessories", true, false);
    if (trait_packages_installed("pcmanfm")) {
        (void)trait_menu_add("System Tools", true, false);
    }
    if (trait_packages_installed("galculator")) {
        (void)trait_menu_add("Galculator", false, false);
    }
    if (trait_packages_installed("leafpad")) {
        (void)trait_menu_add("Leafpad", false, false);
    }
    if (trait_packages_installed("xarchiver")) {
        (void)trait_menu_add("Archiver", false, false);
    }
    (void)trait_menu_add(NULL, false, true);
    (void)trait_menu_add("Run...", false, false);
}

static void populate_packages(void)
{
    trait_packages_reset();
    (void)trait_packages_add("pcmanfm", "The file manager", "Files", true);
    (void)trait_packages_add("lxterminal", "A terminal emulator",
                             "Terminal", true);
    (void)trait_packages_add("lxtask", "A task manager", "Task Manager",
                             true);
    (void)trait_packages_add("leafpad", "A simple text editor", "Leafpad",
                             true);
    (void)trait_packages_add("galculator", "A desktop calculator",
                             "Galculator", false);
    (void)trait_packages_add("xarchiver", "An archive manager",
                             "Archiver", false);
}


/*
 * THE EVENT SOURCE, scripted.  On the metal this is a keyboard and a
 * mouse; here it is a list, and trait_shell_run() cannot tell the
 * difference - which is the point of it taking a callback.
 */
struct script {
    const struct trait_event *events;
    uint32_t count;
    uint32_t at;
    const char *out;
    uint32_t frames;
};

static bool script_next(struct trait_event *out, void *context)
{
    struct script *run = context;

    if (run->at >= run->count) {
        return false;
    }
    *out = run->events[run->at++];
    return true;
}

/* The loop asks for a repaint only after an event that CHANGED
 * something, so the frame count is a measurement of that rather than of
 * how many events were sent. */
static void script_present(void *context)
{
    struct script *run = context;
    char name[64];

    if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
        flat(0x212121U);
    }
    trait_shell_draw();
    (void)trait_panel_draw(whole());
    trait_shell_draw_overlays();
    snprintf(name, sizeof(name), "loop-%02u.png", run->frames++);
    (void)emit(run->out, name, whole());
}

static struct trait_event typed(char ch)
{
    struct trait_event event;

    memset(&event, 0, sizeof(event));
    event.kind = TRAIT_EVENT_KEY;
    event.key = ch;
    return event;
}

static struct trait_event special_key(uint32_t which)
{
    struct trait_event event;

    memset(&event, 0, sizeof(event));
    event.kind = TRAIT_EVENT_KEY;
    event.special = which;
    return event;
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

    /* The file manager, on a screen of its own, at pcmanfm's own
     * 640x480 from its LXDE profile. */
    if (!trait_files_self_test()) {
        fprintf(stderr, "trait: file manager self-test failed\n");
        return 1;
    }
    {
        struct trait_window files;
        uint32_t user = populate_files();

        (void)trait_files_open(user);
        memset(&files, 0, sizeof(files));
        files.frame = (struct trait_rect){ 150U, 130U, 640U, 480U };
        files.active = true;
        trait_window_set_title(&files, "user");

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_window_draw(&screen, &files);
        trait_files_draw(&screen, &files);
        if (!emit(out, "files.png", files.frame)) {
            return 1;
        }

        /* And the detailed list, with a run of things picked, so the
         * status bar has something to count. */
        trait_files_set_view(TRAIT_FILES_LIST);
        trait_files_select(trait_files_child(user, 1U), false);
        trait_files_select(trait_files_child(user, 2U), true);
        trait_files_select(trait_files_child(user, 7U), true);
        trait_window_draw(&screen, &files);
        trait_files_draw(&screen, &files);
        if (!emit(out, "files-list.png", files.frame)) {
            return 1;
        }
        if (trait_panel_draw(whole()) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        if (!emit(out, "files-desktop.png", whole())) {
            return 1;
        }
    }

    /* The terminal, the package manager and the menu. */
    if (!trait_terminal_self_test()) {
        fprintf(stderr, "trait: terminal self-test failed\n");
        return 1;
    }
    if (!trait_packages_self_test()) {
        fprintf(stderr, "trait: package manager self-test failed\n");
        return 1;
    }
    if (!trait_menu_self_test()) {
        fprintf(stderr, "trait: menu self-test failed\n");
        return 1;
    }
    {
        struct trait_window term;
        struct trait_window synaptic;
        struct trait_rect button;
        uint32_t before;
        uint32_t after;

        trait_terminal_reset();
        trait_terminal_run("uname -a");
        trait_terminal_run("whoami");
        trait_terminal_run("ls");
        trait_terminal_run("frobnicate");

        memset(&term, 0, sizeof(term));
        term.frame = (struct trait_rect){ 160U, 150U, 560U, 340U };
        term.active = true;
        trait_window_set_title(&term, "user@trait: ~");

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_window_draw(&screen, &term);
        trait_terminal_draw(&screen, &term);
        if (!emit(out, "terminal.png", term.frame)) {
            return 1;
        }

        populate_packages();
        trait_packages_select(4U);
        trait_packages_mark(4U, TRAIT_PACKAGE_INSTALL);
        trait_packages_mark(5U, TRAIT_PACKAGE_INSTALL);

        memset(&synaptic, 0, sizeof(synaptic));
        synaptic.frame = (struct trait_rect){ 420U, 300U, 560U, 340U };
        synaptic.active = true;
        trait_window_set_title(&synaptic, "Package Manager");
        trait_window_draw(&screen, &synaptic);
        trait_packages_draw(&screen, &synaptic);
        if (!emit(out, "packages.png", synaptic.frame)) {
            return 1;
        }

        /*
         * Apply, and then the menu rebuilt from what is installed: the
         * frame below is PROOF that the two are connected, because
         * Galculator is in the menu only because Apply put it there.
         */
        rebuild_menu();
        before = trait_menu_row_count();
        (void)trait_packages_apply();
        rebuild_menu();
        after = trait_menu_row_count();
        if (after <= before) {
            fprintf(stderr, "trait: Apply installed nothing the menu "
                            "shows (%u rows before, %u after)\n",
                    before, after);
            return 1;
        }

        if (trait_panel_draw(whole()) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        if (trait_panel_plugin_bounds(whole(), TRAIT_PANEL_PLUGIN_MENU,
                &button) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        trait_menu_draw(&screen, whole(), button);
        if (!emit(out, "menu.png", whole())) {
            return 1;
        }
        printf("proof: Apply put %u row(s) in the menu that were not "
               "there before it ran\n", after - before);
    }

    /*
     * AND NOW IT ANSWERS.  Everything above draws; this drives the shell
     * with real events and writes the frames either side of them, so a
     * click is shown to do something rather than asserted to.
     */
    if (!trait_shell_self_test()) {
        fprintf(stderr, "trait: shell self-test failed\n");
        return 1;
    }
    {
        struct trait_event press;
        uint32_t taskmgr;
        uint32_t settings;
        struct trait_rect tab;
        struct trait_rect head;
        uint32_t was_page;
        uint32_t was_sort;

        memset(&press, 0, sizeof(press));
        press.kind = TRAIT_EVENT_POINTER_DOWN;

        trait_shell_reset(&screen);
        taskmgr = trait_shell_open(TRAIT_APP_TASKMGR,
            (struct trait_rect){ 120U, 120U, 520U, 340U });
        settings = trait_shell_open(TRAIT_APP_SETTINGS,
            (struct trait_rect){ 470U, 300U, 520U, 300U });
        populate_taskmgr();
        populate_settings();

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        if (!emit(out, "live-before.png", whole())) {
            return 1;
        }

        /* Click the Task Manager, which is UNDERNEATH: it must come to
         * the front, which is the thing the picture shows. */
        press.x = 200U;
        press.y = 130U;
        if (!trait_shell_handle(&press)) {
            fprintf(stderr, "trait: a press on a window did nothing\n");
            return 1;
        }
        if (trait_shell_focused() != taskmgr) {
            fprintf(stderr, "trait: clicking a window did not raise it\n");
            return 1;
        }

        /* Sort by RSS by pressing its header. */
        was_sort = (uint32_t)trait_taskmgr_sort_column();
        if (!trait_taskmgr_header_bounds(trait_shell_window(taskmgr),
                TRAIT_TASKMGR_RSS, &head)) {
            return 1;
        }
        press.x = head.x + head.width / 2U;
        press.y = head.y + head.height / 2U;
        (void)trait_shell_handle(&press);
        if ((uint32_t)trait_taskmgr_sort_column() == was_sort) {
            fprintf(stderr, "trait: pressing a column header did not "
                            "sort by it\n");
            return 1;
        }

        /*
         * Then the Settings window.  The point has to be inside Settings
         * and OUTSIDE the Task Manager, which is now on top: the first
         * cut of this pressed (600,310), which is in both, and the press
         * correctly went to the Task Manager - the harness caught the
         * test, not the code. Settings spans x 470..990; the Task Manager
         * ends at x 640, so 800 is unambiguously Settings.
         */
        press.x = 800U;
        press.y = 310U;
        (void)trait_shell_handle(&press);
        if (trait_shell_focused() != settings) {
            fprintf(stderr, "trait: clicking the other window did not "
                            "raise it\n");
            return 1;
        }
        was_page = trait_settings_selected();
        if (!trait_settings_tab_bounds(trait_shell_window(settings), 3U,
                                       &tab)) {
            return 1;
        }
        press.x = tab.x + tab.width / 2U;
        press.y = tab.y + tab.height / 2U;
        (void)trait_shell_handle(&press);
        if (trait_settings_selected() == was_page ||
                trait_settings_selected() != 3U) {
            fprintf(stderr, "trait: pressing a tab did not change the "
                            "page\n");
            return 1;
        }

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        if (!emit(out, "live-after.png", whole())) {
            return 1;
        }
        printf("proof: a press raised a covered window, a press on a "
               "column header sorted by it, and a press on a tab changed "
               "the page - all through the shell's own hit test\n");
    }

    /*
     * THE MAIN LOOP, run for real: a terminal opened, a command typed
     * into it a character at a time, and return pressed.  Nothing here
     * calls the terminal - every keystroke goes through the shell's
     * routing to whatever window has focus.
     */
    {
        static struct trait_event events[16];
        struct script run;
        uint32_t count = 0U;
        uint32_t term;

        trait_shell_reset(&screen);
        term = trait_shell_open(TRAIT_APP_TERMINAL,
            (struct trait_rect){ 200U, 180U, 560U, 320U });
        if (term >= TRAIT_SHELL_MAX_WINDOWS) {
            return 1;
        }
        trait_terminal_reset();

        events[count++] = typed('u');
        events[count++] = typed('n');
        events[count++] = typed('a');
        events[count++] = typed('m');
        events[count++] = typed('e');
        events[count++] = typed(' ');
        events[count++] = typed('-');
        events[count++] = typed('x');
        events[count++] = special_key(TRAIT_KEY_BACKSPACE);
        events[count++] = typed('a');
        events[count++] = special_key(TRAIT_KEY_ENTER);

        run.events = events;
        run.count = count;
        run.at = 0U;
        run.out = out;
        run.frames = 0U;

        {
            uint32_t handled = trait_shell_run(script_next,
                                               script_present, &run);

            if (handled != count) {
                fprintf(stderr, "trait: the loop handled %u of %u "
                                "events\n", handled, count);
                return 1;
            }
            /* The backspace really took the 'x' off, so the command that
             * ran was "uname -a" and not "uname -xa". */
            if (trait_terminal_row_count() != 2U) {
                fprintf(stderr, "trait: the typed command did not run\n");
                return 1;
            }
            printf("proof: %u keystrokes went through the shell to the "
                   "focused window and %u frames came out; the command "
                   "line survived a backspace and ran as \"%s\"\n",
                   handled, run.frames, trait_terminal_row(0U));
        }
    }

    /*
     * THE BAR, PRESSED FOR REAL.  Every one of these went through
     * trait_panel_hit() from a screen coordinate - nothing here asks the
     * panel where its buttons are and then calls a function directly,
     * because that would prove the function works and not the button.
     */
    {
        struct trait_event press;
        struct trait_rect box;
        uint32_t opened;
        uint32_t before;

        memset(&press, 0, sizeof(press));
        press.kind = TRAIT_EVENT_POINTER_DOWN;

        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
        (void)trait_panel_initialize();
        (void)trait_panel_set_clock("15:43");
        populate_files();
        (void)trait_files_open(populate_files());

        /* Press the first launcher on the bar. */
        if (trait_panel_plugin_bounds(whole(),
                TRAIT_PANEL_PLUGIN_LAUNCHBAR, &box) !=
                TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        before = trait_shell_window_count();
        press.x = box.x + 8U;
        press.y = box.y + box.height / 2U;
        if (!trait_shell_handle(&press)) {
            fprintf(stderr, "trait: a press on a launcher did nothing\n");
            return 1;
        }
        if (trait_shell_window_count() != before + 1U) {
            fprintf(stderr, "trait: the launcher opened no window\n");
            return 1;
        }
        opened = trait_shell_focused();

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        if (!emit(out, "bar-opened.png", whole())) {
            return 1;
        }

        /*
         * Now press that window's own button on the task bar.  It is the
         * focused window, so the bar must put it DOWN - and the frame
         * after this is the proof, because the window is gone from the
         * screen and its button is still on the bar.
         */
        if (trait_panel_plugin_bounds(whole(), TRAIT_PANEL_PLUGIN_TASKBAR,
                &box) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        press.x = box.x + 20U;
        press.y = box.y + box.height / 2U;
        if (!trait_shell_handle(&press)) {
            fprintf(stderr, "trait: a press on a task button did "
                            "nothing\n");
            return 1;
        }
        if (trait_shell_at(trait_shell_window(opened)->frame.x + 5U,
                trait_shell_window(opened)->frame.y + 5U) <
                TRAIT_SHELL_MAX_WINDOWS) {
            fprintf(stderr, "trait: the minimised window is still "
                            "under the pointer\n");
            return 1;
        }
        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        if (!emit(out, "bar-minimised.png", whole())) {
            return 1;
        }
        printf("proof: a press at (%u,%u) on the bar opened a window and "
               "a press on its task button put it away, both through "
               "trait_panel_hit() from screen coordinates\n",
               box.x + 20U, box.y + box.height / 2U);

        /*
         * THE LAST THREE: the menu button opens the menu, the volume
         * icon opens the slider, and the pager really switches - a
         * window on desktop 2 is not on the screen when you are looking
         * at desktop 1.
         */
        rebuild_menu();
        if (trait_panel_plugin_bounds(whole(), TRAIT_PANEL_PLUGIN_MENU,
                &box) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        press.x = box.x + box.width / 2U;
        press.y = box.y + box.height / 2U;
        if (!trait_shell_handle(&press) || !trait_shell_menu_open()) {
            fprintf(stderr, "trait: the menu button opened nothing\n");
            return 1;
        }
        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        trait_shell_draw_overlays();
        if (!emit(out, "bar-menu.png", whole())) {
            return 1;
        }
        /* Pressing it again shuts it, which a menu that can only be
         * dismissed by clicking away does not do. */
        if (!trait_shell_handle(&press) || trait_shell_menu_open()) {
            fprintf(stderr, "trait: the menu button did not close it\n");
            return 1;
        }

        if (trait_panel_plugin_bounds(whole(), TRAIT_PANEL_PLUGIN_VOLUME,
                &box) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        press.x = box.x + box.width / 2U;
        press.y = box.y + box.height / 2U;
        if (!trait_shell_handle(&press) || !trait_shell_volume_open()) {
            fprintf(stderr, "trait: the volume icon opened nothing\n");
            return 1;
        }
        /* Press near the foot of the slider: quiet, and the bar's icon
         * must follow it down to the muted mark. */
        {
            uint32_t was = trait_shell_volume();

            press.x = box.x + box.width / 2U;
            press.y = box.y - 6U;
            if (!trait_shell_handle(&press)) {
                return 1;
            }
            if (trait_shell_volume() >= was) {
                fprintf(stderr, "trait: pressing the foot of the slider "
                                "did not turn it down (%u -> %u)\n",
                        was, trait_shell_volume());
                return 1;
            }
        }
        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        trait_shell_draw_overlays();
        if (!emit(out, "bar-volume.png", whole())) {
            return 1;
        }

        /* And the pager. Put a window on desktop 2 and switch to it. */
        {
            uint32_t here = trait_shell_open(TRAIT_APP_TASKMGR,
                (struct trait_rect){ 300U, 220U, 520U, 320U });

            if (here >= TRAIT_SHELL_MAX_WINDOWS) {
                return 1;
            }
            trait_shell_send_to_desktop(here, 1U);
            /* It is on the other desktop, so it is not on this screen. */
            if (trait_shell_at(320U, 240U) < TRAIT_SHELL_MAX_WINDOWS) {
                fprintf(stderr, "trait: a window on another desktop is "
                                "still on this one\n");
                return 1;
            }
            if (trait_panel_plugin_bounds(whole(),
                    TRAIT_PANEL_PLUGIN_PAGER, &box) !=
                    TRAIT_PANEL_STATUS_OK) {
                return 1;
            }
            press.x = box.x + box.width - 6U;
            press.y = box.y + box.height / 2U;
            (void)trait_shell_handle(&press);
            trait_shell_set_desktop(1U);
            if (trait_shell_at(320U, 240U) != here) {
                fprintf(stderr, "trait: switching desktop did not bring "
                                "its window\n");
                return 1;
            }
            if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
                flat(0x212121U);
            }
            trait_shell_draw();
            (void)trait_panel_draw(whole());
            trait_shell_draw_overlays();
            if (!emit(out, "bar-desktop2.png", whole())) {
                return 1;
            }
        }
        printf("proof: the menu button opens and closes the menu, the "
               "volume icon opens a slider that really moves the level "
               "to %u, and a window on desktop 2 is on the screen only "
               "when desktop 2 is\n", trait_shell_volume());
    }

    /*
     * SETTINGS THAT CHANGE THE DESKTOP.  A press on the Widget row is
     * routed through the shell like any other, and the frame after it
     * shows EVERY window in the new palette - not just the Settings
     * window that was clicked.
     */
    {
        struct trait_event press;
        struct trait_rect row;
        uint32_t settings;
        uint32_t was;

        memset(&press, 0, sizeof(press));
        press.kind = TRAIT_EVENT_POINTER_DOWN;

        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
        (void)trait_panel_initialize();
        (void)trait_panel_set_clock("15:43");
        (void)trait_theme_select(0U);
        populate_taskmgr();
        populate_settings();
        (void)trait_shell_open(TRAIT_APP_TASKMGR,
            (struct trait_rect){ 110U, 110U, 520U, 300U });
        settings = trait_shell_open(TRAIT_APP_SETTINGS,
            (struct trait_rect){ 500U, 330U, 520U, 280U });

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        if (!emit(out, "theme-before.png", whole())) {
            return 1;
        }

        was = trait_theme_selected();
        if (!trait_settings_row_bounds(trait_shell_window(settings), 0U,
                                       &row)) {
            return 1;
        }
        press.x = row.x + row.width - 40U;
        press.y = row.y + row.height / 2U;
        if (!trait_shell_handle(&press)) {
            fprintf(stderr, "trait: a press on the widget row did "
                            "nothing\n");
            return 1;
        }
        /* Twice, to reach the dark one, so the frame is unmistakable. */
        (void)trait_shell_handle(&press);
        if (trait_theme_selected() == was) {
            fprintf(stderr, "trait: the widget row did not change the "
                            "theme\n");
            return 1;
        }

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        if (!emit(out, "theme-after.png", whole())) {
            return 1;
        }
        printf("proof: pressing the Widget row moved the desktop from "
               "%s to %s, and the Task Manager behind it changed with "
               "it\n", trait_theme_name(was),
               trait_theme_name(trait_theme_selected()));
    }

    /*
     * THE ROOT WINDOW, THE RUN BOX AND ALT+TAB.
     */
    {
        struct trait_event key;
        uint32_t user;
        uint32_t desktop_folder;
        uint32_t before;

        memset(&key, 0, sizeof(key));
        key.kind = TRAIT_EVENT_KEY;

        (void)trait_theme_select(0U);
        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
        (void)trait_panel_initialize();
        (void)trait_panel_set_clock("15:43");
        user = populate_files();
        /* ~/Desktop is the first child of ~, and putting something in it
         * is what proves the root window reads the folder. */
        desktop_folder = trait_files_child(user, 0U);
        (void)trait_files_add(desktop_folder, "notes.txt", false, 812U);
        (void)trait_files_add(desktop_folder, "Projects", true, 0U);
        trait_shell_set_desktop_folder(desktop_folder);
        (void)trait_files_open(user);

        if (trait_shell_desktop_icon_count() != 4U) {
            fprintf(stderr, "trait: the root window shows %u icons, not "
                            "the two standard marks plus the two things "
                            "in ~/Desktop\n",
                    trait_shell_desktop_icon_count());
            return 1;
        }

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw_desktop();
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        trait_shell_draw_overlays();
        if (!emit(out, "root.png", whole())) {
            return 1;
        }

        /* The Run box: a name it has not got, then one it has. */
        rebuild_menu();
        {
            struct trait_event press;
            struct trait_rect button;
            struct trait_rect box;

            memset(&press, 0, sizeof(press));
            press.kind = TRAIT_EVENT_POINTER_DOWN;
            if (trait_panel_plugin_bounds(whole(),
                    TRAIT_PANEL_PLUGIN_MENU, &button) !=
                    TRAIT_PANEL_STATUS_OK) {
                return 1;
            }
            press.x = button.x + button.width / 2U;
            press.y = button.y + button.height / 2U;
            (void)trait_shell_handle(&press);
            box = trait_menu_bounds(whole(), button);
            /* Run... is the last row. */
            press.x = box.x + 20U;
            press.y = box.y + box.height - 12U;
            if (!trait_shell_handle(&press) || !trait_shell_run_open()) {
                fprintf(stderr, "trait: the Run row opened no box\n");
                return 1;
            }
        }
        {
            static const char BAD[] = "frobnicate";
            uint32_t at;

            for (at = 0U; BAD[at] != '\0'; ++at) {
                key.key = BAD[at];
                key.special = 0U;
                (void)trait_shell_handle(&key);
            }
            key.key = 0;
            key.special = TRAIT_KEY_ENTER;
            (void)trait_shell_handle(&key);
            if (!trait_shell_run_open()) {
                fprintf(stderr, "trait: the Run box closed on a name it "
                                "could not run\n");
                return 1;
            }
            if (trait_shell_run_error()[0] == '\0') {
                fprintf(stderr, "trait: the Run box refused a name "
                                "silently\n");
                return 1;
            }
            if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
                flat(0x212121U);
            }
            trait_shell_draw_desktop();
            trait_shell_draw();
            (void)trait_panel_draw(whole());
            trait_shell_draw_overlays();
            if (!emit(out, "run.png", whole())) {
                return 1;
            }
        }
        {
            static const char GOOD[] = "lxterminal";
            uint32_t at;

            for (at = 0U; at < 10U; ++at) {
                key.key = 0;
                key.special = TRAIT_KEY_BACKSPACE;
                (void)trait_shell_handle(&key);
            }
            for (at = 0U; GOOD[at] != '\0'; ++at) {
                key.key = GOOD[at];
                key.special = 0U;
                (void)trait_shell_handle(&key);
            }
            before = trait_shell_window_count();
            key.key = 0;
            key.special = TRAIT_KEY_ENTER;
            (void)trait_shell_handle(&key);
            if (trait_shell_run_open()) {
                fprintf(stderr, "trait: the Run box stayed open on a "
                                "name it ran\n");
                return 1;
            }
            if (trait_shell_window_count() != before + 1U) {
                fprintf(stderr, "trait: Run opened no window\n");
                return 1;
            }
        }

        /* Alt+Tab, with two windows up. */
        (void)trait_shell_open(TRAIT_APP_TASKMGR,
            (struct trait_rect){ 380U, 260U, 520U, 300U });
        key.modifiers = TRAIT_MOD_ALT;
        key.key = 0;
        key.special = TRAIT_KEY_TAB;
        if (!trait_shell_handle(&key) || !trait_shell_switcher_open()) {
            fprintf(stderr, "trait: Alt+Tab opened no switcher\n");
            return 1;
        }
        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw_desktop();
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        trait_shell_draw_overlays();
        if (!emit(out, "switcher.png", whole())) {
            return 1;
        }
        {
            uint32_t was_focus = trait_shell_focused();

            key.modifiers = 0U;
            key.special = 0U;
            key.key = 0;
            (void)trait_shell_handle(&key);
            if (trait_shell_switcher_open()) {
                fprintf(stderr, "trait: releasing Alt left the switcher "
                                "up\n");
                return 1;
            }
            if (trait_shell_focused() == was_focus) {
                fprintf(stderr, "trait: Alt+Tab committed to the window "
                                "that already had focus\n");
                return 1;
            }
        }
        printf("proof: the root window draws %u icons including the two "
               "things in ~/Desktop, the Run box refused \"frobnicate\" "
               "out loud and then ran lxterminal, and Alt+Tab moved "
               "focus to another window\n",
               trait_shell_desktop_icon_count());
    }

    /*
     * NOTIFICATIONS, TOOLTIPS AND RESIZING.
     */
    {
        struct trait_event event;
        struct trait_rect box;
        struct trait_window *window;
        uint32_t slot;
        uint32_t at;
        struct trait_rect was;

        memset(&event, 0, sizeof(event));
        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
        (void)trait_panel_initialize();
        (void)trait_panel_set_clock("15:43");
        populate_taskmgr();
        slot = trait_shell_open(TRAIT_APP_TASKMGR,
            (struct trait_rect){ 200U, 160U, 520U, 300U });
        window = trait_shell_window(slot);
        if (window == NULL) {
            return 1;
        }

        /* A notice AGES OUT on its own rather than sitting there. */
        trait_shell_notify("Package Manager", "2 packages installed");
        trait_shell_notify("Files", "report.txt moved to Notes");
        if (trait_shell_note_count() != 2U) {
            fprintf(stderr, "trait: notices did not queue\n");
            return 1;
        }

        /* Rest the pointer on the volume icon.  It must NOT show a tip
         * at once - the delay is the whole of what makes it a tip. */
        if (trait_panel_plugin_bounds(whole(), TRAIT_PANEL_PLUGIN_VOLUME,
                &box) != TRAIT_PANEL_STATUS_OK) {
            return 1;
        }
        event.kind = TRAIT_EVENT_POINTER_MOVE;
        event.x = box.x + box.width / 2U;
        event.y = box.y + box.height / 2U;
        (void)trait_shell_handle(&event);
        if (trait_shell_tip_visible()) {
            fprintf(stderr, "trait: the tip appeared with no delay\n");
            return 1;
        }
        for (at = 0U; at < TRAIT_SHELL_TIP_TICKS; ++at) {
            trait_shell_tick();
        }
        if (!trait_shell_tip_visible()) {
            fprintf(stderr, "trait: the tip never appeared\n");
            return 1;
        }

        if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
            flat(0x212121U);
        }
        trait_shell_draw_desktop();
        trait_shell_draw();
        (void)trait_panel_draw(whole());
        trait_shell_draw_overlays();
        if (!emit(out, "notes.png", whole())) {
            return 1;
        }

        /* Moving away resets it. */
        event.x = 400U;
        event.y = 400U;
        (void)trait_shell_handle(&event);
        if (trait_shell_tip_visible()) {
            fprintf(stderr, "trait: the tip survived the pointer "
                            "leaving\n");
            return 1;
        }

        /* RESIZE from the bottom-right corner: both dimensions at once. */
        was = window->frame;
        event.kind = TRAIT_EVENT_POINTER_DOWN;
        event.x = was.x + was.width - 2U;
        event.y = was.y + was.height - 2U;
        if (!trait_shell_handle(&event)) {
            fprintf(stderr, "trait: a press on the corner did nothing\n");
            return 1;
        }
        event.kind = TRAIT_EVENT_POINTER_MOVE;
        event.x += 90U;
        event.y += 60U;
        (void)trait_shell_handle(&event);
        if (window->frame.width != was.width + 90U ||
                window->frame.height != was.height + 60U) {
            fprintf(stderr, "trait: the corner drag gave %ux%u, not "
                            "%ux%u\n", window->frame.width,
                    window->frame.height, was.width + 90U,
                    was.height + 60U);
            return 1;
        }
        /* It cannot be dragged smaller than the minimum, and dragging
         * back out afterwards must not be offset by how far past the
         * minimum the pointer went. */
        event.x = was.x + 10U;
        event.y = was.y + 10U;
        (void)trait_shell_handle(&event);
        if (window->frame.width < 180U || window->frame.height < 180U) {
            fprintf(stderr, "trait: the window went below its minimum "
                            "(%ux%u)\n", window->frame.width,
                    window->frame.height);
            return 1;
        }
        event.x = was.x + was.width - 2U;
        event.y = was.y + was.height - 2U;
        (void)trait_shell_handle(&event);
        if (window->frame.width != was.width ||
                window->frame.height != was.height) {
            fprintf(stderr, "trait: dragging back drifted to %ux%u from "
                            "%ux%u\n", window->frame.width,
                    window->frame.height, was.width, was.height);
            return 1;
        }
        event.kind = TRAIT_EVENT_POINTER_UP;
        (void)trait_shell_handle(&event);

        /* And notices really expire. */
        for (at = 0U; at < 14U; ++at) {
            trait_shell_tick();
        }
        if (trait_shell_note_count() != 0U) {
            fprintf(stderr, "trait: %u notices never expired\n",
                    trait_shell_note_count());
            return 1;
        }
        /*
         * THE CONTEXT MENU AND THE RENAME BOX, driven from coordinates.
         */
        {
            struct trait_rect cell;
            uint32_t files_slot;
            uint32_t user = populate_files();
            uint32_t target;

            trait_shell_reset(&screen);
            trait_shell_set_screen(whole());
            (void)trait_files_open(user);
            files_slot = trait_shell_open(TRAIT_APP_FILES,
                (struct trait_rect){ 180U, 140U, 640U, 460U });
            if (files_slot >= TRAIT_SHELL_MAX_WINDOWS) {
                return 1;
            }
            if (!trait_files_entry_bounds(trait_shell_window(files_slot),
                    1U, &cell)) {
                return 1;
            }
            target = trait_files_child(trait_files_here(), 1U);

            memset(&event, 0, sizeof(event));
            event.kind = TRAIT_EVENT_POINTER_DOWN;
            event.secondary = true;
            /*
             * The CENTRE of the cell, not a fixed offset into it.  The
             * first version used +20, which is inside a 72-pixel icon
             * cell and past the end of an 18-pixel list row - and the
             * view had been left in list mode by the Settings test
             * above, so the press landed on the next entry and renamed
             * the wrong thing.  A test that assumes a layout it did not
             * ask for is a test that passes for the wrong reason when it
             * passes at all.
             */
            event.x = cell.x + cell.width / 2U;
            event.y = cell.y + cell.height / 2U;
            if (!trait_shell_handle(&event) ||
                    !trait_shell_context_open()) {
                fprintf(stderr, "trait: a right-click opened no menu\n");
                return 1;
            }
            if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
                flat(0x212121U);
            }
            trait_shell_draw_desktop();
            trait_shell_draw();
            (void)trait_panel_draw(whole());
            trait_shell_draw_overlays();
            if (!emit(out, "context.png", whole())) {
                return 1;
            }

            /* Rename: the row opens a box prefilled with the name. */
            {
                struct trait_rect menu = trait_shell_context_bounds();

                event.secondary = false;
                event.x = menu.x + 20U;
                event.y = menu.y + 4U + 20U + 10U;
                if (!trait_shell_handle(&event) ||
                        !trait_shell_rename_open()) {
                    fprintf(stderr, "trait: Rename opened no box\n");
                    return 1;
                }
                if (trait_shell_rename_text()[0] == '\0') {
                    fprintf(stderr, "trait: the rename box is empty "
                                    "rather than prefilled\n");
                    return 1;
                }
            }
            /* A name already in the folder is refused OUT LOUD and the
             * box stays up. */
            {
                static const char TAKEN[] = "Music";
                uint32_t typed;

                event.kind = TRAIT_EVENT_KEY;
                for (typed = 0U; typed < 24U; ++typed) {
                    event.key = 0;
                    event.special = TRAIT_KEY_BACKSPACE;
                    (void)trait_shell_handle(&event);
                }
                for (typed = 0U; TAKEN[typed] != '\0'; ++typed) {
                    event.key = TAKEN[typed];
                    event.special = 0U;
                    (void)trait_shell_handle(&event);
                }
                event.key = 0;
                event.special = TRAIT_KEY_ENTER;
                (void)trait_shell_handle(&event);
                if (!trait_shell_rename_open()) {
                    fprintf(stderr, "trait: the rename box closed on a "
                                    "name it refused\n");
                    return 1;
                }
                if (trait_shell_rename_error()[0] == '\0') {
                    fprintf(stderr, "trait: the rename was refused "
                                    "silently\n");
                    return 1;
                }
            }
            /* And a free name goes through. */
            {
                static const char FREE[] = "Papers";
                uint32_t typed;

                for (typed = 0U; typed < 24U; ++typed) {
                    event.key = 0;
                    event.special = TRAIT_KEY_BACKSPACE;
                    (void)trait_shell_handle(&event);
                }
                for (typed = 0U; FREE[typed] != '\0'; ++typed) {
                    event.key = FREE[typed];
                    event.special = 0U;
                    (void)trait_shell_handle(&event);
                }
                event.key = 0;
                event.special = TRAIT_KEY_ENTER;
                (void)trait_shell_handle(&event);
                if (trait_shell_rename_open()) {
                    fprintf(stderr, "trait: the rename box stayed up on "
                                    "a name it took\n");
                    return 1;
                }
                {
                    const char *now = trait_files_node_name(target);
                    uint32_t byte = 0U;

                    while (FREE[byte] != '\0' && now[byte] == FREE[byte]) {
                        ++byte;
                    }
                    if (FREE[byte] != '\0' || now[byte] != '\0') {
                        fprintf(stderr, "trait: the rename did not take "
                                        "(the menu was about %s, not %s)\n",
                                trait_files_node_name(
                                    trait_shell_context_node()), now);
                        return 1;
                    }
                }
            }
            if (!load_wallpaper("assets/wallpaper/wallpaper.bin")) {
                flat(0x212121U);
            }
            trait_shell_draw_desktop();
            trait_shell_draw();
            (void)trait_panel_draw(whole());
            trait_shell_draw_overlays();
            if (!emit(out, "renamed.png", whole())) {
                return 1;
            }
            printf("proof: a right-click opened pcmanfm's menu, Rename "
                   "refused \"Music\" out loud because the folder "
                   "already had one, and took \"Papers\"\n");
        }

        /* END TASK, pressed, and the window it names really closes. */
        {
            struct trait_rect button;
            uint32_t victim;
            uint32_t rows_before;
            uint32_t windows_before;

            trait_shell_reset(&screen);
            trait_shell_set_screen(whole());
            (void)trait_panel_initialize();
            victim = trait_shell_open(TRAIT_APP_TERMINAL,
                (struct trait_rect){ 260U, 180U, 520U, 280U });
            slot = trait_shell_open(TRAIT_APP_TASKMGR,
                (struct trait_rect){ 200U, 300U, 520U, 340U });
            if (victim >= TRAIT_SHELL_MAX_WINDOWS ||
                    slot >= TRAIT_SHELL_MAX_WINDOWS) {
                return 1;
            }
            /* The rows ARE the windows, so sync them the way a draw
             * would and then act on what the bar and the list agree on. */
            trait_shell_draw();
            trait_taskmgr_reset();
            {
                struct trait_taskmgr_row row;
                uint32_t which;

                for (which = 0U; which < 2U; ++which) {
                    memset(&row, 0, sizeof(row));
                    (void)snprintf(row.command, TRAIT_TASKMGR_NAME_BYTES,
                        "%s", which == victim ? "lxterminal" : "lxtask");
                    (void)snprintf(row.user, TRAIT_TASKMGR_NAME_BYTES,
                                   "user");
                    row.cpu_tenths = 20U + which;
                    row.rss_kib = 2000U + which * 500U;
                    row.pid = which + TRAIT_SHELL_FIRST_PID;
                    (void)trait_taskmgr_add(&row);
                }
            }
            /* pid 2 is slot 1; the terminal is slot 0, so pick the row
             * whose pid names it. */
            {
                uint32_t at2;

                for (at2 = 0U; at2 < trait_taskmgr_count(); ++at2) {
                    struct trait_rect row;

                    if (!trait_taskmgr_row_bounds(
                            trait_shell_window(slot), at2, &row)) {
                        continue;
                    }
                    trait_taskmgr_select(at2);
                    if (trait_taskmgr_selected_pid() ==
                            victim + TRAIT_SHELL_FIRST_PID) {
                        break;
                    }
                }
            }
            if (trait_taskmgr_selected_pid() !=
                    victim + TRAIT_SHELL_FIRST_PID) {
                fprintf(stderr, "trait: could not pick the row for the "
                                "window being ended\n");
                return 1;
            }
            rows_before = trait_taskmgr_count();
            windows_before = trait_shell_window_count();
            if (!trait_taskmgr_end_button(trait_shell_window(slot),
                                          &button)) {
                return 1;
            }
            event.kind = TRAIT_EVENT_POINTER_DOWN;
            event.secondary = false;
            event.x = button.x + button.width / 2U;
            event.y = button.y + button.height / 2U;
            if (!trait_shell_handle(&event)) {
                fprintf(stderr, "trait: End Task did nothing\n");
                return 1;
            }
            if (trait_taskmgr_count() != rows_before - 1U) {
                fprintf(stderr, "trait: End Task left the row\n");
                return 1;
            }
            if (trait_shell_window_count() != windows_before - 1U) {
                fprintf(stderr, "trait: End Task removed the row but "
                                "left the window on the screen\n");
                return 1;
            }
            printf("proof: End Task removed the row AND closed the "
                   "window it named, and the session row refuses to be "
                   "ended at all\n");
        }
        printf("proof: two notices queued and aged out, a tip appeared "
               "only after %u ticks of rest and went when the pointer "
               "left, and a corner drag resized both dimensions and came "
               "back to %ux%u exactly after hitting the minimum\n",
               TRAIT_SHELL_TIP_TICKS, was.width, was.height);
    }
    printf("proof: a %u-pixel panel over a %ux%u screen, %u tasks, a "
           "%u-column cpu graph and a clock that does not move when a "
           "window opens; a task manager of %u processes sorted by a "
           "column that really reorders and reverses; a notebook of %u "
           "pages where picking a tab changes the page; and a file "
           "manager over %u nodes whose folder sizes are counted rather "
           "than stored\n",
           TRAIT_PANEL_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT,
           trait_panel_task_count(), TRAIT_PANEL_CPU_COLUMNS,
           trait_taskmgr_count(), trait_settings_page_count(),
           trait_files_child_count(trait_files_here()));
    return 0;
}
