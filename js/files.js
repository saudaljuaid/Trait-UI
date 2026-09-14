/*
 * THE FILES WINDOW, IN PCMANFM'S SHAPE.
 *
 * pcmanfm's LXDE profile (lxde-common, /etc/xdg/pcmanfm/LXDE/pcmanfm.conf)
 * gives the defaults this follows:
 *
 *     win_width=640  win_height=480
 *     view_mode=icon
 *     show_hidden=0
 *     sort=name;ascending;
 *
 * and its window is a menu bar, a toolbar of Back/Forward/Up/Reload/Home
 * with a location bar after them, a side pane of Places, an icon view and
 * a two-field status bar.  Every one of those is here and every one of
 * them works: nothing in this window is drawn as a control that does not
 * do what it is drawn as.
 */
"use strict";

const FILES_ICONS = "assets/icons/nuoveXT2/";
const FILES_ICONS16 = "assets/icons/nuoveXT2/16/";
const FILES_ICONS48 = "assets/icons/nuoveXT2/48/";

/*
 * A filesystem to look at.  Folders hold entries; a file holds a size in
 * bytes, which is what the status bar reports when one is picked.
 */
const FS = {
    "/": { dirs: ["bin", "etc", "home", "usr", "var"], files: {} },
    "/home": { dirs: ["user"], files: {} },
    "/home/user": {
        dirs: ["Desktop", "Documents", "Downloads", "Music", "Pictures",
               "Videos"],
        files: { "README.txt": 1284, ".bashrc": 3526 }
    },
    "/home/user/Desktop": { dirs: [], files: {} },
    "/home/user/Documents": {
        dirs: ["Notes"],
        files: { "report.txt": 20481, "letter.txt": 4096 }
    },
    "/home/user/Documents/Notes": {
        dirs: [], files: { "todo.txt": 312 }
    },
    "/home/user/Downloads": {
        dirs: [], files: { "phipia-ui.tar.gz": 1048576 }
    },
    "/home/user/Music": { dirs: [], files: { "track.ogg": 4194304 } },
    "/home/user/Pictures": {
        dirs: [], files: { "wallpaper.png": 262144, "shot.png": 90112 }
    },
    "/home/user/Videos": { dirs: [], files: {} },
    "/bin": { dirs: [], files: { "sh": 129536, "ls": 142848 } },
    "/etc": { dirs: [], files: { "fstab": 812, "hostname": 7 } },
    "/usr": { dirs: ["bin", "share"], files: {} },
    "/usr/bin": { dirs: [], files: {} },
    "/usr/share": { dirs: [], files: {} },
    "/var": { dirs: [], files: {} },
    "trash:///": { dirs: [], files: {} }
};

/*
 * WHAT IS IN THE TEXT FILES, so that opening one opens something.
 *
 * FS records a size per file; this records the CONTENT of the ones that
 * have any, and the two are kept in step - writing from the editor sets
 * both, so a file that grows in the editor grows in the file manager's
 * Size column too.  A size that did not follow the content would be a
 * number the window no longer stands behind.
 */
const FILE_TEXT = {
    "/home/user/README.txt":
        "Phipia\n======\n\nA copy of the Debian LXDE desktop.\n\n" +
        "The panel is laid out from LXDE's own default profile; the\n" +
        "icons are the real nuoveXT2 files; the wallpaper and the mark\n" +
        "on the menu button are this project's own.\n",
    "/home/user/Documents/report.txt":
        "Report\n------\n\nNothing to report.\n",
    "/home/user/Documents/letter.txt":
        "Dear whoever,\n\nThe desktop is finished enough to write on.\n",
    "/home/user/Documents/Notes/todo.txt":
        "- look at the panel\n- open a terminal\n- read the README\n"
};

/* The bytes a string takes, which is what the file manager reports. */
function textSize(text) {
    return new TextEncoder().encode(text).length;
}

function readFile(path) {
    return FILE_TEXT[path] === undefined ? null : FILE_TEXT[path];
}

/*
 * Writing puts the file in BOTH places: the text here and the size in FS,
 * plus the name in its folder's listing when it is new.  A file manager
 * that did not show a file the editor had just saved would be looking at
 * a different filesystem.
 */
function writeFile(path, text) {
    const cut = path.lastIndexOf("/");
    const dir = cut <= 0 ? "/" : path.slice(0, cut);
    const name = path.slice(cut + 1);

    if (!FS[dir]) {
        return false;
    }
    FILE_TEXT[path] = text;
    FS[dir].files[name] = textSize(text);
    return true;
}

/*
 * THE CLIPBOARD, which belongs to the desktop rather than to a window:
 * copying in one Files window and pasting in another is the whole point
 * of having one.  `cut` says whether pasting should also take the names
 * out of where they came from.
 */
const CLIP = { names: [], from: null, cut: false };

/* "x.txt" already there becomes "x (copy).txt", then "x (copy 2).txt" -
 * pcmanfm's shape, and never a silent overwrite. */
