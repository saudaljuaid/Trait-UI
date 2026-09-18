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
#include <trait/settings.h>
#include <trait/taskmgr.h>
#include <trait/theme.h>
#include <trait/window.h>
#include <trait/surface.h>

#include "png.h"
#include "trait_gears_art.h"
#include "trait_logo.h"

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
 * There is no wallpaper and no image to decode.  The session comes up on
 * glxgears, which trait_shell_draw_root() draws from geometry - so the
 * loader that used to read assets/wallpaper/wallpaper.bin, and the flat
 * fill that stood in when the dump was missing, both went with it.
 */

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

/*
 * THE GEARS ARE IN A WINDOW, AND THE CHECK HAS TO SAY SO.
 *
 * Two things are worth proving and neither is "it drew something".  The
 * first is that every pixel inside the window is one of seven colours -
 * black and the three face/side pairs - because the whole point of
 * drawing the gears from geometry rather than decoding a picture is
 * that no eighth colour can get in.  The second is that the big gear's
 * bore is a HOLE: a row through its middle has to leave the gear, cross
 * black, and come back.  A gear drawn as a solid disc passes every
 * colour test there is.
 */
static int gears_window_is_gears(struct trait_surface *surface,
    struct trait_rect where)
{
    uint32_t allowed[1U + TRAIT_GEARS_COUNT * 2U];
    uint32_t counts[1U + TRAIT_GEARS_COUNT * 2U];
    uint32_t total = 1U + TRAIT_GEARS_COUNT * 2U;
    const struct trait_gears_shape *big = &trait_gears[0];
    uint32_t top = where.y + where.height;
    uint32_t bottom = 0U;
    uint32_t runs = 0U;
    uint32_t gap = 0U;
    uint32_t bore = 0U;
    uint32_t inside = 0U;
    uint32_t row;
    uint32_t x;
    uint32_t y;
    uint32_t at;

    allowed[0] = 0x000000U;
    for (at = 0U; at < TRAIT_GEARS_COUNT; ++at) {
        allowed[1U + at * 2U] = trait_gears[at].face;
        allowed[2U + at * 2U] = trait_gears[at].side;
        if (trait_gears[at].outline_points > big->outline_points) {
            big = &trait_gears[at];
        }
    }
    for (at = 0U; at < total; ++at) {
        counts[at] = 0U;
    }
    for (y = where.y; y < where.y + where.height; ++y) {
        for (x = where.x; x < where.x + where.width; ++x) {
            uint32_t pixel = trait_surface_read(surface, x, y);
            uint32_t found = total;

            for (at = 0U; at < total; ++at) {
                if (allowed[at] == pixel) {
                    found = at;
                    break;
                }
            }
            if (found == total) {
                fprintf(stderr, "trait: the gears window has #%06X at "
                        "%ux%u, which is not one of its seven\n",
                        pixel, x, y);
                return 0;
            }
            ++counts[found];
            if (pixel == big->face || pixel == big->side) {
                if (y < top) {
                    top = y;
                }
                if (y > bottom) {
                    bottom = y;
                }
            }
        }
    }
    for (at = 1U; at < total; ++at) {
        if (counts[at] == 0U) {
            fprintf(stderr, "trait: the gears window never draws #%06X "
                    "- a gear or one of its sides is missing\n",
                    allowed[at]);
            return 0;
        }
    }
    if (bottom <= top) {
        fprintf(stderr, "trait: the largest gear is not in the window\n");
        return 0;
    }
    /* Across the middle of the widest gear: gear, black, gear. */
    row = top + (bottom - top) / 2U;
    for (x = where.x; x < where.x + where.width; ++x) {
        uint32_t pixel = trait_surface_read(surface, x, row);
        int solid = pixel == big->face || pixel == big->side;

        if (solid && inside == 0U) {
            ++runs;
            if (runs > 1U && gap > bore) {
                bore = gap;
            }
        }
        if (!solid && runs > 0U) {
            ++gap;
        }
        if (solid) {
            gap = 0U;
        }
        inside = solid ? 1U : 0U;
    }
    if (runs < 2U || bore < 8U) {
        fprintf(stderr, "trait: row %u crosses the gear %u time(s) with "
                "a %u-pixel gap - the bore is not a hole\n",
                row, runs, bore);
        return 0;
    }
    printf("proof: the glxgears window is %u gears in %u colours and "
           "nothing else, and a row through the widest one crosses it "
           "%u times either side of a %u-pixel bore\n",
           TRAIT_GEARS_COUNT, total, runs, bore);
    return 1;
}

