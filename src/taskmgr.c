/* SPDX-License-Identifier: GPL-3.0-only */
#include <trait/taskmgr.h>

#include <trait/font.h>
#include <trait/theme.h>

/* ================================================================ METRICS */

#define TASKMGR_MENUBAR 20U
#define TASKMGR_SUMMARY 20U
#define TASKMGR_HEADER 18U
#define TASKMGR_ROW 17U
#define TASKMGR_PAD 6U
#define TASKMGR_FOOTER 32U

/*
 * The columns.  Command takes what is left, which is right: it is the one
 * that varies in length and the one worth reading.  The rest are sized to
 * their longest plausible value and RIGHT ALIGNED, because a column of
 * numbers that is not right aligned cannot be scanned.
 */
static const uint32_t COLUMN_WIDTH[TRAIT_TASKMGR_COLUMNS] = {
    0U, 70U, 60U, 76U, 50U
};

static const char *const COLUMN_NAME[TRAIT_TASKMGR_COLUMNS] = {
    "Command", "User", "CPU%", "RSS", "PID"
};

/* ================================================================== STATE */

static struct trait_taskmgr_row rows[TRAIT_TASKMGR_MAX_ROWS];
static uint32_t row_count;
static enum trait_taskmgr_column sort_column = TRAIT_TASKMGR_PID;
static bool sort_descending;

/* ================================================================ HELPERS */

static void copy(char *out, const char *text, uint32_t capacity)
{
    uint32_t at = 0U;

    while (text != NULL && text[at] != '\0' && at + 1U < capacity) {
        out[at] = text[at];
        ++at;
    }
    out[at] = '\0';
}

static int compare_text(const char *a, const char *b)
{
    uint32_t at = 0U;

    while (a[at] != '\0' && b[at] != '\0') {
        if (a[at] != b[at]) {
            return a[at] < b[at] ? -1 : 1;
        }
        ++at;
    }
    if (a[at] == b[at]) {
        return 0;
    }
    return a[at] == '\0' ? -1 : 1;
}

static int compare(const struct trait_taskmgr_row *a,
    const struct trait_taskmgr_row *b)
{
    switch (sort_column) {
    case TRAIT_TASKMGR_COMMAND:
        return compare_text(a->command, b->command);
    case TRAIT_TASKMGR_USER:
        return compare_text(a->user, b->user);
    case TRAIT_TASKMGR_CPU:
        if (a->cpu_tenths == b->cpu_tenths) {
            return 0;
        }
        return a->cpu_tenths < b->cpu_tenths ? -1 : 1;
    case TRAIT_TASKMGR_RSS:
        if (a->rss_kib == b->rss_kib) {
            return 0;
        }
        return a->rss_kib < b->rss_kib ? -1 : 1;
    case TRAIT_TASKMGR_PID:
    default:
        if (a->pid == b->pid) {
            return 0;
        }
        return a->pid < b->pid ? -1 : 1;
    }
}

/* Insertion sort: the list is a couple of dozen rows and it is STABLE,
 * which matters - sorting by User must leave equal users in the order
 * they were in rather than shuffling them every redraw. */
static void resort(void)
{
    uint32_t at;

    for (at = 1U; at < row_count; ++at) {
        struct trait_taskmgr_row held = rows[at];
        uint32_t back = at;

        while (back > 0U) {
            int order = compare(&rows[back - 1U], &held);

            if (sort_descending) {
                order = -order;
            }
            if (order <= 0) {
                break;
            }
            rows[back] = rows[back - 1U];
            --back;
        }
        rows[back] = held;
    }
}

