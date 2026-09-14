/*
 * THE PACKAGE MANAGER, IN SYNAPTIC'S SHAPE.
 *
 * Debian's own is Synaptic: a menu bar, a toolbar of Reload, Mark All
 * Upgrades, Apply and Properties, a category pane down the left, a
 * package list with a status column, a description pane under it, and a
 * status bar counting what is listed and what is marked.
 *
 * AND IT ACTUALLY INSTALLS.  A store whose Apply button did nothing would
 * be a shop window, so marking a package and applying it changes what
 * this desktop offers: an installed application appears in the menu and
 * can be run, and removing it takes it out again.  The catalogue is
 * therefore exactly as long as the list of applications this desktop can
 * really provide - there is no point listing Firefox when installing it
 * would install nothing.
 */
"use strict";

const PACKAGES = [
    { name: "lxpanel", section: "x11", version: "0.10.1-2",
      installed: true, essential: true,
      summary: "LXDE panel",
      description: "The panel this desktop draws along the foot of the " +
          "screen: the menu button, the launchers, the pager, the task " +
          "list, the CPU monitor, the clock and the tray." },
    { name: "pcmanfm", section: "utils", version: "1.3.2-1",
      installed: true, essential: true, app: "files",
      summary: "extremely fast and lightweight file manager",
      description: "The file manager, and the program that draws the " +
          "desktop's own icons and wallpaper." },
    { name: "lxterminal", section: "x11", version: "0.4.0-1",
      installed: true, app: "terminal",
      summary: "LXDE terminal emulator",
      description: "A terminal emulator with a black screen and a light " +
          "grey foreground." },
    { name: "lxtask", section: "admin", version: "0.1.10-1",
      installed: true, app: "taskmgr",
      summary: "LXDE task manager",
      description: "Lists what is running, what it is using, and ends " +
          "it if asked." },
    { name: "lxappearance", section: "x11", version: "0.6.3-1",
      installed: true, app: "settings",
      summary: "LXDE configuration tool for GTK+ theme",
      description: "Sets the widget theme, the icon theme, the window " +
          "border, the desktop and the panel." },
    { name: "lxde-icon-theme", section: "x11", version: "0.5.1-2.1",
      installed: true, essential: true,
      summary: "LXDE default icon theme based on nuoveXT2",
      description: "The icons this desktop draws, by Alexandre Moore and " +
          "Hong Jen Yee, under the LGPL." },
    { name: "leafpad", section: "editors", version: "0.8.18.1-5",
      installed: false, app: "leafpad",
      summary: "GTK+ based simple text editor",
      description: "A text editor with a menu bar and a white page, and " +
          "nothing else.  Installing it puts it in the Accessories " +
          "menu." },
    { name: "galculator", section: "math", version: "2.1.4-1.1",
      installed: false, app: "galculator",
      summary: "GTK+ based scientific calculator",
      description: "A calculator that adds up.  Installing it puts it " +
          "in the Accessories menu." },
    { name: "synaptic", section: "admin", version: "0.91.2",
      installed: true, essential: true, app: "synaptic",
      summary: "Graphical package manager",
      description: "This window." }
];

/* What a package is marked for, until Apply carries it out. */
const MARKS = {};

function packageState(pkg) {
    if (MARKS[pkg.name] === "install") {
        return { mark: "+", title: "marked for installation" };
    }
    if (MARKS[pkg.name] === "remove") {
        return { mark: "-", title: "marked for removal" };
    }
    return pkg.installed ?
        { mark: "■", title: "installed" } :
        { mark: "□", title: "not installed" };
}

function installedApps() {
    const set = {};

    PACKAGES.forEach((pkg) => {
        if (pkg.installed && pkg.app) {
            set[pkg.app] = true;
        }
    });
    return set;
}

