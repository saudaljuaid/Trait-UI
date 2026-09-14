/* SPDX-License-Identifier: GPL-3.0-only */
#include <trait/settings.h>

#include <trait/font.h>
#include <trait/theme.h>

#define TAB_HEIGHT 24U
#define TAB_PAD 12U
#define PAGE_PAD 10U
#define ROW_HEIGHT 30U
#define SWITCH_BOX 13U

static struct trait_settings_page pages[TRAIT_SETTINGS_MAX_PAGES];
static uint32_t page_count;
static uint32_t current;

static void copy(char *out, const char *text, uint32_t capacity)
{
    uint32_t at = 0U;

    while (text != NULL && text[at] != '\0' && at + 1U < capacity) {
        out[at] = text[at];
        ++at;
    }
    out[at] = '\0';
}

void trait_settings_reset(void)
{
    page_count = 0U;
    current = 0U;
}

bool trait_settings_add_page(const char *name)
{
    if (name == NULL || page_count >= TRAIT_SETTINGS_MAX_PAGES) {
        return false;
    }
    copy(pages[page_count].name, name, TRAIT_SETTINGS_TEXT_BYTES);
    pages[page_count].row_count = 0U;
    ++page_count;
    return true;
}

bool trait_settings_add_row(uint32_t page,
    const struct trait_settings_row *row)
{
    struct trait_settings_page *target;

    if (row == NULL || page >= page_count) {
        return false;
    }
    target = &pages[page];
    if (target->row_count >= TRAIT_SETTINGS_MAX_ROWS) {
        return false;
    }
    target->rows[target->row_count] = *row;
    copy(target->rows[target->row_count].label, row->label,
         TRAIT_SETTINGS_TEXT_BYTES);
    copy(target->rows[target->row_count].value, row->value,
         TRAIT_SETTINGS_TEXT_BYTES);
    ++target->row_count;
    return true;
}

uint32_t trait_settings_page_count(void)
{
    return page_count;
}

void trait_settings_select(uint32_t page)
{
    if (page < page_count) {
        current = page;
    }
}

uint32_t trait_settings_selected(void)
{
    return current;
}

bool trait_settings_tab_bounds(const struct trait_window *window,
    uint32_t page, struct trait_rect *out)
{
    struct trait_rect client;
    uint32_t left;
    uint32_t at;

    if (window == NULL || out == NULL || page >= page_count) {
        return false;
    }
    client = trait_window_client(window);
    left = client.x + 4U;
    for (at = 0U; at < page; ++at) {
        left += trait_font_width(pages[at].name) + TAB_PAD * 2U;
    }
    out->x = left;
    out->y = client.y + 4U;
    out->width = trait_font_width(pages[page].name) + TAB_PAD * 2U;
    out->height = TAB_HEIGHT;
    return true;
}

/*
 * GTK's notebook: the current tab is the page's own colour and JOINS the
 * page - there is no line under it - while the others sit lower and
 * darker with the line running over them.  That join is the whole of how
 * a notebook says which page you are looking at, so it is drawn rather
 * than approximated with a highlight.
 */
static void draw_tabs(struct trait_surface *surface,
    const struct trait_window *window, struct trait_rect body)
{
    struct trait_rect tab;
    uint32_t at;
    uint32_t edge;

    for (at = 0U; at < page_count; ++at) {
        bool here = at == current;

        if (!trait_settings_tab_bounds(window, at, &tab)) {
            continue;
        }
        if (!here) {
            tab.y += 3U;
            tab.height -= 3U;
        }
        trait_surface_fill(surface, body, tab,
                           here ? TRAIT_BG : TRAIT_BG_ACTIVE);
        for (edge = 0U; edge < tab.width; ++edge) {
            trait_surface_plot(surface, body, tab.x + edge, tab.y,
                               TRAIT_LINE);
        }
        for (edge = 0U; edge < tab.height; ++edge) {
            trait_surface_plot(surface, body, tab.x, tab.y + edge,
                               TRAIT_LINE);
            trait_surface_plot(surface, body, tab.x + tab.width - 1U,
                               tab.y + edge, TRAIT_LINE);
        }
        trait_font_draw(surface, body, tab.x + TAB_PAD,
            tab.y + tab.height - 8U, pages[at].name,
            here ? TRAIT_FG : TRAIT_TEXT);
    }
}