/* Digits, without libc.  `tenths` prints 42 as "4.2". */
static uint32_t number(char *out, uint32_t value, uint32_t capacity)
{
    char digits[12];
    uint32_t length = 0U;
    uint32_t at = 0U;

    if (value == 0U) {
        digits[length++] = '0';
    }
    while (value != 0U && length < sizeof(digits)) {
        digits[length++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (length != 0U && at + 1U < capacity) {
        out[at++] = digits[--length];
    }
    out[at] = '\0';
    return at;
}

static void tenths(char *out, uint32_t value, uint32_t capacity)
{
    uint32_t at = number(out, value / 10U, capacity);

    if (at + 2U < capacity) {
        out[at++] = '.';
        out[at++] = (char)('0' + (value % 10U));
        out[at] = '\0';
    }
}

/* KiB as the unit the value is actually in, which is what lxtask does
 * rather than inventing a precision it has not got. */
static void memory(char *out, uint32_t kib, uint32_t capacity)
{
    if (kib >= 1024U) {
        uint32_t mib = kib * 10U / 1024U;
        uint32_t at = number(out, mib / 10U, capacity);

        if (at + 6U < capacity) {
            out[at++] = '.';
            out[at++] = (char)('0' + (mib % 10U));
            out[at++] = ' ';
            out[at++] = 'M';
            out[at++] = 'i';
            out[at++] = 'B';
            out[at] = '\0';
        }
        return;
    }
    {
        uint32_t at = number(out, kib, capacity);

        if (at + 4U < capacity) {
            out[at++] = ' ';
            out[at++] = 'K';
            out[at++] = 'i';
            out[at++] = 'B';
            out[at] = '\0';
        }
    }
}

/* ================================================================== API */

void trait_taskmgr_reset(void)
{
    row_count = 0U;
}

bool trait_taskmgr_add(const struct trait_taskmgr_row *row)
{
    if (row == NULL || row_count >= TRAIT_TASKMGR_MAX_ROWS) {
        return false;
    }
    rows[row_count] = *row;
    copy(rows[row_count].command, row->command,
         TRAIT_TASKMGR_NAME_BYTES);
    copy(rows[row_count].user, row->user, TRAIT_TASKMGR_NAME_BYTES);
    ++row_count;
    resort();
    return true;
}

uint32_t trait_taskmgr_count(void)
{
    return row_count;
}

void trait_taskmgr_sort(enum trait_taskmgr_column column)
{
    if (column == sort_column) {
        sort_descending = !sort_descending;
    } else {
        sort_column = column;
        sort_descending = false;
    }
    resort();
}

enum trait_taskmgr_column trait_taskmgr_sort_column(void)
{
    return sort_column;
}

bool trait_taskmgr_sort_descending(void)
{
    return sort_descending;
}

/* ================================================================ LAYOUT */

static uint32_t command_width(struct trait_rect client)
{
    uint32_t fixed = 0U;
    uint32_t at;

    for (at = 1U; at < TRAIT_TASKMGR_COLUMNS; ++at) {
        fixed += COLUMN_WIDTH[at];
    }
    if (client.width < fixed + TASKMGR_PAD * 2U + 60U) {
        return 60U;
    }
    return client.width - fixed - TASKMGR_PAD * 2U;
}

bool trait_taskmgr_header_bounds(const struct trait_window *window,
    enum trait_taskmgr_column column, struct trait_rect *out)
{
    struct trait_rect client;
    uint32_t left;
    uint32_t at;

    if (window == NULL || out == NULL ||
            (uint32_t)column >= TRAIT_TASKMGR_COLUMNS) {
        return false;
    }
    client = trait_window_client(window);
    left = client.x + TASKMGR_PAD;
    for (at = 0U; at < (uint32_t)column; ++at) {
        left += at == 0U ? command_width(client) : COLUMN_WIDTH[at];
    }
    out->x = left;
    out->y = client.y + TASKMGR_MENUBAR + TASKMGR_SUMMARY;
    out->width = column == TRAIT_TASKMGR_COMMAND ?
        command_width(client) : COLUMN_WIDTH[column];
    out->height = TASKMGR_HEADER;
    return true;
}

/* ================================================================ DRAWING */

static void draw_cell(struct trait_surface *surface, struct trait_rect clip,
    struct trait_rect box, const char *text, uint32_t baseline,
    uint32_t ink, bool right_aligned)
{
    uint32_t width = trait_font_width(text);
    uint32_t x = box.x + 4U;

    if (right_aligned && box.width > width + 8U) {
        x = box.x + box.width - width - 6U;
    }
    trait_font_draw(surface, clip, x, baseline, text, ink);
}

void trait_taskmgr_draw(struct trait_surface *surface,
    const struct trait_window *window)
{
    struct trait_rect client;
    struct trait_rect strip;
    char scratch[24];
    uint32_t at;
    uint32_t column;
    uint32_t list_top;

    if (window == NULL || !trait_surface_valid(surface)) {
        return;
    }
    client = trait_window_client(window);
    trait_surface_fill(surface, client, client, TRAIT_BG);

    /* the menu bar */
    strip = client;
    strip.height = TASKMGR_MENUBAR;
    trait_surface_fill(surface, client, strip, TRAIT_BG);
    {
        static const char *const MENUS[3] = { "File", "View", "Help" };
        uint32_t pen = client.x + TASKMGR_PAD;

        for (at = 0U; at < 3U; ++at) {
            trait_font_draw(surface, client, pen,
                client.y + 14U, MENUS[at], TRAIT_FG);
            pen += trait_font_width(MENUS[at]) + 14U;
        }
    }
    for (at = 0U; at < client.width; ++at) {
        trait_surface_plot(surface, client, client.x + at,
            client.y + TASKMGR_MENUBAR - 1U, TRAIT_LINE);
    }

    /* the summary line */
    {
        uint32_t pen = client.x + TASKMGR_PAD;
        uint32_t base = client.y + TASKMGR_MENUBAR + 14U;

        trait_font_draw(surface, client, pen, base, "Tasks:", TRAIT_TEXT);
        pen += trait_font_width("Tasks:") + 5U;
        (void)number(scratch, row_count, sizeof(scratch));
        trait_font_draw(surface, client, pen, base, scratch, TRAIT_TEXT);
    }

    /* the column header, with the sorted one marked */
    for (column = 0U; column < TRAIT_TASKMGR_COLUMNS; ++column) {
        struct trait_rect head;

        if (!trait_taskmgr_header_bounds(window,
                (enum trait_taskmgr_column)column, &head)) {
            continue;
        }
        trait_surface_fill(surface, client, head, TRAIT_BG_ACTIVE);
        for (at = 0U; at < head.height; ++at) {
            trait_surface_plot(surface, client,
                head.x + head.width - 1U, head.y + at, TRAIT_LINE);
        }
        for (at = 0U; at < head.width; ++at) {
            trait_surface_plot(surface, client, head.x + at,
                head.y + head.height - 1U, TRAIT_LINE);
        }
        trait_font_draw(surface, head, head.x + 4U, head.y + 13U,
            COLUMN_NAME[column], TRAIT_FG);
        if ((enum trait_taskmgr_column)column == sort_column &&
                head.width > 20U) {
            /* The sort marker: a triangle, pointing the way the list
             * runs.  Drawn rather than carried as a picture, because at
             * five pixels a picture is five lines with a file round it. */
            uint32_t tip = head.x + head.width - 10U;
            uint32_t mid = head.y + head.height / 2U;

            /*
             * ASCENDING POINTS UP, descending points down, which is the
             * way round every list with a sortable header does it.  The
             * first cut had it inverted - a list running largest-first
             * under an upward arrow - which is worse than no marker,
             * because a marker that is wrong is still believed.
             */
            for (at = 0U; at < 4U; ++at) {
                uint32_t span;

                for (span = 0U; span <= at; ++span) {
                    uint32_t row = sort_descending ?
                        mid + 2U - at : mid - 2U + at;

                    trait_surface_plot(surface, head, tip + span, row,
                                       TRAIT_FG);
                    trait_surface_plot(surface, head, tip - span, row,
                                       TRAIT_FG);
                }
            }
        }
    }

    /* the rows */
    list_top = client.y + TASKMGR_MENUBAR + TASKMGR_SUMMARY +
        TASKMGR_HEADER;
    {
        struct trait_rect list;

        list.x = client.x;
        list.y = list_top;
        list.width = client.width;
        list.height = client.height > (list_top - client.y) +
            TASKMGR_FOOTER ?
            client.height - (list_top - client.y) - TASKMGR_FOOTER : 0U;
        trait_surface_fill(surface, client, list, TRAIT_BASE);

        for (at = 0U; at < row_count; ++at) {
            uint32_t top = list_top + at * TASKMGR_ROW;
            uint32_t baseline = top + 12U;

            if (top + TASKMGR_ROW > list.y + list.height) {
                break;
            }
            if ((at & 1U) != 0U) {
                struct trait_rect band;

                band.x = list.x;
                band.y = top;
                band.width = list.width;
                band.height = TASKMGR_ROW;
                trait_surface_fill(surface, list, band,
                                   TRAIT_BASE_PRELIGHT);
            }
            for (column = 0U; column < TRAIT_TASKMGR_COLUMNS; ++column) {
                struct trait_rect head;
                struct trait_rect cell;

                if (!trait_taskmgr_header_bounds(window,
                        (enum trait_taskmgr_column)column, &head)) {
                    continue;
                }
                cell = head;
                cell.y = top;
                cell.height = TASKMGR_ROW;
                switch (column) {
                case TRAIT_TASKMGR_COMMAND:
                    draw_cell(surface, list, cell, rows[at].command,
                              baseline, TRAIT_TEXT, false);
                    break;
                case TRAIT_TASKMGR_USER:
                    draw_cell(surface, list, cell, rows[at].user,
                              baseline, TRAIT_TEXT, false);
                    break;
                case TRAIT_TASKMGR_CPU:
                    tenths(scratch, rows[at].cpu_tenths, sizeof(scratch));
                    draw_cell(surface, list, cell, scratch, baseline,
                              TRAIT_TEXT, true);
                    break;
                case TRAIT_TASKMGR_RSS:
                    memory(scratch, rows[at].rss_kib, sizeof(scratch));
                    draw_cell(surface, list, cell, scratch, baseline,
                              TRAIT_TEXT, true);
                    break;
                default:
                    (void)number(scratch, rows[at].pid, sizeof(scratch));
                    draw_cell(surface, list, cell, scratch, baseline,
                              TRAIT_TEXT, true);
                    break;
                }
            }
        }
    }

    /* the End Task button */
    {
        struct trait_rect button;

        button.width = 82U;
        button.height = 22U;
        button.x = client.x + client.width - button.width - TASKMGR_PAD;
        button.y = client.y + client.height - button.height - 5U;
        trait_surface_fill(surface, client, button, TRAIT_BG);
        for (at = 0U; at < button.width; ++at) {
            trait_surface_plot(surface, client, button.x + at, button.y,
                               TRAIT_LINE_LIGHT);
            trait_surface_plot(surface, client, button.x + at,
                button.y + button.height - 1U, TRAIT_LINE);
        }
        for (at = 0U; at < button.height; ++at) {
            trait_surface_plot(surface, client, button.x, button.y + at,
                               TRAIT_LINE_LIGHT);
            trait_surface_plot(surface, client,
                button.x + button.width - 1U, button.y + at, TRAIT_LINE);
        }
        {
            uint32_t width = trait_font_width("End Task");

            trait_font_draw(surface, client,
                button.x + (button.width - width) / 2U,
                button.y + 15U, "End Task", TRAIT_FG);
        }
    }
}

/*
 * The self test asks what a sortable list has to get right and what the
 * JavaScript beside this got wrong first time: does sorting by a column
 * actually reorder, and does asking for the same column again reverse it
 * rather than doing nothing?
 */
bool trait_taskmgr_self_test(void)
{
    struct trait_taskmgr_row row;
    uint32_t first;

    trait_taskmgr_reset();
    sort_column = TRAIT_TASKMGR_PID;
    sort_descending = false;

    copy(row.command, "zsh", TRAIT_TASKMGR_NAME_BYTES);
    copy(row.user, "user", TRAIT_TASKMGR_NAME_BYTES);
    row.cpu_tenths = 10U;
    row.rss_kib = 900U;
    row.pid = 7U;
    if (!trait_taskmgr_add(&row)) {
        return false;
    }
    copy(row.command, "awk", TRAIT_TASKMGR_NAME_BYTES);
    row.cpu_tenths = 50U;
    row.rss_kib = 4096U;
    row.pid = 2U;
    if (!trait_taskmgr_add(&row)) {
        return false;
    }
    /* Added out of order, sorted by pid: the low pid comes first. */
    if (rows[0].pid != 2U) {
        return false;
    }
    trait_taskmgr_sort(TRAIT_TASKMGR_COMMAND);
    if (compare_text(rows[0].command, "awk") != 0) {
        return false;
    }
    trait_taskmgr_sort(TRAIT_TASKMGR_CPU);
    first = rows[0].cpu_tenths;
    if (first != 10U) {
        return false;
    }
    /* The same column again REVERSES it. */
    trait_taskmgr_sort(TRAIT_TASKMGR_CPU);
    if (rows[0].cpu_tenths == first) {
        return false;
    }
    if (!trait_taskmgr_sort_descending()) {
        return false;
    }
    trait_taskmgr_reset();
    return true;
}