function uniqueName(dir, name) {
    const here = FS[dir];
    const taken = (n) => here.dirs.indexOf(n) >= 0 ||
        here.files[n] !== undefined;
    const dot = name.lastIndexOf(".");
    const stem = dot > 0 ? name.slice(0, dot) : name;
    const ext = dot > 0 ? name.slice(dot) : "";
    let tag = " (copy)";
    let nth = 1;

    if (!taken(name)) {
        return name;
    }
    while (taken(stem + tag + ext)) {
        nth += 1;
        tag = " (copy " + nth + ")";
    }
    return stem + tag + ext;
}

/*
 * Copying a FOLDER copies what is under it, so this walks the tree: the
 * listing in FS and the text in FILE_TEXT both have to follow, or the
 * copy would be a name with nothing behind it.
 */
function copyTree(from, to) {
    const src = FS[from];

    FS[to] = { dirs: [], files: {} };
    Object.keys(src.files).forEach((f) => {
        FS[to].files[f] = src.files[f];
        if (FILE_TEXT[from + "/" + f] !== undefined) {
            FILE_TEXT[to + "/" + f] = FILE_TEXT[from + "/" + f];
        }
    });
    src.dirs.forEach((d) => {
        FS[to].dirs.push(d);
        copyTree(from + "/" + d, to + "/" + d);
    });
}

function dropTree(path) {
    if (!FS[path]) {
        return;
    }
    FS[path].dirs.slice().forEach((d) => dropTree(path + "/" + d));
    Object.keys(FS[path].files).forEach(
        (f) => { delete FILE_TEXT[path + "/" + f]; });
    delete FS[path];
}

/*
 * PASTING, which is the clipboard's whole point and so belongs to the
 * desktop rather than to one window: the file manager pastes into the
 * folder it is showing, the desktop menu pastes into ~/Desktop, and both
 * go through here.  Returns the names that landed, which is what the
 * caller selects afterwards.
 */
function pasteInto(dir) {
    const here = FS[dir];
    const src = FS[CLIP.from];
    const landed = [];

    if (CLIP.names.length === 0 || !here || !src) {
        return landed;
    }
    /* Moving a folder into itself would take the tree with it. */
    if (CLIP.cut && CLIP.names.some(
            (n) => dir === CLIP.from + "/" + n ||
                dir.indexOf(CLIP.from + "/" + n + "/") === 0)) {
        notify("Files", "A folder cannot be moved into itself.");
        return landed;
    }
    if (CLIP.cut && CLIP.from === dir) {
        return landed;
    }
    CLIP.names.forEach((name) => {
        const isDir = src.dirs.indexOf(name) >= 0;
        const to = uniqueName(dir, name);
        const fromPath = CLIP.from === "/" ? "/" + name :
            CLIP.from + "/" + name;
        const toPath = dir === "/" ? "/" + to : dir + "/" + to;

        if (isDir) {
            if (!FS[fromPath]) {
                return;
            }
            copyTree(fromPath, toPath);
            here.dirs.push(to);
            if (CLIP.cut) {
                src.dirs.splice(src.dirs.indexOf(name), 1);
                dropTree(fromPath);
            }
        } else {
            if (src.files[name] === undefined) {
                return;
            }
            here.files[to] = src.files[name];
            if (FILE_TEXT[fromPath] !== undefined) {
                FILE_TEXT[toPath] = FILE_TEXT[fromPath];
            }
            if (CLIP.cut) {
                delete src.files[name];
                delete FILE_TEXT[fromPath];
            }
        }
        landed.push(to);
    });
    if (CLIP.cut) {
        /* A cut is spent once it is pasted; a copy is not. */
        CLIP.names = [];
        CLIP.from = null;
        CLIP.cut = false;
    }
    redrawFilesWindows();
    return landed;
}

/*
 * Every open Files window looks at the same filesystem, and so does the
 * desktop - it draws ~/Desktop.  A change that only redrew the window it
 * was made in would leave the others showing a filesystem that no longer
 * exists, so anything that CHANGES the filesystem comes through here.
 */
function redrawFilesWindows() {
    if (typeof windows !== "undefined") {
        windows.forEach((w) => {
            if (w.files && w.files.redraw) {
                w.files.redraw();
            }
        });
    }
    if (typeof paintDesktopIcons === "function") {
        paintDesktopIcons();
    }
}

const PLACES = [
    ["user", "user-home", "/home/user"],
    ["Desktop", "user-desktop", "/home/user/Desktop"],
    ["Trash", "user-trash", "trash:///"],
    ["Filesystem", "drive-harddisk", "/"]
];

/* Which 48-pixel mark a name gets.  By extension, the way a file manager
 * does it, and the generic sheet when nothing matches. */
