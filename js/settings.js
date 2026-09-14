/*
 * SETTINGS, WHICH ON LXDE IS THREE PROGRAMS.
 *
 * LXDE has no single control panel.  Look and feel is lxappearance, whose
 * tabs are Widget, Colour, Icon Theme, Mouse Cursor, Window Border, Font
 * and Other; the desktop's own settings are pcmanfm's Desktop
 * Preferences; and the bar's are lxpanel's Panel Preferences.  They are
 * one notebook here, with a page per program, because three windows to
 * change three things on one desktop is three windows.
 *
 * EVERY CONTROL ON IT CHANGES SOMETHING.  A settings window whose
 * switches do nothing is the largest possible version of a control that
 * does not do what it is drawn as, so a setting this desktop cannot carry
 * out is not offered - and where lxappearance would list a dozen themes
 * that are not installed, this lists the ones that are.
 */
"use strict";

/* What the desktop is currently set to.  Read by the appliers below. */
const SETTINGS = {
    widget: "Clearlooks",
    iconTheme: "nuoveXT2",
    windowBorder: "Onyx",
    fontSize: 13,
    wallpaperMode: "crop",
    desktopIcons: true,
    panelEdge: "bottom",
    panelHeight: 26,
    clockFormat: "%R",
    showHidden: false
};

const WIDGET_THEMES = {
    /* Clearlooks' own palette, out of its gtkrc in gtk2-engines. */
    Clearlooks: {
        "--gtk-bg": "#EDECEB", "--gtk-bg-prelight": "#F2F1F0",
        "--gtk-bg-active": "#D5D4D3", "--gtk-base": "#FFFFFF",
        "--gtk-base-prelight": "#E1E0DF", "--gtk-fg": "#000000",
        "--gtk-text": "#1A1A1A", "--gtk-sel-bg": "#86ABD9",
        "--gtk-sel-fg": "#FFFFFF", "--gtk-line": "#B6B3AF",
        "--gtk-line-light": "#FAFAF9"
    },
    /*
     * Adwaita's GTK2 palette, from the gtkrc that gnome-themes-extra-data
     * ships beside Clearlooks', and the same shading rules applied to it.
     */
    Adwaita: {
        "--gtk-bg": "#EDEDED", "--gtk-bg-prelight": "#F2F2F2",
        "--gtk-bg-active": "#D5D5D5", "--gtk-base": "#FFFFFF",
        "--gtk-base-prelight": "#E1E1E1", "--gtk-fg": "#2E3436",
        "--gtk-text": "#2E3436", "--gtk-sel-bg": "#4A90D9",
        "--gtk-sel-fg": "#FFFFFF", "--gtk-line": "#C3C3C3",
        "--gtk-line-light": "#FBFBFB"
    },
    "Adwaita-dark": {
        "--gtk-bg": "#3C3C3C", "--gtk-bg-prelight": "#464646",
        "--gtk-bg-active": "#2E2E2E", "--gtk-base": "#2B2B2B",
        "--gtk-base-prelight": "#353535", "--gtk-fg": "#EEEEEC",
        "--gtk-text": "#EEEEEC", "--gtk-sel-bg": "#215D9C",
        "--gtk-sel-fg": "#FFFFFF", "--gtk-line": "#1B1B1B",
        "--gtk-line-light": "#4A4A4A"
    }
};

const BORDER_THEMES = {
    /* Openbox themes, as the two colours a title bar actually needs. */
    Onyx: { active: "#5b6472,#38404c", inactive: "#3b3f46,#2b2f35",
            ink: "#d9dde3", dim: "#9aa0a8" },
    Clearlooks: { active: "#8FB0D6,#6F94BF", inactive: "#DAD8D5,#C3C0BC",
                  ink: "#12233A", dim: "#5E5B57" },
    Mist: { active: "#6E6E6E,#4F4F4F", inactive: "#454545,#333333",
            ink: "#F2F2F2", dim: "#A8A8A8" }
};

function applyWidgetTheme(name) {
    const theme = WIDGET_THEMES[name];

    if (!theme) {
        return;
    }
    Object.keys(theme).forEach((key) => {
        document.documentElement.style.setProperty(key, theme[key]);
    });
    SETTINGS.widget = name;
}