static void populate(void)
{
    /* NOTHING TO POPULATE.  This used to load the bar: four task
     * buttons, a clock and thirty-six columns of cpu history.  With no
     * bar there is no list of open windows to keep in step with the
     * windows, which was the only thing here that could go stale. */
    return;
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
        { "glxgears", 19U, 3355U, 6U }
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
    (void)trait_settings_add_page("Keyboard");

    /*
     * WHAT CHANGED HERE.  Five of these rows used to be CHOICE boxes with
     * TRAIT_SET_NOTHING behind them - Font size, Icon theme, Position,
     * Height, Clock format.  They were drawn exactly like the theme
     * picker beside them, and pressing them did nothing at all, which is
     * the failure this header warns about in the largest possible form.
     *
     * Each is now one of two things.  If the shell can carry it out it
     * is a real control wired to the thing it names.  If it cannot, it
     * is a NOTE, which is not drawn as a control and says what it is
     * instead - the icon theme and the font are stated rather than
     * offered, because there is one of each and a picker over one
     * choice is a picker that does nothing.
     */

    /* ---- Widget: lxappearance ---- */
    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_CHOICE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "Widget theme");
    row.setting = TRAIT_SET_WIDGET_THEME;
    (void)trait_settings_add_row(0U, &row);

    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_NOTE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Icons: gentoo's, Johan Hanson 1998.");
    (void)trait_settings_add_row(0U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Font: Misc-Fixed, unpacked ahead.");
    (void)trait_settings_add_row(0U, &row);

    /* ---- Desktop: pcmanfm's Desktop Preferences ---- */
    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_CHOICE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES, "File view");
    row.setting = TRAIT_SET_FILES_VIEW;
    (void)trait_settings_add_row(1U, &row);

    row.kind = TRAIT_SETTINGS_SWITCH;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Show icons on the desktop");
    row.setting = TRAIT_SET_DESKTOP_ICONS;
    (void)trait_settings_add_row(1U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Show hidden files");
    row.setting = TRAIT_SET_SHOW_HIDDEN;
    (void)trait_settings_add_row(1U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "Open on a single click");
    row.setting = TRAIT_SET_SINGLE_CLICK;
    (void)trait_settings_add_row(1U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "See through the terminal");
    row.setting = TRAIT_SET_TERM_SHEER;
    (void)trait_settings_add_row(1U, &row);

    /* ---- Keyboard ---- */
    memset(&row, 0, sizeof(row));
    row.kind = TRAIT_SETTINGS_NOTE;
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "W-e   Open the file manager");
    (void)trait_settings_add_row(2U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "W-r   Open the Run box");
    (void)trait_settings_add_row(2U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "C-A-t Open a terminal");
    (void)trait_settings_add_row(2U, &row);
    (void)snprintf(row.label, TRAIT_SETTINGS_TEXT_BYTES,
                   "A-F4  Close the focused window");
    (void)trait_settings_add_row(2U, &row);
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

    trait_shell_draw_root();
    trait_shell_draw();
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

    populate();

    /* The shell has to be attached before it can draw: the wallpaper
     * this replaced was loaded straight into the surface and never went
     * through the shell at all. */
    trait_shell_reset(&screen);
    trait_shell_set_screen(whole());
    trait_shell_draw_root();
    if (!emit(out, "desktop.png", whole())) {
        return 1;
    }

    /* glxgears, in a window, which is where glxgears runs. */
    {
        uint32_t gears = trait_shell_open(TRAIT_APP_GEARS,
            (struct trait_rect){ 300U, 220U, 320U, 320U });

        if (gears >= TRAIT_SHELL_MAX_WINDOWS) {
            fprintf(stderr, "trait: glxgears opened no window\n");
            return 1;
        }
        trait_shell_draw();
        if (!gears_window_is_gears(&screen,
                trait_window_client(trait_shell_window(gears)))) {
            return 1;
        }
        if (!emit(out, "gears.png", whole())) {
            return 1;
        }
        (void)trait_shell_close(gears);
    }

    /*
     * A session: two windows on the weave, and the menu where the root
     * was pressed. With no panel there is nowhere else for the menu to
     * be, which is how fvwm and twm have always worked.
     */
    /* Files first, then the terminal ON TOP of it: the terminal is
     * see-through, and a see-through window over bare root only shows
     * you the root. */
    (void)trait_shell_open(TRAIT_APP_FILES,
        (struct trait_rect){ 300U, 120U, 620U, 400U });
    (void)trait_shell_open(TRAIT_APP_GEARS,
        (struct trait_rect){ 760U, 400U, 320U, 320U });
    (void)trait_shell_open(TRAIT_APP_TERMINAL,
        (struct trait_rect){ 140U, 300U, 560U, 320U });
    /*
     * twm's root menu: what you can start, and nothing else.  There is
     * no Restart or Exit on it, which twm has, because this shell does
     * not own an X session to restart or leave - a row that cannot do
     * what it says is the one thing this desktop does not draw.
     */
    trait_menu_reset();
    (void)trait_menu_add("xterm", false, false);
    (void)trait_menu_add("Files", false, false);
    (void)trait_menu_add("Packages", false, false);
    (void)trait_menu_add("glxgears", false, false);
    (void)trait_menu_add("Task Manager", false, false);
    (void)trait_menu_add("Settings", false, false);
    (void)trait_menu_add("", false, true);
    (void)trait_menu_add("Run...", false, false);

    trait_shell_draw_root();
    trait_shell_draw();
    /* Bare root, below and left of every window in this session. */
    if (!trait_shell_root_press(80U, 520U)) {
        fprintf(stderr, "trait: a press on bare root was not the root\n");
        return 1;
    }
    trait_shell_draw_overlays();
    if (!emit(out, "session.png", whole())) {
        return 1;
    }
    {
        struct trait_rect menu;

        if (!trait_shell_root_menu_bounds(&menu)) {
            fprintf(stderr, "trait: the root menu has no bounds\n");
            return 1;
        }
        if (menu.x < 80U || menu.y < 520U) {
            fprintf(stderr, "trait: the menu did not open where the "
                    "press was\n");
            return 1;
        }
        printf("proof: no panel and a root menu at %ux%u - where the "
               "press was, not where a button is\n", menu.x, menu.y);
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

        trait_shell_draw_root();
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
        trait_window_set_title(&term, "user@openrfs: ~");

        trait_shell_draw_root();
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

        /* The windows above are drawn straight onto the surface rather
         * than opened in the shell, but the shell still holds the ones
         * the session block opened - and a press that lands on one of
         * those is not a press on the root. */
        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
        trait_shell_draw_root();
        if (!trait_shell_root_press(320U, 520U)) {
            fprintf(stderr, "trait: the root refused the press\n");
            return 1;
        }
        trait_shell_draw_overlays();
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

        trait_shell_draw_root();
        trait_shell_draw();
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
        if (!trait_settings_tab_bounds(trait_shell_window(settings), 2U,
                                       &tab)) {
            return 1;
        }
        press.x = tab.x + tab.width / 2U;
        press.y = tab.y + tab.height / 2U;
        (void)trait_shell_handle(&press);
        if (trait_settings_selected() == was_page ||
                trait_settings_selected() != 2U) {
            fprintf(stderr, "trait: pressing a tab did not change the "
                            "page\n");
            return 1;
        }

        trait_shell_draw_root();
        trait_shell_draw();
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
            /* gfetch, which is the one command that prints a picture. */
        trait_terminal_run("gfetch");
        {
            uint32_t rows = trait_terminal_row_count();
            uint32_t seen = 0U;
            uint32_t at;

            for (at = 0U; at < rows; ++at) {
                const char *line = trait_terminal_row(at);

                if (strstr(line, "OS      OpenRFS") != NULL ||
                        strstr(line, "Misc-Fixed") != NULL) {
                    ++seen;
                }
            }
            if (seen != 2U) {
                fprintf(stderr, "trait: gfetch printed %u of its two "
                                "checked facts\n", seen);
                return 1;
            }
        }
        trait_shell_draw_root();
        trait_shell_draw();
        /*
         * AND THE FISH IS THE FISH'S COLOUR.  The face is a bitmap
         * font, so a lit pixel is the ink exactly rather than a blend
         * of it - which means the mark's own red has to be ON SCREEN,
         * not merely in a table.
         */
        {
            struct trait_rect client =
                trait_window_client(trait_shell_window(term));
            uint32_t body = 0U;
            uint32_t tongue = 0U;
            uint32_t x;
            uint32_t y;

            for (y = client.y; y < client.y + client.height; ++y) {
                for (x = client.x; x < client.x + client.width; ++x) {
                    uint32_t pixel = trait_surface_read(&screen, x, y);

                    if (pixel == TRAIT_LOGO_BODY) {
                        ++body;
                    } else if (pixel == TRAIT_LOGO_TONGUE) {
                        ++tongue;
                    }
                }
            }
            if (body == 0U || tongue == 0U) {
                fprintf(stderr, "trait: gfetch drew %u pixels of #%06X "
                                "and %u of #%06X - the mark is not in "
                                "its own colours\n",
                        body, TRAIT_LOGO_BODY, tongue,
                        TRAIT_LOGO_TONGUE);
                return 1;
            }
            printf("proof: the mark came out in the drawing's own reds - "
                   "%u pixels of #%06X and %u of #%06X, both averaged "
                   "out of the PNG rather than chosen\n",
                   body, TRAIT_LOGO_BODY, tongue, TRAIT_LOGO_TONGUE);
        }
        trait_shell_draw_root();
        trait_shell_draw();
        if (!emit(out, "gfetch.png", whole())) {
            return 1;
        }

        /*
         * AND YOU CAN SEE THROUGH IT.  There is no compositor, so the
         * only way this can be true is that the terminal read the
         * framebuffer back and mixed with it - which means the proof is
         * a pixel that is NEITHER the terminal's black NOR the root.
         */
        {
            struct trait_rect client =
                trait_window_client(trait_shell_window(term));
            uint32_t root;
            uint32_t sheer;
            uint32_t solid;
            uint32_t sample_x = client.x + client.width - 4U;
            uint32_t sample_y = client.y + client.height - 4U;

            root = trait_surface_read(&screen, 4U, 4U);
            sheer = trait_surface_read(&screen, sample_x, sample_y);
            if (sheer == 0x000000U || sheer == root) {
                fprintf(stderr, "trait: the terminal's ground is #%06X - "
                                "neither mixed with the root #%06X nor "
                                "anything else\n", sheer, root);
                return 1;
            }
            /* Off, and it is the terminal's black exactly. */
            trait_terminal_set_transparent(false);
            trait_shell_draw_root();
            trait_shell_draw();
            solid = trait_surface_read(&screen, sample_x, sample_y);
            if (solid != 0x000000U) {
                fprintf(stderr, "trait: an opaque terminal drew #%06X, "
                                "not black\n", solid);
                return 1;
            }
            trait_terminal_set_transparent(true);
            trait_shell_draw_root();
            trait_shell_draw();
            printf("proof: gfetch printed the mark and its facts, and the "
                   "terminal's ground over a #%06X root reads #%06X "
                   "rather than #000000 - it is mixed with what is "
                   "behind it, and the switch makes it black again\n",
                   root, sheer);
        }

        printf("proof: %u keystrokes went through the shell to the "
                   "focused window and %u frames came out; the command "
                   "line survived a backspace and ran as \"%s\"\n",
                   handled, run.frames, trait_terminal_row(0U));
        }
    }

    /*
     * THE ROOT MENU, PRESSED FOR REAL.  Every press here goes in as a
     * screen coordinate through trait_shell_handle() - nothing asks
     * where the menu is and then calls a function directly, because
     * that would prove the function works and not the menu.
     *
     * This replaces the bar's proofs.  There is no launcher to press,
     * no task button to put a window down with and no pager, because
     * there is no bar: the menu IS the way in, so it is the thing that
     * has to answer.
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
        populate_files();
        (void)trait_files_open(populate_files());
        trait_menu_reset();
        (void)trait_menu_add("xterm", false, false);
        (void)trait_menu_add("Files", false, false);
        (void)trait_menu_add("Packages", false, false);
        (void)trait_menu_add("glxgears", false, false);
        (void)trait_menu_add("Task Manager", false, false);
        (void)trait_menu_add("Settings", false, false);
        (void)trait_menu_add("", false, true);
        (void)trait_menu_add("Run...", false, false);

        trait_shell_draw_root();
        if (!trait_shell_root_press(300U, 470U)) {
            fprintf(stderr, "trait: the root refused a press\n");
            return 1;
        }
        if (!trait_shell_root_menu_bounds(&box)) {
            fprintf(stderr, "trait: the open menu has no bounds\n");
            return 1;
        }
        trait_shell_draw();
        trait_shell_draw_overlays();
        if (!emit(out, "root-menu.png", whole())) {
            return 1;
        }

        /* The second row is Files, so pressing it has to open Files -
         * and the menu has to be gone afterwards.  The rows start below
         * the title bar. */
        before = trait_shell_window_count();
        press.x = box.x + 20U;
        press.y = box.y + TRAIT_MENU_TITLE_HEIGHT + 4U + 20U + 10U;
        if (!trait_shell_handle(&press)) {
            fprintf(stderr, "trait: a press on a menu row did nothing\n");
            return 1;
        }
        if (trait_shell_window_count() != before + 1U) {
            fprintf(stderr, "trait: the menu row opened no window\n");
            return 1;
        }
        opened = trait_shell_focused();
        if (trait_shell_app_of(opened) != TRAIT_APP_FILES) {
            fprintf(stderr, "trait: the Files row opened something "
                            "else\n");
            return 1;
        }
        if (trait_shell_root_menu_open()) {
            fprintf(stderr, "trait: the menu stayed open after it was "
                            "used\n");
            return 1;
        }
        trait_shell_draw_root();
        trait_shell_draw();
        if (!emit(out, "root-opened.png", whole())) {
            return 1;
        }
        printf("proof: a press at (300,470) on bare root opened the menu "
               "at %ux%u and its Files row opened a Files window; the "
               "menu shut itself afterwards\n", box.x, box.y);
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
        (void)trait_theme_select(0U);
        populate_taskmgr();
        populate_settings();
        (void)trait_shell_open(TRAIT_APP_TASKMGR,
            (struct trait_rect){ 110U, 110U, 520U, 300U });
        settings = trait_shell_open(TRAIT_APP_SETTINGS,
            (struct trait_rect){ 500U, 330U, 520U, 280U });

        trait_shell_draw_root();
        trait_shell_draw();
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

        trait_shell_draw_root();
        trait_shell_draw();
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
        user = populate_files();
        /* ~/Desktop is the first child of ~, and putting something in it
         * is what proves the root window reads the folder. */
        desktop_folder = trait_files_child(user, 0U);
        (void)trait_files_add(desktop_folder, "notes.txt", false, 812U);
        (void)trait_files_add(desktop_folder, "Projects", true, 0U);
        trait_shell_set_desktop_folder(desktop_folder);
        (void)trait_files_open(user);
        /* The root comes up bare, so this turns them on rather than
         * assuming they are there. */
        trait_shell_set_desktop_icons(true);

        if (trait_shell_desktop_icon_count() != 3U) {
            fprintf(stderr, "trait: the root window shows %u icons, not "
                            "the home mark plus the two things in "
                            "~/Desktop\n",
                    trait_shell_desktop_icon_count());
            return 1;
        }

        trait_shell_draw_root();
        trait_shell_draw_desktop();
        trait_shell_draw_window_icons();
        trait_shell_draw();
        trait_shell_draw_overlays();
        if (!emit(out, "root.png", whole())) {
            return 1;
        }

        /* The Run box: a name it has not got, then one it has. */
        rebuild_menu();
        {
            struct trait_event press;
            struct trait_rect box;

            memset(&press, 0, sizeof(press));
            press.kind = TRAIT_EVENT_POINTER_DOWN;
            if (!trait_shell_root_press(300U, 470U) ||
                    !trait_shell_root_menu_bounds(&box)) {
                fprintf(stderr, "trait: the root menu did not open\n");
                return 1;
            }
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
            trait_shell_draw_root();
            trait_shell_draw_desktop();
            trait_shell_draw_window_icons();
            trait_shell_draw();
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

        /*
         * ICONIFY, AND THE WAY BACK.  With no taskbar, a minimised
         * window has to land somewhere you can see and press: fvwm puts
         * an icon on the root and so does this.  Pressing the window's
         * own minimise box puts it down, the icon appears at the foot of
         * the screen, and a press on the icon brings it back.
         */
        {
            uint32_t slot = trait_shell_focused();
            struct trait_event press;
            struct trait_rect box;
            struct trait_rect cell;

            memset(&press, 0, sizeof(press));
            press.kind = TRAIT_EVENT_POINTER_DOWN;
            if (slot >= TRAIT_SHELL_MAX_WINDOWS) {
                fprintf(stderr, "trait: nothing was focused to iconify\n");
                return 1;
            }
            if (!trait_window_button_bounds(trait_shell_window(slot),
                    TRAIT_WINDOW_MINIMISE, &box)) {
                fprintf(stderr, "trait: the minimise box has no bounds\n");
                return 1;
            }
            press.x = box.x + box.width / 2U;
            press.y = box.y + box.height / 2U;
            if (!trait_shell_handle(&press)) {
                fprintf(stderr, "trait: the minimise box did nothing\n");
                return 1;
            }
            if (trait_shell_window_icon_count() != 1U ||
                    trait_shell_window_icon_slot(0U) != slot) {
                fprintf(stderr, "trait: iconifying put no icon on the "
                                "root (%u there)\n",
                        trait_shell_window_icon_count());
                return 1;
            }
            if (!trait_shell_window_icon_bounds(0U, &cell)) {
                return 1;
            }
            trait_shell_draw_root();
            trait_shell_draw_window_icons();
            trait_shell_draw();
            if (!emit(out, "iconified.png", whole())) {
                return 1;
            }
            press.x = cell.x + cell.width / 2U;
            press.y = cell.y + cell.height / 2U;
            if (!trait_shell_root_press(press.x, press.y)) {
                fprintf(stderr, "trait: a press on the icon did "
                                "nothing\n");
                return 1;
            }
            if (trait_shell_window_icon_count() != 0U ||
                    trait_shell_focused() != slot) {
                fprintf(stderr, "trait: the icon did not put the window "
                                "back\n");
                return 1;
            }
            /* And it is not the MENU that opened: an icon press is not
             * a root press, even though the icon is on the root. */
            if (trait_shell_root_menu_open()) {
                fprintf(stderr, "trait: the icon press opened the menu "
                                "over it\n");
                return 1;
            }
        }

        /*
         * THE THREE SHORTCUTS THE KEYBOARD PAGE CLAIMS.  They were
         * three lines of text and nothing behind them until now, so
         * this asks each one to do the thing its line says.
         */
        {
            uint32_t had = trait_shell_window_count();

            key.modifiers = TRAIT_MOD_SUPER;
            key.special = 0U;
            key.key = 'e';
            if (!trait_shell_handle(&key) ||
                    trait_shell_window_count() != had + 1U ||
                    trait_shell_app_of(trait_shell_focused()) !=
                        TRAIT_APP_FILES) {
                fprintf(stderr, "trait: W-e opened no file manager\n");
                return 1;
            }
            key.key = 'r';
            if (!trait_shell_handle(&key) || !trait_shell_run_open()) {
                fprintf(stderr, "trait: W-r opened no Run box\n");
                return 1;
            }
            key.modifiers = 0U;
            key.special = TRAIT_KEY_ESCAPE;
            key.key = 0;
            (void)trait_shell_handle(&key);

            had = trait_shell_window_count();
            key.modifiers = TRAIT_MOD_CTRL | TRAIT_MOD_ALT;
            key.special = 0U;
            key.key = 't';
            if (!trait_shell_handle(&key) ||
                    trait_shell_window_count() != had + 1U ||
                    trait_shell_app_of(trait_shell_focused()) !=
                        TRAIT_APP_TERMINAL) {
                fprintf(stderr, "trait: C-A-t opened no terminal\n");
                return 1;
            }
            /* And Alt+F4 shuts the one it just opened. */
            key.modifiers = TRAIT_MOD_ALT;
            key.special = TRAIT_KEY_F4;
            key.key = 0;
            if (!trait_shell_handle(&key) ||
                    trait_shell_window_count() != had) {
                fprintf(stderr, "trait: A-F4 closed nothing\n");
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
        trait_shell_draw_root();
        trait_shell_draw_desktop();
        trait_shell_draw_window_icons();
        trait_shell_draw();
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
     * SETTINGS: EVERY CONTROL ON THE WINDOW DOES THE THING IT NAMES.
     *
     * Two of these switches used to flip a bool inside the row and stop
     * there, which made the Settings window the largest possible version
     * of a control that does not do what it is drawn as.  This presses
     * each one through trait_settings_press() - the same call the shell
     * makes when you click it - and then asks the thing it claims to
     * control, not the row, whether it changed.
     */
    {
        uint32_t user;
        uint32_t before;
        uint32_t after;
        uint32_t docs;

        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
        user = populate_files();
        (void)trait_files_add(user, ".bashrc", false, 3771U);
        (void)trait_files_open(user);
        trait_shell_set_desktop_folder(trait_files_child(user, 0U));
        populate_settings();

        /* Show icons on the desktop - page 1, row 1.  The root comes
         * up bare now, so the switch is asked to FILL it and then to
         * clear it again, which is the same switch either way. */
        if (trait_shell_desktop_icon_count() != 0U) {
            fprintf(stderr, "trait: the root came up with icons on it\n");
            return 1;
        }
        if (!trait_settings_press(1U, 1U)) {
            fprintf(stderr, "trait: the desktop-icons switch refused the "
                            "press\n");
            return 1;
        }
        before = trait_shell_desktop_icon_count();
        if (before == 0U) {
            fprintf(stderr, "trait: the desktop-icons switch drew "
                            "nothing on the root\n");
            return 1;
        }
        if (!trait_settings_press(1U, 1U) ||
                trait_shell_desktop_icon_count() != 0U) {
            fprintf(stderr, "trait: the desktop-icons switch did not "
                            "clear the root again (%u icons)\n", before);
            return 1;
        }
        (void)trait_settings_press(1U, 1U);

        /* Show hidden files - page 1, row 2.  ~/.bashrc is the one. */
        before = trait_files_visible_count(user);
        if (before != trait_files_child_count(user) - 1U) {
            fprintf(stderr, "trait: a dot file was visible with hidden "
                            "files off (%u of %u shown)\n",
                    before, trait_files_child_count(user));
            return 1;
        }
        if (!trait_settings_press(1U, 2U) ||
                trait_files_visible_count(user) !=
                    trait_files_child_count(user)) {
            fprintf(stderr, "trait: the hidden-files switch did not show "
                            "the dot file\n");
            return 1;
        }
        after = trait_files_visible_count(user);
        (void)trait_settings_press(1U, 2U);

        /* Open on a single click - page 1, row 3.  With it off, one press
         * on a folder selects; with it on, the same press opens. */
        docs = trait_files_child(user, 3U);
        {
            struct trait_event press;
            struct trait_rect cell;
            struct trait_window *window;
            uint32_t slot;
            uint32_t at;
            uint32_t index = TRAIT_FILES_MAX_NODES;

            struct trait_rect where;

            where.x = 60U;
            where.y = 60U;
            where.width = 640U;
            where.height = 480U;
            slot = trait_shell_open(TRAIT_APP_FILES, where);
            window = trait_shell_window(slot);
            for (at = 0U; at < trait_files_visible_count(user); ++at) {
                if (trait_files_visible_child(user, at) == docs) {
                    index = at;
                }
            }
            if (index == TRAIT_FILES_MAX_NODES ||
                    !trait_files_entry_bounds(window, index, &cell)) {
                fprintf(stderr, "trait: could not find Documents in the "
                                "view\n");
                return 1;
            }
            memset(&press, 0, sizeof(press));
            press.kind = TRAIT_EVENT_POINTER_DOWN;
            press.x = cell.x + cell.width / 2U;
            press.y = cell.y + cell.height / 2U;
            (void)trait_shell_handle(&press);
            if (trait_files_here() == docs) {
                fprintf(stderr, "trait: one click opened a folder with "
                                "single click off\n");
                return 1;
            }
            if (!trait_settings_press(1U, 3U)) {
                fprintf(stderr, "trait: the single-click switch refused "
                                "the press\n");
                return 1;
            }
            (void)trait_shell_handle(&press);
            if (trait_files_here() != docs) {
                fprintf(stderr, "trait: single click was on and one click "
                                "still did not open the folder\n");
                return 1;
            }
            (void)trait_settings_press(1U, 3U);
            (void)trait_files_open(user);
        }

        printf("proof: every switch on the Settings window moves the "
               "thing it names - the root went from bare to %u icons and "
               "back, a dot file appeared (%u of %u), and one click "
               "opened a folder only once single click was on\n",
               before, after, trait_files_child_count(user));
    }

    /*
     * NOTIFICATIONS AND RESIZING.
     */
    {
        struct trait_event event;
        struct trait_window *window;
        uint32_t slot;
        uint32_t at;
        struct trait_rect was;

        memset(&event, 0, sizeof(event));
        trait_shell_reset(&screen);
        trait_shell_set_screen(whole());
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

        trait_shell_draw_root();
        trait_shell_draw_desktop();
        trait_shell_draw_window_icons();
        trait_shell_draw();
        trait_shell_draw_overlays();
        if (!emit(out, "notes.png", whole())) {
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
            trait_shell_draw_root();
            trait_shell_draw_desktop();
            trait_shell_draw_window_icons();
            trait_shell_draw();
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
            trait_shell_draw_root();
            trait_shell_draw_desktop();
            trait_shell_draw_window_icons();
            trait_shell_draw();
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

        /* THE CLIPBOARD, through the shell's own keys. */
        {
            uint32_t user = populate_files();
            uint32_t files_slot;
            uint32_t docs;
            uint32_t before;

            trait_shell_reset(&screen);
            trait_shell_set_screen(whole());
            (void)trait_files_open(user);
            files_slot = trait_shell_open(TRAIT_APP_FILES,
                (struct trait_rect){ 200U, 150U, 640U, 460U });
            if (files_slot >= TRAIT_SHELL_MAX_WINDOWS) {
                return 1;
            }
            docs = trait_files_child(user, 1U);
            trait_files_select(trait_files_child(user, 6U), false);

            event.kind = TRAIT_EVENT_KEY;
            event.modifiers = TRAIT_MOD_CTRL;
            event.special = 0U;
            event.key = 'c';
            if (!trait_shell_handle(&event)) {
                fprintf(stderr, "trait: Ctrl+C copied nothing\n");
                return 1;
            }
            before = trait_files_child_count(docs);
            (void)trait_files_open(docs);
            event.key = 'v';
            if (!trait_shell_handle(&event)) {
                fprintf(stderr, "trait: Ctrl+V pasted nothing\n");
                return 1;
            }
            if (trait_files_child_count(docs) != before + 1U) {
                fprintf(stderr, "trait: the paste did not arrive\n");
                return 1;
            }
            /* A copy is not spent, so a second paste lands too - and it
             * cannot overwrite the first. */
            event.key = 'v';
            (void)trait_shell_handle(&event);
            if (trait_files_child_count(docs) != before + 2U) {
                fprintf(stderr, "trait: the second paste overwrote the "
                                "first\n");
                return 1;
            }
            event.modifiers = 0U;
            printf("proof: Ctrl+C then Ctrl+V put README.txt in "
                   "Documents and a second Ctrl+V put \"%s\" beside "
                   "it rather than over it\n",
                   trait_files_node_name(
                       trait_files_child(docs, before + 1U)));
        }
        printf("proof: two notices queued and aged out, and a corner "
               "drag resized both dimensions and came back to %ux%u "
               "exactly after hitting the minimum\n",
               was.width, was.height);
    }
    printf("proof: a %ux%u screen with nothing on it but the root and "
           "what you start from its menu - no bar, no dock, no tray; a "
           "task manager of %u processes sorted by a column that really "
           "reorders and reverses; a notebook of %u pages where picking "
           "a tab changes the page; and a file manager over %u nodes "
           "whose folder sizes are counted rather than stored\n",
           SCREEN_WIDTH, SCREEN_HEIGHT,
           trait_taskmgr_count(), trait_settings_page_count(),
           trait_files_child_count(trait_files_here()));
    return 0;
}