function fileMark(name) {
    const dot = name.lastIndexOf(".");
    const ext = dot > 0 ? name.slice(dot + 1).toLowerCase() : "";

    if (["png", "jpg", "jpeg", "gif", "bmp", "svg"].indexOf(ext) >= 0) {
        return "image-x-generic";
    }
    if (["ogg", "mp3", "wav", "flac"].indexOf(ext) >= 0) {
        return "audio-x-generic";
    }
    if (["sh", "py", "pl"].indexOf(ext) >= 0) {
        return "text-x-script";
    }
    if (ext === "") {
        return "application-x-executable";
    }
    return "text-x-generic";
}

function humanSize(bytes) {
    if (bytes < 1024) {
        return bytes + " B";
    }
    if (bytes < 1024 * 1024) {
        return (bytes / 1024).toFixed(1) + " KiB";
    }
    return (bytes / (1024 * 1024)).toFixed(1) + " MiB";
}

function parentOf(path) {
    if (path === "/" || path.indexOf(":///") >= 0) {
        return null;
    }
    const cut = path.lastIndexOf("/");

    return cut <= 0 ? "/" : path.slice(0, cut);
}

function displayName(path) {
    if (path === "trash:///") {
        return "Trash";
    }
    if (path === "/") {
        return "/";
    }
    return path.slice(path.lastIndexOf("/") + 1);
}

