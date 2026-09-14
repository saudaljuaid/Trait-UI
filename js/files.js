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
                    selected: null, showHidden: false, view: "icon" };

    body.className = "files-body files";

    /* ------------------------------------------------------ menu bar */
    const menubar = document.createElement("div");

    menubar.className = "files-menubar";
    const MENUS = [
        ["File", [["New Window", () => launch("files")],
                  ["Open Terminal", () => launch("terminal")],
                  null,
                  ["Close", () => { closeFilesWindow(body); }]]],
        ["Edit", [["Select All", () => selectAll()],
                  ["Invert Selection", null],
                  null,
                  ["Preferences", null]]],
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
            const [name, act] = row;
            const cell = document.createElement("div");

            /*
             * A row with nothing behind it is DIMMED rather than left out,
             * so the menu keeps its shape and nothing pretends to work.
             */
            cell.className = act ? "row" : "row off";
            cell.textContent = name;
            if (act) {
                cell.addEventListener("click", (event) => {
                    event.stopPropagation();
                    item.classList.remove("open");
                    act();
                });
            }
            drop.appendChild(cell);
        });
        item.appendChild(drop);
        item.addEventListener("click", (event) => {
            const wasOpen = item.classList.contains("open");

            event.stopPropagation();
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
    view.addEventListener("click", () => {
        state.selected = null;
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
        state.selected = null;
        draw();
    }

    function goBack() {
        if (state.at > 0) {
            state.at -= 1;
            state.path = state.history[state.at];
            state.selected = null;
            draw();
        }
    }

    function goForward() {
        if (state.at < state.history.length - 1) {
            state.at += 1;
            state.path = state.history[state.at];
            state.selected = null;
            draw();
        }
    }

    function goUp() {
        const parent = parentOf(state.path);

        if (parent) {
            go(parent);
        }
    }

    function selectAll() {
        state.selected = "*";
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

        line.className = (state.selected === name ||
            state.selected === "*") ? "line selected" : "line";
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
            state.selected = name;
            draw();
        });
        line.addEventListener("dblclick", () => {
            if (isDir) {
                go(state.path === "/" ? "/" + name :
                   state.path + "/" + name);
            }
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

        if (state.selected === "*") {
            count.textContent = names.length + " items selected";
        } else if (state.selected && here.files[state.selected] !== undefined) {
            count.textContent = "\"" + state.selected + "\" (" +
                humanSize(here.files[state.selected]) + ") selected";
        } else if (state.selected) {
            count.textContent = "\"" + state.selected + "\" selected";
        } else {
            count.textContent = names.length +
                (names.length === 1 ? " item" : " items");
        }
    }

    function entry(name, mark, isDir) {
        const cell = document.createElement("div");
        const img = document.createElement("img");
        const span = document.createElement("span");

        cell.className = (state.selected === name || state.selected === "*") ?
            "files-entry selected" : "files-entry";
        img.src = FILES_ICONS48 + mark + ".png";
        img.alt = "";
        span.textContent = name;
        cell.appendChild(img);
        cell.appendChild(span);
        cell.addEventListener("click", (event) => {
            event.stopPropagation();
            state.selected = name;
            draw();
        });
        cell.addEventListener("dblclick", () => {
            if (isDir) {
                go(state.path === "/" ? "/" + name :
                   state.path + "/" + name);
            }
        });
        return cell;
    }

    draw();
    return { body: body, state: state, go: go };
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