function applyBorderTheme(name) {
    const theme = BORDER_THEMES[name];

    if (!theme) {
        return;
    }
    const root = document.documentElement.style;

    root.setProperty("--ob-active",
        "linear-gradient(" + theme.active.replace(",", ",") + ")");
    root.setProperty("--ob-inactive",
        "linear-gradient(" + theme.inactive.replace(",", ",") + ")");
    root.setProperty("--ob-ink", theme.ink);
    root.setProperty("--ob-dim", theme.dim);
    SETTINGS.windowBorder = name;
}

function applyFontSize(px) {
    document.documentElement.style.setProperty("--ui-font", px + "px");
    SETTINGS.fontSize = px;
}

function applyWallpaperMode(mode) {
    const sizes = { crop: "cover", fit: "contain", stretch: "100% 100%",
                    center: "auto", tile: "auto" };
    const wall = document.getElementById("wallpaper");

    wall.style.backgroundSize = sizes[mode] || "cover";
    wall.style.backgroundRepeat = mode === "tile" ? "repeat" : "no-repeat";
    SETTINGS.wallpaperMode = mode;
}

function applyDesktopIcons(on) {
    document.getElementById("desktop-icons").hidden = !on;
    SETTINGS.desktopIcons = on;
}

function applyPanelEdge(edge) {
    /*
     * --work-top is where the work area STARTS, which is under the panel
     * when the panel is at the top and at nought when it is at the foot.
     * The desktop's icons and a maximised window both read it, so moving
     * the bar moves them with it rather than leaving them under it.
     */
    document.getElementById("panel").classList.toggle("top",
                                                      edge === "top");
    document.documentElement.style.setProperty("--work-top",
        edge === "top" ? "var(--panel-height)" : "0px");
    SETTINGS.panelEdge = edge;
}

function applyPanelHeight(px) {
    document.documentElement.style.setProperty("--panel-height", px + "px");
    SETTINGS.panelHeight = px;
    paintClock();
}

function applyClockFormat(fmt) {
    SETTINGS.clockFormat = fmt;
    paintClock();
}

/* ------------------------------------------------------ the window */

function notebook(pages) {
    const book = document.createElement("div");
    const tabs = document.createElement("div");

    book.className = "gtk-notebook";
    tabs.className = "gtk-tabs";
    book.appendChild(tabs);
    pages.forEach(([label, build], index) => {
        const tab = document.createElement("div");
        const page = document.createElement("div");

        tab.className = index === 0 ? "tab current" : "tab";
        tab.textContent = label;
        page.className = index === 0 ? "gtk-page" : "gtk-page hidden";
        build(page);
        tab.addEventListener("click", () => {
            tabs.querySelectorAll(".tab").forEach(
                (t) => t.classList.remove("current"));
            book.querySelectorAll(".gtk-page").forEach(
                (p) => p.classList.add("hidden"));
            tab.classList.add("current");
            page.classList.remove("hidden");
        });
        tabs.appendChild(tab);
        book.appendChild(page);
    });
    return book;
}

function themeList(names, current, onPick) {
    const list = document.createElement("div");

    list.className = "gtk-list";
    names.forEach((name) => {
        const item = document.createElement("div");

        item.className = name === current ? "item current" : "item";
        item.textContent = name;
        item.addEventListener("click", () => {
            list.querySelectorAll(".item").forEach(
                (i) => i.classList.remove("current"));
            item.classList.add("current");
            onPick(name);
        });
        list.appendChild(item);
    });
    return list;
}

function field(label, control, hint) {
    const wrap = document.createElement("div");
    const name = document.createElement("label");

    wrap.className = "gtk-field";
    name.textContent = label;
    wrap.appendChild(name);
    wrap.appendChild(control);
    if (hint) {
        const note = document.createElement("div");

        note.className = "gtk-hint";
        note.textContent = hint;
        wrap.appendChild(note);
    }
    return wrap;
}

/*
 * Each control carries the name of the setting it changes, which is how
 * anything outside this file - the check harness, or a caller that wants
 * to drive Settings - can reach one without counting its way down the
 * page.  A field found by "the select below the word Height" moves the
 * moment the page does.
 */