function makeFilesWindow() {
    const body = document.createElement("div");
    /*
     * view_mode=icon in pcmanfm's profile, so that is what it opens in.
     * Detailed list is its other view and the View menu switches between
     * them, which is where pcmanfm puts that choice.
     */
    const state = { path: "/home/user", history: ["/home/user"], at: 0,
                    /*
                     * A LIST, NOT ONE NAME.  Select All set a "*" that
                     * every drawing had to know about, Invert Selection
                     * had nothing to invert, and the status bar counted
                     * a wildcard rather than a selection.  It is the
                     * names that are picked.
                     */
                    selected: [], showHidden: false, view: "icon" };

    body.className = "files-body files";

    /* ------------------------------------------------------ menu bar */
    const menubar = document.createElement("div");

    menubar.className = "files-menubar";
    const MENUS = [
        ["File", [["Create Folder", () => createBox("folder")],
                  ["Create Blank File", () => createBox("file")],
                  null,
                  ["New Window", () => launch("files")],
                  ["Open Terminal", () => launch("terminal")],
                  null,
                  ["Close", () => { closeFilesWindow(body); }]]],
        ["Edit", [["Cut", () => copySelection(true), "sel"],
                  ["Copy", () => copySelection(false), "sel"],
                  ["Paste", () => pasteHere(), "clip"],
                  null,
                  ["Select All", () => selectAll()],
                  ["Invert Selection", () => invertSelection()],
                  null,
                  ["Preferences", () => launch("settings")]]],
        ["View", [["Icon View", () => { state.view = "icon"; draw(); }],
                  ["Detailed List View", () => {
                      state.view = "list";
                      draw();
                  }],
                  null,
                  ["Reload", () => go(state.path, true)],
                  ["Show Hidden", () => {
                      state.showHidden = !state.showHidden;
                      draw();
                  }],
                  null,
                  ["Go Up", () => goUp()]]],
        ["Bookmarks", PLACES.map((p) => [p[0], () => go(p[2])])],
        ["Tools", [["Open Terminal", () => launch("terminal")]]],
        ["Help", [["About", () => aboutBox()]]]
    ];
    MENUS.forEach(([label, rows]) => {
        const item = document.createElement("div");
        const drop = document.createElement("div");

        item.className = "m";
        item.textContent = label;
        drop.className = "drop";
        rows.forEach((row) => {
            if (row === null) {
                const sep = document.createElement("div");

                sep.className = "sep";
                drop.appendChild(sep);
                return;
            }
            const [name, act, needs] = row;
            const cell = document.createElement("div");

            /*
             * A row with nothing behind it is DIMMED rather than left out,
             * so the menu keeps its shape and nothing pretends to work.
             * `needs` names the state a row cannot work without - "sel" a
             * selection to act on, "clip" something on the clipboard - and
             * the row is dimmed until it is there.  A live Paste with an
             * empty clipboard would be exactly the lie this file avoids.
             */
            if (needs) {
                cell.dataset.needs = needs;
            }
            cell.className = act ? "row" : "row off";
            cell.textContent = name;
            if (act) {
                cell.addEventListener("click", (event) => {
                    event.stopPropagation();
                    item.classList.remove("open");
                    if (!cell.classList.contains("off")) {
                        act();
                    }
                });
            }
            drop.appendChild(cell);
        });
        item.appendChild(drop);
        item.addEventListener("click", (event) => {
            const wasOpen = item.classList.contains("open");

            event.stopPropagation();
            drop.querySelectorAll("[data-needs]").forEach((cell) => {
                const ready = cell.dataset.needs === "clip" ?
                    CLIP.names.length > 0 : state.selected.length > 0;

                cell.classList.toggle("off", !ready);
            });
            menubar.querySelectorAll(".m").forEach(
                (m) => m.classList.remove("open"));
            if (!wasOpen) {
                item.classList.add("open");
            }
        });
        menubar.appendChild(item);
    });
    document.addEventListener("click", () => {
        menubar.querySelectorAll(".m").forEach(
            (m) => m.classList.remove("open"));
    });

    /* ------------------------------------------------------- toolbar */
    const toolbar = document.createElement("div");
    const back = toolButton("go-previous", "Back", () => goBack());
    const forward = toolButton("go-next", "Forward", () => goForward());
    const up = toolButton("go-up", "Up", () => goUp());
    const reload = toolButton("view-refresh", "Reload",
                              () => go(state.path, true));
    const home = toolButton("go-home", "Home", () => go("/home/user"));
    const location = document.createElement("input");

    toolbar.className = "files-toolbar";
    location.type = "text";
    location.className = "location";
    location.spellcheck = false;
    location.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            const wanted = location.value.trim();

            if (FS[wanted]) {
                go(wanted);
            } else {
                location.value = state.path;
            }
        }
    });
    [back, forward, up, reload, home].forEach(
        (b) => toolbar.appendChild(b));
    toolbar.appendChild(location);

    function toolButton(icon, title, act) {
        const button = document.createElement("button");
        const img = document.createElement("img");

        img.src = FILES_ICONS16 + icon + ".png";
        img.alt = title;
        button.title = title;
        button.appendChild(img);
        button.addEventListener("click", act);
        return button;
    }

    /* --------------------------------------------------------- panes */
    const panes = document.createElement("div");
    const places = document.createElement("div");
    const view = document.createElement("div");

    panes.className = "files-panes";
    places.className = "files-places";
    view.className = "files-view";
    /*
     * The empty part of the view is a target too: right-clicking it is
     * how pcmanfm offers Paste, and clicking it drops the selection.
     */
    view.addEventListener("contextmenu", (event) => {
        if (event.target !== view) {
            return;
        }
        event.preventDefault();
        openBlankMenu(event.clientX, event.clientY);
    });
    view.addEventListener("click", (event) => {
        if (event.target === view && state.selected.length > 0) {
            state.selected = [];
            draw();
        }
    });
    /*
     * Ctrl+X/C/V, which is what a file manager answers.  The view takes
     * focus so the keys reach THIS window and not another one: two Files
     * windows are open often enough that guessing would be wrong.
     */
    view.tabIndex = 0;
    view.addEventListener("keydown", (event) => {
        if (!event.ctrlKey && !event.metaKey) {
            return;
        }
        if (event.key === "x" || event.key === "c") {
            if (state.selected.length === 0) {
                return;
            }
            event.preventDefault();
            copySelection(event.key === "x");
        } else if (event.key === "v") {
            if (CLIP.names.length === 0) {
                return;
            }
            event.preventDefault();
            pasteHere();
        } else if (event.key === "a") {
            event.preventDefault();
            selectAll();
        }
    });
    view.addEventListener("mousedown", () => view.focus());
    view.addEventListener("click", () => {
        state.selected = [];
        draw();
    });
    panes.appendChild(places);
    panes.appendChild(view);

    /* -------------------------------------------------------- status */
    const status = document.createElement("div");
    const count = document.createElement("span");
    const free = document.createElement("span");

    status.className = "files-status";
    /*
     * pcmanfm's right-hand field is the free space on the filesystem the
     * folder is on.  There is no filesystem under this one, so the field
     * reports WHAT IS IN THE FOLDER instead - a number this window can
     * actually stand behind.
     *
     * The first cut of this printed "Free space: 3.2 GiB (Total: 7.4
     * GiB)" under a comment claiming it was a number the desktop could
     * stand behind.  It was invented, and a status bar that makes up a
     * figure is worse than one that leaves the field out.
     */
    status.appendChild(count);
    status.appendChild(free);

    body.appendChild(menubar);
    body.appendChild(toolbar);
    body.appendChild(panes);
    body.appendChild(status);

    /* ---------------------------------------------------- navigation */

    function go(path, replace) {
        if (!FS[path]) {
            return;
        }
        if (!replace && path !== state.path) {
            state.history = state.history.slice(0, state.at + 1);
            state.history.push(path);
            state.at = state.history.length - 1;
        }
        state.path = path;
        state.selected = [];
        draw();
    }

    function goBack() {
        if (state.at > 0) {
            state.at -= 1;
            state.path = state.history[state.at];
            state.selected = [];
            draw();
        }
    }

    function goForward() {
        if (state.at < state.history.length - 1) {
            state.at += 1;
            state.path = state.history[state.at];
            state.selected = [];
            draw();
        }
    }

    function goUp() {
        const parent = parentOf(state.path);

        if (parent) {
            go(parent);
        }
    }

    function visibleNames() {
        const here = FS[state.path];

        return here.dirs.slice().sort().concat(
            Object.keys(here.files).sort()
                .filter((n) => state.showHidden || n[0] !== "."));
    }

    function selectAll() {
        state.selected = visibleNames();
        draw();
    }

    function invertSelection() {
        const all = visibleNames();

        state.selected = all.filter(
            (n) => state.selected.indexOf(n) < 0);
        draw();
    }

    /*
     * Cut and Copy take the WHOLE selection, which is what makes the
     * selection worth having.  Paste is refused with a word rather than
     * silently when there is nothing to paste or nowhere to put it.
     */
    function copySelection(cut) {
        if (state.selected.length === 0) {
            return;
        }
        CLIP.names = state.selected.slice();
        CLIP.from = state.path;
        CLIP.cut = cut;
    }

    function pasteHere() {
        const landed = pasteInto(state.path);

        if (landed.length === 0) {
            return;
        }
        state.selected = landed;
        state.anchor = landed[landed.length - 1];
        draw();
    }

    function isPicked(name) {
        return state.selected.indexOf(name) >= 0;
    }

    /*
     * Plain click picks one; Ctrl adds or removes; Shift takes the run
     * from the last one picked to this one.  That is what every file
     * manager does, and what makes Select All and Invert Selection
     * commands rather than decorations.
     */
    function pickItem(name, event) {
        const all = visibleNames();

        if (event.shiftKey && state.anchor !== undefined &&
                all.indexOf(state.anchor) >= 0) {
            const from = all.indexOf(state.anchor);
            const to = all.indexOf(name);

            state.selected = all.slice(Math.min(from, to),
                Math.max(from, to) + 1);
        } else if (event.ctrlKey || event.metaKey) {
            if (isPicked(name)) {
                state.selected = state.selected.filter((n) => n !== name);
            } else {
                state.selected = state.selected.concat([name]);
            }
            state.anchor = name;
        } else {
            state.selected = [name];
            state.anchor = name;
        }
        draw();
    }

    function aboutBox() {
        const dialog = makeDialog("About Files", 300);
        const text = document.createElement("div");

        text.className = "body";
        text.innerHTML = "<b>Files</b><br>The desktop's file manager, in " +
            "pcmanfm's shape.";
        dialog.appendChild(text);
    }

    /* ------------------------------------------------------- drawing */

    function draw() {
        const here = FS[state.path];
        const names = here.dirs.slice().concat(
            Object.keys(here.files).filter(
                (n) => state.showHidden || n[0] !== "."));

        location.value = state.path;
        back.disabled = state.at === 0;
        forward.disabled = state.at >= state.history.length - 1;
        up.disabled = parentOf(state.path) === null;

        places.textContent = "";
        PLACES.forEach(([label, icon, path]) => {
            const row = document.createElement("div");
            const img = document.createElement("img");
            const span = document.createElement("span");

            row.className = path === state.path ? "place current" : "place";
            img.src = FILES_ICONS16 + icon + ".png";
            img.alt = "";
            span.textContent = label;
            row.appendChild(img);
            row.appendChild(span);
            row.addEventListener("click", () => go(path));
            places.appendChild(row);
        });

        view.textContent = "";
        view.className = state.view === "list" ?
            "files-view files-view-list" : "files-view";
        if (state.view === "list") {
            drawList(here, names);
            said();
            return;
        }
        if (names.length === 0) {
            const empty = document.createElement("div");

            empty.className = "files-empty";
            empty.textContent = state.path === "trash:///" ?
                "The trash is empty" : "This folder is empty";
            view.appendChild(empty);
        }
        /* sort=name;ascending; - folders first, which is what a file
         * manager sorting by name does with a folder flag. */
        here.dirs.slice().sort().forEach((name) => {
            view.appendChild(entry(name, "folder", true));
        });
        Object.keys(here.files).sort()
            .filter((n) => state.showHidden || n[0] !== ".")
            .forEach((name) => {
                view.appendChild(entry(name, fileMark(name), false));
            });

        said();
    }

    /*
     * The detailed list: Name, Description, Size, Modified - which are
     * pcmanfm's own columns, in its own order.  A folder's size is not a
     * number a file manager reports, so that cell is empty rather than
     * filled in with nought.
     */
    const LIST_COLUMNS = [["Name", "1 1 0"], ["Description", "0 0 128px"],
                          ["Size", "0 0 82px"], ["Modified", "0 0 96px"]];

    function drawList(here, names) {
        const tree = document.createElement("div");
        const head = document.createElement("div");

        tree.className = "gtk-tree files-tree";
        head.className = "head";
        LIST_COLUMNS.forEach(([label, flex]) => {
            const cell = document.createElement("div");

            cell.style.flex = flex;
            cell.textContent = label;
            head.appendChild(cell);
        });
        tree.appendChild(head);
        const rows = document.createElement("div");

        here.dirs.slice().sort().forEach((name) => {
            rows.appendChild(listRow(name, "folder", true, null));
        });
        Object.keys(here.files).sort()
            .filter((n) => state.showHidden || n[0] !== ".")
            .forEach((name) => {
                rows.appendChild(listRow(name, fileMark(name), false,
                                         here.files[name]));
            });
        tree.appendChild(rows);
        view.appendChild(tree);
        (void 0);
        if (names.length === 0) {
            const empty = document.createElement("div");

            empty.className = "files-empty";
            empty.textContent = state.path === "trash:///" ?
                "The trash is empty" : "This folder is empty";
            view.appendChild(empty);
        }
    }

    function listRow(name, mark, isDir, size) {
        const line = document.createElement("div");
        const fields = [name, isDir ? "Folder" : describe(name),
                        isDir ? "" : humanSize(size), "14 Sep 2026"];

        line.className = isPicked(name) ? "line selected" : "line";
        fields.forEach((text, at) => {
            const cell = document.createElement("div");

            cell.style.flex = LIST_COLUMNS[at][1];
            cell.className = at === 2 ? "num" : "";
            cell.textContent = text;
            if (at === 0) {
                const img = document.createElement("img");

                /* The same mark the icon view gives it, at 16 - a
                 * drawer beside a text file would be the file manager's
                 * own icon standing in for a document. */
                img.src = FILES_ICONS16 + (isDir ? "folder" : mark) +
                    ".png";
                img.alt = "";
                img.className = "row-mark";
                cell.prepend(img);
            }
            line.appendChild(cell);
        });
        line.addEventListener("click", (event) => {
            event.stopPropagation();
            pickItem(name, event);
        });
        line.addEventListener("dblclick", () => {
            if (isDir) {
                go(state.path === "/" ? "/" + name :
                   state.path + "/" + name);
            }
        });
        line.addEventListener("contextmenu", (event) => {
            event.preventDefault();
            event.stopPropagation();
            if (!isPicked(name)) {
                state.selected = [name];
                state.anchor = name;
            }
            draw();
            openItemMenu(name, isDir, event.clientX, event.clientY);
        });
        return line;
    }

    function describe(name) {
        const dot = name.lastIndexOf(".");
        const ext = dot > 0 ? name.slice(dot + 1).toLowerCase() : "";
        const known = { txt: "plain text document", png: "PNG image",
                        gz: "gzip archive", ogg: "Ogg audio" };

        return known[ext] || (ext === "" ? "executable" :
            ext.toUpperCase() + " file");
    }

    function said() {
        const here = FS[state.path];
        const names = here.dirs.slice().concat(
            Object.keys(here.files).filter(
                (n) => state.showHidden || n[0] !== "."));
        let total = 0;

        Object.keys(here.files)
            .filter((n) => state.showHidden || n[0] !== ".")
            .forEach((n) => { total += here.files[n]; });
        free.textContent = total === 0 ? "" :
            humanSize(total) + " in this folder";

        if (state.selected.length > 1) {
            /* What was picked, and how much of it - which is what a
             * file manager says about a run of files. */
            let bytes = 0;

            state.selected.forEach((n) => {
                if (here.files[n] !== undefined) {
                    bytes += here.files[n];
                }
            });
            count.textContent = state.selected.length + " items selected" +
                (bytes === 0 ? "" : " (" + humanSize(bytes) + ")");
        } else if (state.selected.length === 1 &&
                here.files[state.selected[0]] !== undefined) {
            count.textContent = "\"" + state.selected[0] + "\" (" +
                humanSize(here.files[state.selected[0]]) + ") selected";
        } else if (state.selected.length === 1) {
            count.textContent = "\"" + state.selected[0] + "\" selected";
        } else {
            count.textContent = names.length +
                (names.length === 1 ? " item" : " items");
        }
    }

    /*
     * pcmanfm's context menu on a file or folder: Open, then the two
     * commands that change it, then Properties.  Rename and Delete work -
     * this window has a filesystem of its own to change - and Open on a
     * file nothing installed can read says so, which is more use than a
     * row that quietly does nothing.
     */
    function fullPath(name) {
        return state.path === "/" ? "/" + name : state.path + "/" + name;
    }

    function note(text) {
        const dialog = makeDialog("Files", 330);
        const line = document.createElement("div");

        line.className = "body";
        line.textContent = text;
        dialog.appendChild(line);
    }

    /*
     * pcmanfm's menu on the EMPTY part of the view, which is where Paste
     * lives: there is no item under the pointer to act on, so the rows
     * are the ones that act on the folder itself.
     */
    function openBlankMenu(x, y) {
        popupMenu([
            ["Create Folder", () => createBox("folder")],
            ["Create Blank File", () => createBox("file")],
            null,
            ["Paste", CLIP.names.length > 0 ? () => pasteHere() : null],
            null,
            ["Select All", () => selectAll()],
            ["Reload", () => go(state.path, true)]
        ], x, y);
    }

    function openItemMenu(name, isDir, x, y) {
        const rows = [
            ["Open", () => {
                if (isDir) {
                    go(fullPath(name));
                } else if (name.slice(-4) === ".txt" &&
                        appIsInstalled("leafpad")) {
                    /* Opened AT the file, not merely opened. */
                    launch("leafpad", fullPath(name));
                } else if (name.slice(-4) === ".txt") {
                    note("Nothing installed opens a text file. leafpad " +
                        "is in the package manager.");
                } else {
                    note("There is no application installed that opens " +
                        name + ".");
                }
            }],
            null,
            ["Cut", () => copySelection(true)],
            ["Copy", () => copySelection(false)],
            null,
            ["Rename", () => renameBox(name, isDir)],
            ["Delete", () => deleteBox(name, isDir)],
            null,
            ["Properties", () => propertiesBox(name, isDir)]
        ];

        popupMenu(rows, x, y);
    }

    function popupMenu(rows, x, y) {
        const menu = document.createElement("div");
        const room = document.getElementById("desktop")
            .getBoundingClientRect();

        menu.id = "window-menu";
        menu.className = "open";
        rows.forEach((entry_) => {
            if (entry_ === null) {
                const sep = document.createElement("div");

                sep.className = "sep";
                menu.appendChild(sep);
                return;
            }
            const row = document.createElement("div");

            /* Same rule as the menu bar: a row with nothing behind it is
             * dimmed, not live and silent. */
            row.className = entry_[1] ? "row" : "row off";
            row.textContent = entry_[0];
            if (entry_[1]) {
                row.addEventListener("click", () => {
                    menu.remove();
                    entry_[1]();
                });
            }
            menu.appendChild(row);
        });
        document.getElementById("desktop").appendChild(menu);
        menu.style.left = Math.min(x, Math.max(0,
            room.width - menu.offsetWidth)) + "px";
        menu.style.top = Math.min(y, Math.max(0,
            room.height - menu.offsetHeight)) + "px";
        const away = () => {
            menu.remove();
            document.removeEventListener("click", away);
        };

        setTimeout(() => document.addEventListener("click", away), 0);
    }

    /*
     * pcmanfm's Create New: a folder, or an empty file.  Both are rows
     * that were dimmed while this window could not make one; it can, so
     * they are live.  A name already taken is refused rather than
     * quietly overwriting what is there.
     */
    function createBox(kind) {
        const here = FS[state.path];

        askName(kind === "folder" ? "Create Folder" : "Create File",
            "Create", kind === "folder" ? "New Folder" : "New File.txt",
            (wanted) => {
                if (wanted === "") {
                    return "Give it a name";
                }
                if (wanted.indexOf("/") >= 0) {
                    return "A name cannot hold a /";
                }
                if (here.dirs.indexOf(wanted) >= 0 ||
                        here.files[wanted] !== undefined) {
                    return "\u201C" + wanted + "\u201D is already here";
                }
                if (kind === "folder") {
                    here.dirs.push(wanted);
                    FS[fullPath(wanted)] = { dirs: [], files: {} };
                } else {
                    writeFile(fullPath(wanted), "");
                }
                state.selected = [wanted];
                state.anchor = wanted;
                draw();
                redrawFilesWindows();
                return true;
            });
    }

    /* One box for every "type a name and press a button" in this window,
     * so Rename and Create cannot drift apart. */
    function askName(title, action, start, then) {
        const dialog = makeDialog(title, 330);
        const wrap = document.createElement("div");
        const field = document.createElement("input");
        const note = document.createElement("div");
        const row = document.createElement("div");
        const cancel = document.createElement("button");
        const ok = document.createElement("button");

        wrap.className = "body";
        field.type = "text";
        field.value = start;
        note.style.cssText = "min-height:16px;margin-top:6px;color:#a22;" +
            "font-size:12px";
        wrap.appendChild(field);
        wrap.appendChild(note);
        row.className = "row";
        cancel.textContent = "Cancel";
        ok.textContent = action;
        row.appendChild(cancel);
        row.appendChild(ok);
        dialog.appendChild(wrap);
        dialog.appendChild(row);
        const go = () => {
            const said = then(field.value.trim());

            if (said === true) {
                dialog.remove();
            } else {
                note.textContent = said;
            }
        };

        cancel.addEventListener("click", () => dialog.remove());
        ok.addEventListener("click", go);
        field.addEventListener("keydown", (event) => {
            if (event.key === "Enter") {
                go();
            }
            if (event.key === "Escape") {
                dialog.remove();
            }
        });
        field.focus();
        field.select();
        return dialog;
    }

    function renameBox(name, isDir) {
        const dialog = makeDialog("Rename", 330);
        const wrap = document.createElement("div");
        const field = document.createElement("input");
        const row = document.createElement("div");
        const cancel = document.createElement("button");
        const ok = document.createElement("button");
        const here = FS[state.path];

        wrap.className = "body";
        field.type = "text";
        field.value = name;
        wrap.appendChild(field);
        row.className = "row";
        cancel.textContent = "Cancel";
        ok.textContent = "Rename";
        row.appendChild(cancel);
        row.appendChild(ok);
        dialog.appendChild(wrap);
        dialog.appendChild(row);

        const rename = () => {
            const wanted = field.value.trim();

            if (wanted === "" || wanted === name) {
                dialog.remove();
                return;
            }
            if (isDir) {
                /*
                 * A folder carries its contents with it, so its entry in
                 * FS moves too - a rename that left the contents behind
                 * would be a new empty folder wearing the old name.
                 */
                FS[fullPath(wanted)] = FS[fullPath(name)];
                delete FS[fullPath(name)];
                here.dirs[here.dirs.indexOf(name)] = wanted;
            } else {
                here.files[wanted] = here.files[name];
                delete here.files[name];
            }
            state.selected = [wanted];
            dialog.remove();
            draw();
            redrawFilesWindows();
        };

        cancel.addEventListener("click", () => dialog.remove());
        ok.addEventListener("click", rename);
        field.addEventListener("keydown", (event) => {
            if (event.key === "Enter") {
                rename();
            }
            if (event.key === "Escape") {
                dialog.remove();
            }
        });
        field.focus();
        field.select();
    }

    function deleteBox(name, isDir) {
        const dialog = makeDialog("Delete", 330);
        const wrap = document.createElement("div");
        const row = document.createElement("div");
        const cancel = document.createElement("button");
        const ok = document.createElement("button");
        const here = FS[state.path];

        wrap.className = "body";
        wrap.textContent = "Delete \u201C" + name + "\u201D?";
        row.className = "row";
        cancel.textContent = "Cancel";
        ok.textContent = "Delete";
        row.appendChild(cancel);
        row.appendChild(ok);
        dialog.appendChild(wrap);
        dialog.appendChild(row);
        cancel.addEventListener("click", () => dialog.remove());
        ok.addEventListener("click", () => {
            if (isDir) {
                here.dirs.splice(here.dirs.indexOf(name), 1);
                delete FS[fullPath(name)];
            } else {
                delete here.files[name];
            }
            state.selected = [];
            dialog.remove();
            draw();
            redrawFilesWindows();
        });
    }

    /*
     * Properties: the name, what it is, where it is, and how big.  A
     * FOLDER is reported by what is in it rather than by a size, which is
     * what a file manager can actually say about one.
     */
    function propertiesBox(name, isDir) {
        const dialog = makeDialog("Properties", 350);
        const wrap = document.createElement("div");
        const here = FS[state.path];
        const inside = isDir ? FS[fullPath(name)] : null;
        const facts = [
            ["Name", name],
            ["Type", isDir ? "Folder" : describe(name)],
            ["Location", state.path],
            [isDir ? "Contents" : "Size",
             isDir ? (inside.dirs.length +
                 Object.keys(inside.files).length) + " items" :
                 humanSize(here.files[name])],
            ["Modified", "14 Sep 2026"]
        ];

        wrap.className = "body";
        facts.forEach(([label, value]) => {
            const line = document.createElement("div");
            const key = document.createElement("b");
            const val = document.createElement("span");

            line.style.cssText = "display:flex;gap:8px;margin-bottom:3px";
            key.style.cssText = "flex:0 0 84px";
            key.textContent = label;
            val.textContent = value;
            line.appendChild(key);
            line.appendChild(val);
            wrap.appendChild(line);
        });
        dialog.appendChild(wrap);
    }

    function entry(name, mark, isDir) {
        const cell = document.createElement("div");
        const img = document.createElement("img");
        const span = document.createElement("span");

        cell.className = isPicked(name) ?
            "files-entry selected" : "files-entry";
        img.src = FILES_ICONS48 + mark + ".png";
        img.alt = "";
        span.textContent = name;
        cell.appendChild(img);
        cell.appendChild(span);
        cell.addEventListener("click", (event) => {
            event.stopPropagation();
            pickItem(name, event);
        });
        cell.addEventListener("dblclick", () => {
            if (isDir) {
                go(state.path === "/" ? "/" + name :
                   state.path + "/" + name);
            }
        });
        cell.addEventListener("contextmenu", (event) => {
            event.preventDefault();
            event.stopPropagation();
            if (!isPicked(name)) {
                state.selected = [name];
                state.anchor = name;
            }
            draw();
            openItemMenu(name, isDir, event.clientX, event.clientY);
        });
        return cell;
    }

    draw();
    return { body: body, state: state, go: go, redraw: draw };
}

/* The window this lives in is closed the way any other is; the File menu
 * needs a handle on it, so the frame is found from the body. */
function closeFilesWindow(body) {
    const frame = body.closest(".window");

    if (frame) {
        const win = windows.filter((w) => w.frame === frame)[0];

        if (win) {
            closeWindow(win);
        }
    }
}