void trait_settings_draw(struct trait_surface *surface,
    const struct trait_window *window)
{
    struct trait_rect client;
    struct trait_rect body;
    struct trait_settings_page *page;
    uint32_t at;
    uint32_t edge;
    uint32_t top;

    if (window == NULL || !trait_surface_valid(surface) ||
            page_count == 0U) {
        return;
    }
    client = trait_window_client(window);
    trait_surface_fill(surface, client, client, TRAIT_BG);

    body.x = client.x + 4U;
    body.y = client.y + 4U + TAB_HEIGHT;
    body.width = client.width > 8U ? client.width - 8U : 0U;
    body.height = client.height > TAB_HEIGHT + 12U ?
        client.height - TAB_HEIGHT - 12U : 0U;
    trait_surface_fill(surface, client, body, TRAIT_BG);
    for (edge = 0U; edge < body.width; ++edge) {
        trait_surface_plot(surface, client, body.x + edge, body.y,
                           TRAIT_LINE);
        trait_surface_plot(surface, client, body.x + edge,
                           body.y + body.height - 1U, TRAIT_LINE);
    }
    for (edge = 0U; edge < body.height; ++edge) {
        trait_surface_plot(surface, client, body.x, body.y + edge,
                           TRAIT_LINE);
        trait_surface_plot(surface, client, body.x + body.width - 1U,
                           body.y + edge, TRAIT_LINE);
    }
    draw_tabs(surface, window, client);

    page = &pages[current];
    top = body.y + PAGE_PAD;
    for (at = 0U; at < page->row_count; ++at) {
        const struct trait_settings_row *row = &page->rows[at];
        uint32_t baseline = top + 12U;

        if (top + ROW_HEIGHT > body.y + body.height) {
            break;
        }
        switch (row->kind) {
        case TRAIT_SETTINGS_SWITCH: {
            struct trait_rect box;

            box.x = body.x + PAGE_PAD;
            box.y = top + 2U;
            box.width = SWITCH_BOX;
            box.height = SWITCH_BOX;
            trait_surface_fill(surface, body, box, TRAIT_BASE);
            for (edge = 0U; edge < SWITCH_BOX; ++edge) {
                trait_surface_plot(surface, body, box.x + edge, box.y,
                                   TRAIT_LINE);
                trait_surface_plot(surface, body, box.x + edge,
                    box.y + SWITCH_BOX - 1U, TRAIT_LINE);
                trait_surface_plot(surface, body, box.x, box.y + edge,
                                   TRAIT_LINE);
                trait_surface_plot(surface, body,
                    box.x + SWITCH_BOX - 1U, box.y + edge, TRAIT_LINE);
            }
            if (row->on) {
                /* A tick, drawn: two strokes, the short one up from the
                 * bottom left and the long one down from the top right. */
                for (edge = 0U; edge < 3U; ++edge) {
                    trait_surface_plot(surface, body, box.x + 3U + edge,
                        box.y + 6U + edge, TRAIT_FG);
                }
                for (edge = 0U; edge < 5U; ++edge) {
                    trait_surface_plot(surface, body, box.x + 6U + edge,
                        box.y + 8U - edge, TRAIT_FG);
                }
            }
            trait_font_draw(surface, body,
                body.x + PAGE_PAD + SWITCH_BOX + 7U, baseline,
                row->label, TRAIT_FG);
            break;
        }
        case TRAIT_SETTINGS_NOTE:
            trait_font_draw(surface, body, body.x + PAGE_PAD, baseline,
                            row->label, TRAIT_TEXT);
            break;
        case TRAIT_SETTINGS_CHOICE:
        default: {
            struct trait_rect box;
            uint32_t width;

            trait_font_draw(surface, body, body.x + PAGE_PAD, baseline,
                            row->label, TRAIT_FG);
            width = trait_font_width(row->value) + 26U;
            box.width = width < 110U ? 110U : width;
            box.height = 20U;
            box.x = body.x + body.width - box.width - PAGE_PAD;
            box.y = top;
            trait_surface_fill(surface, body, box, TRAIT_BG);
            for (edge = 0U; edge < box.width; ++edge) {
                trait_surface_plot(surface, body, box.x + edge, box.y,
                                   TRAIT_LINE_LIGHT);
                trait_surface_plot(surface, body, box.x + edge,
                    box.y + box.height - 1U, TRAIT_LINE);
            }
            for (edge = 0U; edge < box.height; ++edge) {
                trait_surface_plot(surface, body, box.x, box.y + edge,
                                   TRAIT_LINE_LIGHT);
                trait_surface_plot(surface, body,
                    box.x + box.width - 1U, box.y + edge, TRAIT_LINE);
            }
            trait_font_draw(surface, box, box.x + 6U, box.y + 14U,
                            row->value, TRAIT_TEXT);
            /* The drop arrow, which is what says this is a chooser and
             * not a box somebody typed in. */
            for (edge = 0U; edge < 4U; ++edge) {
                uint32_t span;

                for (span = 0U; span + edge < 4U; ++span) {
                    trait_surface_plot(surface, box,
                        box.x + box.width - 13U + edge + span,
                        box.y + 8U + edge, TRAIT_FG);
                }
            }
            break;
        }
        }
        top += ROW_HEIGHT;
    }
}

/*
 * The self test asks the one thing a notebook has to get right: does
 * picking a tab actually change which page is drawn?  A notebook whose
 * tabs only move a highlight is a picture of a notebook.
 */
bool trait_settings_self_test(void)
{
    struct trait_settings_row row;

    trait_settings_reset();
    if (!trait_settings_add_page("Widget") ||
            !trait_settings_add_page("Desktop")) {
        return false;
    }
    copy(row.label, "Widget theme", TRAIT_SETTINGS_TEXT_BYTES);
    copy(row.value, "Clearlooks", TRAIT_SETTINGS_TEXT_BYTES);
    row.kind = TRAIT_SETTINGS_CHOICE;
    row.on = false;
    if (!trait_settings_add_row(0U, &row)) {
        return false;
    }
    copy(row.label, "Show icons on the desktop", TRAIT_SETTINGS_TEXT_BYTES);
    copy(row.value, "", TRAIT_SETTINGS_TEXT_BYTES);
    row.kind = TRAIT_SETTINGS_SWITCH;
    row.on = true;
    if (!trait_settings_add_row(1U, &row)) {
        return false;
    }
    if (trait_settings_selected() != 0U) {
        return false;
    }
    trait_settings_select(1U);
    if (trait_settings_selected() != 1U) {
        return false;
    }
    /* A page that does not exist is REFUSED rather than selected: a tab
     * index off the end would draw whatever was in memory. */
    trait_settings_select(9U);
    if (trait_settings_selected() != 1U) {
        return false;
    }
    trait_settings_reset();
    return true;
}