function makeSynapticWindow() {
    const body = document.createElement("div");
    const toolbar = document.createElement("div");
    const panes = document.createElement("div");
    const sections = document.createElement("div");
    const right = document.createElement("div");
    const tree = document.createElement("div");
    const head = document.createElement("div");
    const rows = document.createElement("div");
    const detail = document.createElement("div");
    const status = document.createElement("div");
    const state = { section: "All", selected: null };
    const COLUMNS = [["S", "0 0 26px"], ["Package", "0 0 150px"],
                     ["Version", "0 0 92px"], ["Description", "1 1 0"]];

    body.className = "files-body files synaptic";

    /* -------------------------------------------------------- toolbar */
    toolbar.className = "files-toolbar";
    const apply = toolButton("Apply", () => applyMarks());
    const markAll = toolButton("Mark All Upgrades", () => {
        /* Nothing here is out of date, and saying so is better than a
         * button that appears to have done something. */
        note("Everything installed is at its latest version.");
    });

    function toolButton(label, act) {
        const button = document.createElement("button");

        button.className = "gtk";
        button.style.cssText = "width:auto;padding:2px 10px;height:24px";
        button.textContent = label;
        button.addEventListener("click", act);
        return button;
    }

    toolbar.appendChild(toolButton("Reload", () => draw()));
    toolbar.appendChild(markAll);
    toolbar.appendChild(apply);

    /* ---------------------------------------------------------- panes */
    panes.className = "files-panes";
    sections.className = "files-places";
    right.style.cssText = "flex:1 1 auto;min-width:0;display:flex;" +
        "flex-direction:column";
    tree.className = "gtk-tree";
    head.className = "head";
    COLUMNS.forEach(([label, flex]) => {
        const cell = document.createElement("div");

        cell.style.flex = flex;
        cell.textContent = label;
        head.appendChild(cell);
    });
    tree.appendChild(head);
    tree.appendChild(rows);
    detail.className = "synaptic-detail";
    right.appendChild(tree);
    right.appendChild(detail);
    panes.appendChild(sections);
    panes.appendChild(right);

    status.className = "files-status";

    body.appendChild(toolbar);
    body.appendChild(panes);
    body.appendChild(status);

    function note(text) {
        const dialog = makeDialog("Synaptic", 320);
        const line = document.createElement("div");

        line.className = "body";
        line.textContent = text;
        dialog.appendChild(line);
    }

    function listed() {
        return PACKAGES.filter((pkg) => state.section === "All" ||
            pkg.section === state.section);
    }

    /*
     * Apply carries out every mark at once, which is what Synaptic does -
     * you mark a set and then apply the set.  An essential package is
     * refused: taking the panel away would leave a desktop with no way to
     * put it back.
     */
    function applyMarks() {
        const names = Object.keys(MARKS);

        if (names.length === 0) {
            note("Nothing is marked.");
            return;
        }
        names.forEach((name) => {
            const pkg = PACKAGES.filter((p) => p.name === name)[0];

            if (!pkg) {
                return;
            }
            pkg.installed = MARKS[name] === "install";
            delete MARKS[name];
        });
        rebuildMenu();
        draw();
    }

    function draw() {
        const here = listed();
        const marked = Object.keys(MARKS).length;
        const installed = PACKAGES.filter((p) => p.installed).length;

        sections.textContent = "";
        ["All"].concat(PACKAGES.map((p) => p.section)
            .filter((v, i, a) => a.indexOf(v) === i).sort())
            .forEach((name) => {
                const row = document.createElement("div");

                row.className = name === state.section ?
                    "place current" : "place";
                row.textContent = name;
                row.addEventListener("click", () => {
                    state.section = name;
                    draw();
                });
                sections.appendChild(row);
            });

        rows.textContent = "";
        here.forEach((pkg) => {
            const line = document.createElement("div");
            const shown = packageState(pkg);
            const fields = [shown.mark, pkg.name, pkg.version, pkg.summary];

            line.className = state.selected === pkg.name ?
                "line selected" : "line";
            line.title = shown.title;
            fields.forEach((text, at) => {
                const cell = document.createElement("div");

                cell.style.flex = COLUMNS[at][1];
                cell.textContent = text;
                line.appendChild(cell);
            });
            line.addEventListener("click", () => {
                state.selected = pkg.name;
                draw();
            });
            /*
             * Synaptic marks with a double click or from the Package
             * menu; a double click is the one everybody uses.  An
             * essential package refuses, and says why.
             */
            line.addEventListener("dblclick", () => {
                if (pkg.essential) {
                    note(pkg.name + " is part of the desktop itself. " +
                        "Removing it would leave nothing to put it back " +
                        "with.");
                    return;
                }
                if (MARKS[pkg.name]) {
                    delete MARKS[pkg.name];
                } else {
                    MARKS[pkg.name] = pkg.installed ? "remove" : "install";
                }
                draw();
            });
            rows.appendChild(line);
        });

        const picked = PACKAGES.filter((p) => p.name === state.selected)[0];

        detail.textContent = "";
        if (picked) {
            const title = document.createElement("b");
            const text = document.createElement("div");

            title.textContent = picked.name + " " + picked.version;
            text.textContent = picked.description;
            detail.appendChild(title);
            detail.appendChild(text);
        } else {
            detail.textContent = "No package is selected.";
        }

        status.textContent = "";
        const left = document.createElement("span");
        const rightSpan = document.createElement("span");

        left.textContent = here.length + " listed, " + installed +
            " installed";
        rightSpan.textContent = marked === 0 ? "Nothing marked" :
            marked + (marked === 1 ? " package marked" : " packages marked");
        status.appendChild(left);
        status.appendChild(rightSpan);
        apply.disabled = marked === 0;
    }

    draw();
    return { body: body, draw: draw, state: state };
}
