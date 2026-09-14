/*
 * THE WEB BROWSER, WHICH BROWSES WHAT IS HERE.
 *
 * The panel's second launcher is lxde-x-www-browser.desktop, and until
 * now it opened a grey rectangle with the words "Web Browser" in it -
 * the last control on this desktop that did not do what it was drawn as.
 *
 * There is no network behind this desktop, so it browses the pages this
 * desktop SHIPS: a small set of local documents about the thing you are
 * looking at, reached by address, with Back, Forward, Reload and Home
 * working and links between pages that follow.  A browser of local
 * documents is a browser; a rectangle with a name on it is not.  Typing
 * an address that is not here says so, the way a browser does.
 */
"use strict";

const PAGES = {
    "about:home": {
        title: "Trait OS",
        html: "<h1>Trait OS</h1>" +
            "<p>A copy of the Debian LXDE desktop: the panel, its plugins " +
            "and its icons, with this project's own mark on the menu " +
            "button and a minimal wallpaper in place of the " +
            "distribution's.</p>" +
            "<ul>" +
            "<li><a href='about:panel'>The panel</a></li>" +
            "<li><a href='about:apps'>The applications</a></li>" +
            "<li><a href='about:licences'>Where the icons came from</a></li>" +
            "</ul>"
    },
    "about:panel": {
        title: "The panel",
        html: "<h1>The panel</h1>" +
            "<p>Laid out from LXDE's own default profile, " +
            "<code>/etc/xdg/lxpanel/LXDE/panels/panel</code>, which says " +
            "<code>edge=bottom</code>, <code>height=26</code>, " +
            "<code>fontcolor=#ffffff</code>, and names its plugins in " +
            "this order:</p>" +
            "<pre>space | menu | launchbar | space | wincmd | space |\n" +
            "pager | space | taskbar(expand=1) | cpu | volume |\n" +
            "tray | dclock(%R) | launchbar</pre>" +
            "<p>The CPU monitor is lxpanel's own geometry: a 40 by 26 " +
            "widget, <code>BORDER_SIZE</code> 2, a black ground and " +
            "<code>gdk_color_parse(\"green\")</code> — X11 green, " +
            "which is <code>#00FF00</code>.</p>" +
            "<p><a href='about:home'>Back to the start</a></p>"
    },
    "about:apps": {
        title: "The applications",
        html: "<h1>The applications</h1>" +
            "<dl>" +
            "<dt>pcmanfm</dt><dd>The file manager, at its profile's own " +
            "640 by 480, in icon view with dotfiles hidden.</dd>" +
            "<dt>lxterminal</dt><dd>Black, with a light grey foreground " +
            "and a block cursor.</dd>" +
            "<dt>lxtask</dt><dd>Command, User, CPU%, RSS, PID — and " +
            "every number in it is measured.</dd>" +
            "<dt>lxappearance</dt><dd>Widget, icon theme, window border, " +
            "desktop and panel. Every control changes something.</dd>" +
            "<dt>synaptic</dt><dd>Marks packages, and Apply installs " +
            "them: an installed package appears in the menu.</dd>" +
            "</dl>" +
            "<p><a href='about:home'>Back to the start</a></p>"
    },
    "about:licences": {
        title: "Where the icons came from",
        html: "<h1>Where the icons came from</h1>" +
            "<p>Nothing here was redrawn. Each file is the byte-for-byte " +
            "original out of a Debian package:</p>" +
            "<ul>" +
            "<li><b>lxde-icon-theme</b> 0.5.1-2.1 (nuoveXT2) — " +
            "LGPL-3+, by Alexandre Moore and Hong Jen Yee</li>" +
            "<li><b>lxde-common</b> 0.99.2-4 — GPL-2+</li>" +
            "<li><b>lxpanel-data</b> 0.11.1-2 — GPL-2+</li>" +
            "</ul>" +
            "<p>Two things are this project's own and say so in the " +
            "<code>SOURCE.txt</code> beside them: the mark on the menu " +
            "button, and the wallpaper.</p>" +
            "<p><a href='about:home'>Back to the start</a></p>"
    }
};

function makeBrowserWindow() {
    const body = document.createElement("div");
    const toolbar = document.createElement("div");
    const view = document.createElement("div");
    const status = document.createElement("div");
    const address = document.createElement("input");
    const state = { at: "about:home", history: ["about:home"], index: 0 };

    body.className = "files-body files";
    toolbar.className = "files-toolbar";
    view.className = "browser-view";
    status.className = "files-status";
    address.type = "text";
    address.className = "location";
    address.spellcheck = false;

    function button(icon, title, act) {
        const b = document.createElement("button");
        const img = document.createElement("img");

        img.src = "assets/icons/nuoveXT2/16/" + icon + ".png";
        img.alt = title;
        b.title = title;
        b.appendChild(img);
        b.addEventListener("click", act);
        return b;
    }

    const back = button("go-previous", "Back", () => {
        if (state.index > 0) {
            state.index -= 1;
            state.at = state.history[state.index];
            draw();
        }
    });
    const forward = button("go-next", "Forward", () => {
        if (state.index < state.history.length - 1) {
            state.index += 1;
            state.at = state.history[state.index];
            draw();
        }
    });
    const reload = button("view-refresh", "Reload", () => draw());
    const home = button("go-home", "Home", () => go("about:home"));

    function go(where) {
        if (!PAGES[where]) {
            /* The page a browser shows for an address that is not there,
             * rather than a blank window or a silent refusal. */
            view.innerHTML = "<h1>Not found</h1><p>There is no page at " +
                "<code>" + where.replace(/</g, "&lt;") + "</code>.</p>" +
                "<p><a href='about:home'>Back to the start</a></p>";
            address.value = where;
            status.textContent = "";
            const said = document.createElement("span");

            said.textContent = "Not found";
            status.appendChild(said);
            wireLinks();
            return;
        }
        if (where !== state.at) {
            state.history = state.history.slice(0, state.index + 1);
            state.history.push(where);
            state.index = state.history.length - 1;
        }
        state.at = where;
        draw();
    }

    function wireLinks() {
        view.querySelectorAll("a[href]").forEach((link) => {
            link.addEventListener("click", (event) => {
                event.preventDefault();
                go(link.getAttribute("href"));
            });
        });
    }

    function draw() {
        const page = PAGES[state.at];

        address.value = state.at;
        back.disabled = state.index === 0;
        forward.disabled = state.index >= state.history.length - 1;
        view.innerHTML = page.html;
        wireLinks();
        status.textContent = "";
        const left = document.createElement("span");
        const right = document.createElement("span");

        left.textContent = page.title;
        right.textContent = "Done";
        status.appendChild(left);
        status.appendChild(right);

        const frame = body.closest(".window");

        if (frame) {
            const label = frame.querySelector(".titlebar .title");

            if (label) {
                label.textContent = page.title + " - Web Browser";
            }
        }
    }

    address.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            go(address.value.trim());
        }
    });

    [back, forward, reload, home].forEach((b) => toolbar.appendChild(b));
    toolbar.appendChild(address);
    body.appendChild(toolbar);
    body.appendChild(view);
    body.appendChild(status);
    draw();
    return body;
}