function pick(name, options, current, onPick) {
    const box = document.createElement("select");

    box.className = "gtk";
    box.dataset.setting = name;
    options.forEach(([value, label]) => {
        const opt = document.createElement("option");

        opt.value = value;
        opt.textContent = label;
        if (value === current) {
            opt.selected = true;
        }
        box.appendChild(opt);
    });
    box.addEventListener("change", () => onPick(box.value));
    return box;
}

function toggle(name, label, on, onPick) {
    const row = document.createElement("label");
    const box = document.createElement("input");
    const text = document.createElement("span");

    row.className = "gtk-row";
    box.type = "checkbox";
    box.dataset.setting = name;
    box.checked = on;
    text.textContent = label;
    box.addEventListener("change", () => onPick(box.checked));
    row.appendChild(box);
    row.appendChild(text);
    return row;
}

function makeSettingsWindow() {
    const body = document.createElement("div");

    body.className = "files-body files";
    body.appendChild(notebook([
        ["Widget", (page) => {
            page.appendChild(field("Widget theme",
                themeList(Object.keys(WIDGET_THEMES), SETTINGS.widget,
                          applyWidgetTheme),
                "Applies at once, to every window on the desktop."));
            page.appendChild(field("Font size",
                pick("font-size",
                     [[12, "Small (12)"], [13, "Normal (13)"],
                      [15, "Large (15)"], [17, "Larger (17)"]],
                     SETTINGS.fontSize,
                     (v) => applyFontSize(Number(v))),
                "The interface font, which on Debian is Sans."));
        }],
        ["Icon Theme", (page) => {
            page.appendChild(field("Icon theme",
                themeList(["nuoveXT2"], SETTINGS.iconTheme, () => {}),
                "One theme, because one is what is installed.  " +
                "lxappearance lists what is on the machine and so does " +
                "this; a list of themes that are not here would be a " +
                "list of choices that do nothing."));
        }],
        ["Window Border", (page) => {
            page.appendChild(field("Openbox theme",
                themeList(Object.keys(BORDER_THEMES), SETTINGS.windowBorder,
                          applyBorderTheme),
                "The title bar's colours, active and inactive."));
        }],
        ["Desktop", (page) => {
            page.appendChild(field("Wallpaper mode",
                pick("wallpaper-mode",
                     [["crop", "Crop to fit"], ["fit", "Fit whole image"],
                      ["stretch", "Stretch"], ["center", "Centre"],
                      ["tile", "Tile"]],
                     SETTINGS.wallpaperMode, applyWallpaperMode),
                "pcmanfm's own wallpaper_mode, which its LXDE profile " +
                "sets to crop."));
            page.appendChild(toggle("desktop-icons", "Show icons on the desktop",
                SETTINGS.desktopIcons, applyDesktopIcons));
        }],
        ["Panel", (page) => {
            page.appendChild(field("Position on screen",
                pick("panel-edge", [["bottom", "Bottom"], ["top", "Top"]],
                     SETTINGS.panelEdge, applyPanelEdge),
                "edge=bottom in the panel's profile."));
            page.appendChild(field("Height",
                pick("panel-height",
                     [[24, "24"], [26, "26 (default)"], [30, "30"],
                      [36, "36"]],
                     SETTINGS.panelHeight,
                     (v) => applyPanelHeight(Number(v))),
                "height=26 in the panel's profile."));
            page.appendChild(field("Clock format",
                pick("clock-format",
                     [["%R", "%R - 13:45"], ["%T", "%T - 13:45:07"],
                      ["%R %a", "%R %a - 13:45 Sun"],
                      ["%I:%M %p", "%I:%M %p - 01:45 PM"]],
                     SETTINGS.clockFormat, applyClockFormat),
                "ClockFmt in the panel's profile, which is %R."));
        }],
        ["Other", (page) => {
            page.appendChild(toggle("show-hidden", "Show hidden files in new windows",
                SETTINGS.showHidden,
                (on) => { SETTINGS.showHidden = on; }));
            const note = document.createElement("div");

            note.className = "gtk-hint";
            note.textContent = "lxappearance also offers a mouse cursor " +
                "theme and a toolbar style.  Neither is here: a browser " +
                "cannot set an X cursor theme, and this desktop's " +
                "toolbars are icon-only by construction.  A page of " +
                "switches that do nothing is the largest possible " +
                "version of a control that does not do what it is drawn " +
                "as.";
            page.appendChild(note);
        }]
    ]));
    return body;
}
