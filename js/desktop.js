/*
 * The panel's live parts: the clock, the CPU graph, the pager and the
 * task list.  Everything else on the bar is a launcher, and a launcher
 * that opened nothing would be a button that lies about what it is - so
 * each one opens its window.
 */
"use strict";

/* ------------------------------------------------------------- the clock */

/*
 * ClockFmt=%R, which strftime defines as "the time in 24-hour notation
 * (%H:%M)".  Not a locale-formatted time: %R is the same two fields in
 * the same order wherever it runs.
 */
const WEEKDAY = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];

/*
 * strftime, for the handful of fields the panel's ClockFmt uses.  %R is
 * the profile's own and is "the time in 24-hour notation (%H:%M)"; the
 * rest are there because Settings offers them, and Settings only offers
 * what this can actually format.
 */
function strftime(fmt, now) {
    const two = (n) => String(n).padStart(2, "0");
    const hour12 = now.getHours() % 12 === 0 ? 12 : now.getHours() % 12;

    return fmt
        .replace(/%R/g, two(now.getHours()) + ":" + two(now.getMinutes()))
        .replace(/%T/g, two(now.getHours()) + ":" + two(now.getMinutes()) +
                 ":" + two(now.getSeconds()))
        .replace(/%H/g, two(now.getHours()))
        .replace(/%M/g, two(now.getMinutes()))
        .replace(/%I/g, two(hour12))
        .replace(/%p/g, now.getHours() < 12 ? "AM" : "PM")
        .replace(/%a/g, WEEKDAY[now.getDay()]);
}

function paintClock() {
    const fmt = (typeof SETTINGS === "object" && SETTINGS.clockFormat) ||
        "%R";

    document.getElementById("clock").textContent =
        strftime(fmt, new Date());
}

/* --------------------------------------------------------------- the cpu */

/*
 * lxpanel's cpu plugin keeps one sample per pixel of graph width and
 * scrolls them left, drawing each as a column rising from the bottom.
 * Thirty-six columns because the widget is 40 wide with a 2-pixel border
 * either side.
 */
const CPU_COLUMNS = 36;
const CPU_ROWS = 22;
const cpuHistory = new Array(CPU_COLUMNS).fill(0);

function paintCpu() {
    const canvas = document.getElementById("cpu-canvas");
    const ctx = canvas.getContext("2d");

    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, CPU_COLUMNS, CPU_ROWS);
    ctx.fillStyle = "#00FF00";
    for (let x = 0; x < CPU_COLUMNS; x += 1) {
        const height = Math.round(cpuHistory[x] * CPU_ROWS);
        if (height > 0) {
            ctx.fillRect(x, CPU_ROWS - height, 1, height);
        }
    }
}

/*
 * There is no /proc to read in a browser, so the load is measured the one
 * way a page honestly can: how long a fixed slice of work takes against
 * how long it took when the machine was idlest.  That is a real reading
 * of this machine rather than a decorative sine wave, and it is stated
 * here rather than dressed up as a CPU percentage.
 */
let cpuFloor = Infinity;

function sampleCpu() {
    const started = performance.now();
    let sink = 0;
    for (let i = 0; i < 200000; i += 1) {
        sink += i % 7;
    }
    const took = performance.now() - started + (sink === -1 ? 1 : 0);

    cpuFloor = Math.min(cpuFloor, took);
    /*
     * Against three times the idlest slice seen: the probe is a fixed
     * amount of work, so how much LONGER it takes than its own best is
     * how much of the machine somebody else is using.  Three, because a
     * slice that takes four times its floor is a machine with nothing
     * left, and that is the top of the graph.
     */
    const ratio = cpuFloor > 0 ? (took - cpuFloor) / (cpuFloor * 3) : 0;
    cpuHistory.shift();
    cpuHistory.push(Math.max(0, Math.min(1, ratio)));
    paintCpu();
}

/* ------------------------------------------------------------- the pager */

const DESKTOPS = 2;
let currentDesktop = 0;

function paintPager() {
    const pager = document.getElementById("pager");

    pager.textContent = "";
    for (let index = 0; index < DESKTOPS; index += 1) {
        const cell = document.createElement("div");

        cell.className = index === currentDesktop ? "desk current" : "desk";
        cell.title = "Desktop " + (index + 1);
        cell.addEventListener("click", () => {
            currentDesktop = index;
            paintPager();
        });
        pager.appendChild(cell);
    }
}

/* ------------------------------------------------------------ the taskbar */

const windows = [];

/*
 * Anything that shows the window list wants telling when the list moves.
 * The task manager was drawing itself BEFORE openWindow() had added its
 * own window, so it listed everything except itself until its next tick -
 * a task manager with no row for the task manager.
 */
const windowWatchers = [];

function watchWindows(fn) {
    windowWatchers.push(fn);
    return () => {
        const at = windowWatchers.indexOf(fn);

        if (at >= 0) {
            windowWatchers.splice(at, 1);
        }
    };
}

function paintTaskbar() {
    const bar = document.getElementById("taskbar");

    windowWatchers.forEach((fn) => fn());

    bar.textContent = "";
    windows.forEach((win) => {
        const button = document.createElement("div");
        const icon = document.createElement("img");
        const label = document.createElement("span");

        button.className = win.active ? "task active" : "task";
        button.title = win.title;
        icon.src = win.icon;
        icon.alt = "";
        label.textContent = win.title;
        button.appendChild(icon);
        button.appendChild(label);
        button.addEventListener("click", () => {
            if (win.minimised) {
                setMinimised(win, false);
            } else if (win.active) {
                setMinimised(win, true);
            } else {
                focusWindow(win);
            }
        });
        bar.appendChild(button);
    });
}

/* ---------------------------------------------------------------- windows */

/*
 * A TASK BUTTON WITH NO WINDOW BEHIND IT IS A LIE, so the taskbar is a
 * view of this list and the list holds real frames.  Clicking a button
 * raises its window; clicking the button of the window already on top
 * puts it away, which is what a taskbar does.
 */
const stage = document.getElementById("windows");
let topZ = 10;
/*
 * A window's id, handed out by the thing that starts it - which is what a
 * process id is.  It starts at one because the desktop itself is one, the
 * way a session leader is, and the task manager lists that row first.
 */
let nextPid = 1;

function focusWindow(win) {
    windows.forEach((other) => {
        other.active = false;
        other.frame.classList.add("inactive");
    });
    win.active = true;
    win.frame.classList.remove("inactive");
    topZ += 1;
    win.frame.style.zIndex = String(topZ);
    paintTaskbar();
}

function closeWindow(win) {
    const at = windows.indexOf(win);

    if (at >= 0) {
        windows.splice(at, 1);
    }
    /* So a window can take its timers down with it rather than leave
     * them running against a frame nobody can see. */
    win.frame.dispatchEvent(new CustomEvent("phipia-closed"));
    win.frame.querySelectorAll("*").forEach((el) => {
        el.dispatchEvent(new CustomEvent("phipia-closed"));
    });
    win.frame.remove();
    const last = windows[windows.length - 1];
    if (last) {
        focusWindow(last);
    } else {
        paintTaskbar();
    }
}

function setMinimised(win, minimised) {
    win.minimised = minimised;
    win.frame.hidden = minimised;
    if (minimised) {
        win.active = false;
        win.frame.classList.add("inactive");
        paintTaskbar();
    } else {
        focusWindow(win);
    }
}

/*
 * Maximise fills the work area and restores to exactly where the window
 * was, which is the half that makes it a toggle rather than a one-way
 * trip.  A maximised window does not drag: there is nowhere for it to go.
 */
function toggleMaximised(win) {
    win.maximised = !win.maximised;
    win.frame.classList.toggle("maximised", win.maximised);
    focusWindow(win);
}

/* Dragging by the title bar, which is the only place Openbox lets you. */
function makeDraggable(win, handle) {
    handle.addEventListener("mousedown", (event) => {
        if (event.button !== 0 || event.target.tagName === "BUTTON") {
            return;
        }
        if (win.maximised) {
            return;   /* nowhere to drag a window that fills the screen */
        }
        const startX = event.clientX;
        const startY = event.clientY;
        const fromX = win.frame.offsetLeft;
        const fromY = win.frame.offsetTop;

        focusWindow(win);
        const move = (moved) => {
            win.frame.style.left = (fromX + moved.clientX - startX) + "px";
            win.frame.style.top = (fromY + moved.clientY - startY) + "px";
        };
        const drop = () => {
            document.removeEventListener("mousemove", move);
            document.removeEventListener("mouseup", drop);
        };
        document.addEventListener("mousemove", move);
        document.addEventListener("mouseup", drop);
        event.preventDefault();
    });
}

function openWindow(spec) {
    const frame = document.createElement("div");
    const bar = document.createElement("div");
    const label = document.createElement("span");
    nextPid += 1;
    const win = { title: spec.title, icon: spec.icon, frame: frame,
                  active: true, minimised: false, maximised: false,
                  files: spec.files || null,
                  command: spec.command || spec.title, pid: nextPid };

    frame.className = "window";
    frame.style.left = spec.x + "px";
    frame.style.top = spec.y + "px";
    frame.style.width = spec.width + "px";
    frame.style.height = spec.height + "px";

    bar.className = "titlebar";
    label.className = "title";
    label.textContent = spec.title;
    bar.appendChild(label);
    /*
     * THREE BUTTONS, AND MAXIMISE IS ONE OF THEM NOW.
     *
     * It was left off while there was nothing behind it - a control that
     * does not do what it is drawn as does not get drawn.  There is
     * something behind it now: the window fills the WORK AREA, which is
     * the screen less the panel, because the panel's profile says
     * setpartialstrut=1 and that is the space it reserves.
     */
    [["iconify", "\u2013", "Minimise", () => setMinimised(win, true)],
     ["maximize", "\u25A1", "Maximise", () => toggleMaximised(win)],
     ["close", "\u2715", "Close", () => closeWindow(win)]].forEach(
        ([kind, mark, title, act]) => {
            const button = document.createElement("button");

            button.className = kind;
            button.textContent = mark;
            button.title = title;
            button.addEventListener("click", (event) => {
                event.stopPropagation();
                act();
            });
            bar.appendChild(button);
        });

    /* Openbox maximises on a double click of the title bar, and so does
     * this - the same gesture, on the same strip. */
    bar.addEventListener("dblclick", (event) => {
        if (event.target.tagName !== "BUTTON") {
            toggleMaximised(win);
        }
    });
    frame.appendChild(bar);
    frame.appendChild(spec.body);
    frame.addEventListener("mousedown", () => focusWindow(win));
    stage.appendChild(frame);
    makeDraggable(win, bar);
    windows.push(win);
    focusWindow(win);
    return win;
}

/* --------------------------------------------------------- the terminal */

/*
 * An old black terminal: #000000, a light grey foreground, a monospace
 * face and a block cursor.  It answers a handful of commands and says so
 * when it does not know one - a prompt that swallowed everything would be
 * a picture of a terminal rather than a terminal.
 */
const TERMINAL_HOST = "phipia";
const TERMINAL_USER = "user";

function terminalPrompt() {
    return TERMINAL_USER + "@" + TERMINAL_HOST + ":~$ ";
}

function runCommand(line) {
    const argv = line.trim().split(/\s+/);
    const name = argv[0] || "";

    if (name === "") {
        return "";
    }
    if (name === "help") {
        return "built-ins: help, echo, date, uname, whoami, pwd, ls, " +
            "clear";
    }
    if (name === "echo") {
        return argv.slice(1).join(" ");
    }
    if (name === "date") {
        return new Date().toString();
    }
    if (name === "uname") {
        return argv.includes("-a") ?
            "Phipia " + TERMINAL_HOST + " 1.0 x86_64 GNU/Linux" : "Phipia";
    }
    if (name === "whoami") {
        return TERMINAL_USER;
    }
    if (name === "pwd") {
        return "/home/" + TERMINAL_USER;
    }
    if (name === "ls") {
        return "Desktop  Documents  Downloads  Music  Pictures  Videos";
    }
    if (name === "clear") {
        return null;   /* the one command that empties the screen */
    }
    return name + ": command not found";
}

function makeTerminalBody() {
    const body = document.createElement("div");
    const output = document.createElement("div");
    const line = document.createElement("div");
    const typed = document.createElement("span");
    const cursor = document.createElement("span");
    const prompt = document.createElement("span");

    body.className = "terminal-body";
    body.tabIndex = 0;
    prompt.className = "prompt";
    prompt.textContent = terminalPrompt();
    cursor.className = "cursor";
    line.appendChild(prompt);
    line.appendChild(typed);
    line.appendChild(cursor);
    body.appendChild(output);
    body.appendChild(line);

    let buffer = "";
    const write = (text) => {
        const row = document.createElement("div");

        row.textContent = text;
        output.appendChild(row);
    };

    body.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            const answer = runCommand(buffer);

            write(terminalPrompt() + buffer);
            if (answer === null) {
                output.textContent = "";
            } else if (answer !== "") {
                write(answer);
            }
            buffer = "";
        } else if (event.key === "Backspace") {
            buffer = buffer.slice(0, -1);
        } else if (event.key.length === 1 && !event.ctrlKey &&
                !event.metaKey) {
            buffer += event.key;
        } else {
            return;
        }
        typed.textContent = buffer;
        body.scrollTop = body.scrollHeight;
        event.preventDefault();
    });
    body.addEventListener("mousedown", () => { body.focus(); });
    return body;
}

/* ----------------------------------------------------------------- launch */

let cascade = 0;

function launch(what) {
    const step = (cascade % 6) * 22;

    cascade += 1;
    if (what === "terminal") {
        openWindow({ title: TERMINAL_USER + "@" + TERMINAL_HOST + ": ~",
                     command: "lxterminal",
                     icon: "assets/icons/nuoveXT2/terminal.png",
                     x: 90 + step, y: 70 + step,
                     width: 620, height: 400,
                     body: makeTerminalBody() });
        const body = stage.lastChild.querySelector(".terminal-body");
        if (body) {
            body.focus();
        }
        return;
    }
    if (what === "synaptic") {
        openWindow({ title: "Synaptic Package Manager",
                     command: "synaptic",
                     icon: "assets/icons/nuoveXT2/16/applications-system.png",
                     x: 100 + step, y: 60 + step,
                     width: 700, height: 500,
                     body: makeSynapticWindow().body });
        return;
    }
    if (what === "leafpad") {
        openWindow({ title: "Untitled - Leafpad", command: "leafpad",
                     icon: "assets/icons/nuoveXT2/16/" +
                         "applications-accessories.png",
                     x: 170 + step, y: 100 + step,
                     width: 520, height: 380,
                     body: makeLeafpadWindow() });
        return;
    }
    if (what === "galculator") {
        openWindow({ title: "galculator", command: "galculator",
                     icon: "assets/icons/nuoveXT2/16/" +
                         "applications-accessories.png",
                     x: 240 + step, y: 130 + step,
                     width: 240, height: 300,
                     body: makeGalculatorWindow() });
        return;
    }
    if (what === "settings") {
        openWindow({ title: "Desktop Preferences",
                     command: "lxappearance",
                     icon: "assets/icons/nuoveXT2/16/gtk-preferences.png",
                     x: 150 + step, y: 90 + step,
                     width: 540, height: 420,
                     body: makeSettingsWindow() });
        return;
    }
    if (what === "taskmgr") {
        /* lxtask opens small: it is a list, and a list that fills the
         * screen is a list you have to look around. */
        openWindow({ title: "Task Manager", command: "lxtask",
                     icon: "assets/icons/nuoveXT2/16/applications-system.png",
                     x: 180 + step, y: 110 + step,
                     width: 560, height: 360,
                     body: makeTaskManagerWindow().body });
        return;
    }
    if (what === "files") {
        /*
         * 640 by 480, which is pcmanfm's own win_width and win_height in
         * the LXDE profile rather than a size picked here.
         */
        const files = makeFilesWindow();

        openWindow({ title: "user", command: "pcmanfm",
                     icon: "assets/logo/files.svg",
                     x: 120 + step, y: 70 + step,
                     width: 640, height: 480, body: files.body,
                     files: files });
        return;
    }
    const plain = { browser: ["Web Browser",
                              "assets/icons/nuoveXT2/browser.png"] }[what];
    if (!plain) {
        return;
    }
    const body = document.createElement("div");

    body.style.cssText = "flex:1 1 auto;background:#ededed;color:#333;" +
        "padding:10px;font-size:13px";
    body.textContent = plain[0];
    openWindow({ title: plain[0], command: "x-www-browser",
                 icon: plain[1], x: 120 + step,
                 y: 90 + step, width: 560, height: 360, body: body });
}

/* ------------------------------------------------------------------ start */

document.querySelectorAll("[data-launch]").forEach((button) => {
    button.addEventListener("click", () => {
        launch(button.getAttribute("data-launch"));
    });
});

/*
 * wincmd's Button1 is `iconify` in the profile, so this minimises
 * everything rather than merely dropping focus.  The second press puts
 * back exactly what the first press took down.
 */
let iconifiedByWincmd = [];

document.getElementById("wincmd").addEventListener("click", () => {
    const showing = windows.filter((win) => !win.minimised);

    if (showing.length !== 0) {
        iconifiedByWincmd = showing;
        showing.forEach((win) => setMinimised(win, true));
    } else {
        iconifiedByWincmd.forEach((win) => {
            if (windows.indexOf(win) >= 0) {
                setMinimised(win, false);
            }
        });
        iconifiedByWincmd = [];
    }
});

/*
 * THE GRAPH STARTS FULL, and every column in it is a real reading.
 *
 * lxpanel's graph is already thirty-six samples deep by the time anybody
 * looks at a running desktop.  Starting from an empty history draws a
 * black rectangle for the first half-minute, which is what the first
 * screenshot of this panel showed - a CPU monitor with no monitor in it.
 * So the history is filled at load by taking the samples rather than by
 * inventing them: thirty-six probes back to back, which is the same
 * measurement the interval takes, run thirty-six times.
 */
function primeCpu() {
    for (let i = 0; i < CPU_COLUMNS; i += 1) {
        sampleCpu();
    }
}

paintClock();
paintPager();
paintTaskbar();
primeCpu();
setInterval(paintClock, 1000);
setInterval(sampleCpu, 250);

/* ------------------------------------------------------------- the menu */

/*
 * WHAT IS IN IT IS NOT A CHOICE MADE HERE.
 *
 * The panel profile's menu plugin lists its contents literally:
 *
 *     system { }  |  separator  |  item{command=run}  |  separator  |
 *     item{image=gnome-logout  command=logout}
 *
 * `system` is the application menu, which LXDE builds from the .desktop
 * files on the machine and groups by their freedesktop category.  There
 * are no .desktop files in a browser, so the categories here are the
 * freedesktop registered ones that LXDE shows, holding the applications
 * this desktop actually has.  A category with nothing in it is not drawn:
 * LXDE does not draw one either.
 */
/*
 * EVERY ENTRY IS A PACKAGE, AND A PACKAGE THAT IS NOT INSTALLED IS NOT IN
 * THE MENU.  That is what makes the package manager a package manager
 * rather than a shop window: applying a mark rebuilds this list, so
 * installing leafpad puts Leafpad in Accessories and removing it takes it
 * out.  `pkg` names which package provides the entry; an entry with no
 * package is always there.
 */
const MENU_CATEGORIES = [
    ["Accessories", "applications-accessories", [
        ["Terminal", "terminal", "terminal", "lxterminal"],
        ["Text Editor", "applications-accessories", "leafpad", "leafpad"],
        ["Calculator", "applications-accessories", "galculator",
         "galculator"]
    ]],
    ["Graphics", "applications-graphics", []],
    ["Internet", "applications-internet", [
        ["Web Browser", "browser", "browser", null]
    ]],
    ["Office", "applications-office", []],
    ["Sound & Video", "applications-multimedia", []],
    ["System Tools", "applications-system", [
        ["File Manager", "file-manager", "files", "pcmanfm"],
        ["Task Manager", "applications-system", "taskmgr", "lxtask"],
        ["Synaptic Package Manager", "applications-system", "synaptic",
         "synaptic"]
    ]],
    ["Preferences", "gtk-preferences", [
        ["Desktop Preferences", "gtk-preferences", "settings",
         "lxappearance"]
    ]]
];

/* Which applications the installed packages provide, asked of the
 * package manager rather than assumed here. */
function appIsInstalled(pkgName) {
    if (!pkgName || typeof PACKAGES === "undefined") {
        return true;
    }
    const pkg = PACKAGES.filter((p) => p.name === pkgName)[0];

    return pkg ? pkg.installed : false;
}

const MENU16 = "assets/icons/nuoveXT2/16/";

function menuRow(label, icon, onPick) {
    const row = document.createElement("div");
    const mark = document.createElement("img");
    const text = document.createElement("span");

    row.className = "menu-item";
    row.setAttribute("role", "menuitem");
    mark.src = MENU16 + icon + ".png";
    mark.alt = "";
    text.textContent = label;
    row.appendChild(mark);
    row.appendChild(text);
    if (onPick) {
        row.addEventListener("click", (event) => {
            event.stopPropagation();
            closeMenu();
            onPick();
        });
    }
    return row;
}

function menuRule() {
    const rule = document.createElement("div");

    rule.className = "menu-rule";
    return rule;
}

/* Called by the package manager when a mark is applied, so the menu is a
 * view of what is installed rather than a copy of it. */
function rebuildMenu() {
    buildMenu();
}

function buildMenu() {
    const popup = document.getElementById("menu-popup");

    popup.textContent = "";
    MENU_CATEGORIES.forEach(([label, icon, all]) => {
        const entries = all.filter(([, , , pkg]) => appIsInstalled(pkg));

        if (entries.length === 0) {
            return;   /* LXDE leaves an empty category out */
        }
        const row = menuRow(label, icon, null);
        const arrow = document.createElement("span");
        const sub = document.createElement("div");

        arrow.className = "arrow";
        arrow.textContent = "▸";
        row.appendChild(arrow);
        sub.className = "submenu";
        entries.forEach(([name, mark, what]) => {
            sub.appendChild(menuRow(name, mark, () => launch(what)));
        });
        /* The submenu hangs off the right of its row and is pulled up so
         * that its foot sits on the row rather than below the screen. */
        sub.style.left = "100%";
        sub.style.bottom = "0";
        row.appendChild(sub);
        popup.appendChild(row);
    });
    popup.appendChild(menuRule());
    popup.appendChild(menuRow("Run...", "applications-system", () => {
        openRunBox();
    }));
    popup.appendChild(menuRule());
    popup.appendChild(menuRow("Logout", "gnome-logout", () => {
        openLogoutBox();
    }));
}

function menuOpen() {
    return document.getElementById("menu-popup").classList.contains("open");
}

function closeMenu() {
    document.getElementById("menu-popup").classList.remove("open");
}

function openMenu() {
    const popup = document.getElementById("menu-popup");
    const button = document.getElementById("menu-button");
    const panel = document.getElementById("panel");

    popup.classList.add("open");
    /* AWAY FROM WHATEVER EDGE THE PANEL IS ON.  A menu that always opened
     * upwards would run off the top of the screen once Settings moves the
     * bar to edge=top. */
    const bar = panel.getBoundingClientRect();

    popup.style.left = button.getBoundingClientRect().left + "px";
    popup.style.top = (panel.classList.contains("top") ? bar.bottom :
        bar.top - popup.offsetHeight) + "px";
}

document.getElementById("menu-button").addEventListener("click", (event) => {
    event.stopPropagation();
    if (menuOpen()) {
        closeMenu();
    } else {
        openMenu();
    }
});

document.addEventListener("click", () => {
    if (menuOpen()) {
        closeMenu();
    }
});

buildMenu();

/* --------------------------------------------------------- the Run box */

/*
 * lxpanel's Run dialog: one field, Cancel and OK.  It runs what the menu
 * can run, which is this desktop's own applications - anything else comes
 * back as "no such program", because a Run box that silently did nothing
 * would be a field with no bottom to it.
 */
const RUNNABLE = { terminal: "terminal", "lxterminal": "terminal",
                   "x-terminal-emulator": "terminal",
                   pcmanfm: "files", "file-manager": "files",
                   browser: "browser", "x-www-browser": "browser",
                   lxappearance: "settings", settings: "settings",
                   lxtask: "taskmgr", "task-manager": "taskmgr",
                   synaptic: "synaptic", leafpad: "leafpad",
                   galculator: "galculator" };

function makeDialog(title, width) {
    const frame = document.createElement("div");
    const bar = document.createElement("div");
    const label = document.createElement("span");
    const close = document.createElement("button");

    frame.className = "dialog";
    frame.style.width = width + "px";
    frame.style.left = Math.round((window.innerWidth - width) / 2) + "px";
    frame.style.top = "220px";
    bar.className = "titlebar";
    label.className = "title";
    label.textContent = title;
    close.className = "close";
    close.textContent = "✕";
    close.title = "Close";
    close.addEventListener("click", () => frame.remove());
    bar.appendChild(label);
    bar.appendChild(close);
    frame.appendChild(bar);
    document.getElementById("desktop").appendChild(frame);
    return frame;
}

function openRunBox() {
    const frame = makeDialog("Run", 320);
    const body = document.createElement("div");
    const field = document.createElement("input");
    const note = document.createElement("div");
    const row = document.createElement("div");
    const cancel = document.createElement("button");
    const ok = document.createElement("button");

    body.className = "body";
    field.type = "text";
    field.placeholder = "Enter the command to run";
    note.style.cssText = "min-height:16px;margin-top:6px;color:#a22;" +
        "font-size:12px";
    body.appendChild(field);
    body.appendChild(note);
    row.className = "row";
    cancel.textContent = "Cancel";
    ok.textContent = "OK";
    row.appendChild(cancel);
    row.appendChild(ok);
    frame.appendChild(body);
    frame.appendChild(row);

    const run = () => {
        const typed = field.value.trim().toLowerCase();
        const what = RUNNABLE[typed];
        /* A program that is in the catalogue but not installed is not on
         * the machine, so Run cannot start it - the same answer the shell
         * would give. */
        const here = what && (typeof PACKAGES === "undefined" ||
            PACKAGES.filter((p) => p.name === typed).length === 0 ||
            PACKAGES.filter((p) => p.name === typed)[0].installed);

        if (here) {
            frame.remove();
            launch(what);
        } else {
            note.textContent = field.value.trim() === "" ?
                "" : field.value.trim() + ": no such program";
        }
    };

    cancel.addEventListener("click", () => frame.remove());
    ok.addEventListener("click", run);
    field.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            run();
        }
        if (event.key === "Escape") {
            frame.remove();
        }
    });
    field.focus();
    return frame;
}

/* ------------------------------------------------------ the logout box */

/*
 * lxsession-logout offers Shut down, Reboot, Suspend, Hibernate, Log out
 * and Cancel.  Four of those six are things a page cannot do, and a
 * button that cannot do what it says is a button that lies - so this
 * offers the two it can honour and says plainly why the others are not
 * here.  Log out closes every window and puts the desktop back as it was
 * found.
 */
function openLogoutBox() {
    const frame = makeDialog("Log out", 320);
    const body = document.createElement("div");
    const row = document.createElement("div");
    const cancel = document.createElement("button");
    const out = document.createElement("button");

    body.className = "body";
    body.innerHTML = "Close every window and clear the desktop?<br>" +
        "<span style=\"color:#666;font-size:12px\">Shut down, reboot, " +
        "suspend and hibernate are not offered: a page cannot do them, " +
        "and a button that cannot do what it says is not drawn.</span>";
    row.className = "row";
    cancel.textContent = "Cancel";
    out.textContent = "Log out";
    row.appendChild(cancel);
    row.appendChild(out);
    frame.appendChild(body);
    frame.appendChild(row);
    cancel.addEventListener("click", () => frame.remove());
    out.addEventListener("click", () => {
        frame.remove();
        windows.slice().forEach(closeWindow);
    });
    return frame;
}

/* -------------------------------------------------------- the volume */

/*
 * lxpanel's volume plugin: a slider above the icon, and the icon reads
 * the level - volume-2 with waves, volume-1 without, volume-x when muted.
 * Three marks rather than one, so the state is legible without opening
 * anything and without relying on colour.
 */
let volumeLevel = 65;
let volumeMuted = false;

/*
 * TWO MARKS, WHICH IS WHAT LXPANEL SHIPS FOR THIS.
 *
 * Its images directory holds stock_volume.png and mute.png and that is
 * the whole set the volume plugin draws from - a speaker, and a speaker
 * with a cross.  Two wrong turns got here:
 *
 *   - volume-low/medium/high are also in that directory, and they are
 *     COLOURED CONES rather than speakers.  The panel drew a green
 *     triangle where the reference has a speaker.
 *   - nuoveXT2 has a full audio-volume-* family, and every one of them
 *     points LEFT.  The reference's speaker points right, because it is
 *     lxpanel's own file and not the icon theme's.
 *
 * So a four-level scheme was this project inventing a set nobody ships.
 * Two marks, from the two files.
 */
function volumeMark() {
    return (volumeMuted || volumeLevel === 0) ? "mute" : "stock_volume";
}

function paintVolume() {
    const img = document.querySelector("#volume img");

    img.src = "assets/icons/lxpanel/" + volumeMark() + ".png";
    img.alt = volumeMuted ? "Muted" : "Volume " + volumeLevel + "%";
    document.getElementById("volume").title = img.alt;
}

function buildVolumePopup() {
    const popup = document.createElement("div");
    const slider = document.createElement("input");
    const mute = document.createElement("button");

    popup.id = "volume-popup";
    slider.type = "range";
    slider.min = "0";
    slider.max = "100";
    slider.value = String(volumeLevel);
    mute.className = "mute";
    mute.textContent = "×";
    mute.title = "Mute";
    slider.addEventListener("input", () => {
        volumeLevel = Number(slider.value);
        volumeMuted = false;
        paintVolume();
    });
    mute.addEventListener("click", (event) => {
        event.stopPropagation();
        volumeMuted = !volumeMuted;
        paintVolume();
    });
    popup.appendChild(slider);
    popup.appendChild(mute);
    popup.addEventListener("click", (e) => e.stopPropagation());
    document.getElementById("desktop").appendChild(popup);
    return popup;
}

const volumePopup = buildVolumePopup();

document.getElementById("volume").addEventListener("click", (event) => {
    const button = event.currentTarget.getBoundingClientRect();
    const panel = document.getElementById("panel").getBoundingClientRect();

    event.stopPropagation();
    volumePopup.classList.toggle("open");
    if (volumePopup.classList.contains("open")) {
        volumePopup.style.left = Math.round(button.left) + "px";
        volumePopup.style.top =
            (document.getElementById("panel").classList.contains("top") ?
                panel.bottom :
                panel.top - volumePopup.offsetHeight) + "px";
    }
});
document.addEventListener("click", () => {
    volumePopup.classList.remove("open");
});

/* ---------------------------------------------------- the lock screen */

/*
 * A lock that covers the panel too, because a lock you could click past
 * would be a picture of a lock.  Any password unlocks it: there is no
 * account here to check one against, and pretending to check would be
 * worse than saying so.
 */
function buildLockScreen() {
    const lock = document.createElement("div");
    const note = document.createElement("div");
    const box = document.createElement("div");
    const field = document.createElement("input");
    const enter = document.createElement("button");

    lock.id = "lockscreen";
    note.textContent = "This screen is locked.";
    field.type = "password";
    field.placeholder = "Password";
    enter.textContent = "Unlock";
    enter.className = "mute";
    const unlock = () => {
        lock.classList.remove("open");
        field.value = "";
    };
    enter.addEventListener("click", unlock);
    field.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            unlock();
        }
    });
    box.className = "box";
    box.appendChild(field);
    box.appendChild(enter);
    lock.appendChild(note);
    lock.appendChild(box);
    document.getElementById("desktop").appendChild(lock);
    return { lock: lock, field: field };
}

const lockScreen = buildLockScreen();

document.querySelector('[data-launch="lock"]').addEventListener("click",
    () => {
        lockScreen.lock.classList.add("open");
        lockScreen.field.focus();
    });

document.querySelector('[data-launch="logout"]').addEventListener("click",
    () => { openLogoutBox(); });

paintVolume();

/* ---------------------------------------------------- desktop icons */

/*
 * LXDE's desktop is pcmanfm --desktop, and what it puts there by default
 * is the home folder, the trash, and any mounted volume.  There are no
 * volumes to mount here, so there are two - and both open the folder they
 * name in the file manager, because an icon on a desktop that opened
 * nothing would be a picture of an icon.
 */
const DESKTOP_ICONS = [
    ["user", "assets/icons/nuoveXT2/48/user-home.png", "/home/user"],
    ["Trash", "assets/icons/nuoveXT2/48/user-trash.png", "trash:///"]
];

let desktopSelected = null;

function paintDesktopIcons() {
    const host = document.getElementById("desktop-icons");

    host.textContent = "";
    DESKTOP_ICONS.forEach(([label, icon, path]) => {
        const cell = document.createElement("div");
        const img = document.createElement("img");
        const span = document.createElement("span");

        cell.className = desktopSelected === label ?
            "desktop-icon selected" : "desktop-icon";
        img.src = icon;
        img.alt = "";
        span.textContent = label;
        cell.appendChild(img);
        cell.appendChild(span);
        cell.addEventListener("click", (event) => {
            event.stopPropagation();
            desktopSelected = label;
            paintDesktopIcons();
        });
        cell.addEventListener("dblclick", () => {
            launch("files");
            const opened = windows[windows.length - 1];

            if (opened && opened.files) {
                opened.files.go(path);
            }
        });
        host.appendChild(cell);
    });
}

document.getElementById("wallpaper").addEventListener("click", () => {
    if (desktopSelected !== null) {
        desktopSelected = null;
        paintDesktopIcons();
    }
});

/* ------------------------------------------------------ desktop menu */

/*
 * pcmanfm's desktop menu.  The rows it can carry out are live; the rest
 * are DIMMED rather than left out, which is the same rule the file
 * manager's menus follow - the menu keeps its shape and nothing pretends
 * to work.
 */
const DESKTOP_MENU = [
    ["Create New...", null],
    ["Paste", null],
    ["Select All", () => {
        desktopSelected = DESKTOP_ICONS[0][0];
        paintDesktopIcons();
    }],
    null,
    ["Sort Files", null],
    null,
    ["Open in Terminal", () => launch("terminal")],
    ["Open Files", () => launch("files")],
    ["Task Manager", () => launch("taskmgr")],
    ["Package Manager", () => launch("synaptic")],
    null,
    ["Desktop Preferences", () => launch("settings")]
];

function buildDesktopMenu() {
    const menu = document.getElementById("desktop-menu");

    DESKTOP_MENU.forEach((entry) => {
        if (entry === null) {
            const sep = document.createElement("div");

            sep.className = "sep";
            menu.appendChild(sep);
            return;
        }
        const [label, act] = entry;
        const row = document.createElement("div");

        row.className = act ? "row" : "row off";
        row.textContent = label;
        if (act) {
            row.addEventListener("click", () => {
                menu.classList.remove("open");
                act();
            });
        }
        menu.appendChild(row);
    });
}

buildDesktopMenu();

document.getElementById("desktop").addEventListener("contextmenu",
    (event) => {
        const menu = document.getElementById("desktop-menu");
        const onDesktop = event.target.closest(".window") === null &&
            event.target.closest("#panel") === null &&
            event.target.closest("#menu-popup") === null;

        if (!onDesktop) {
            return;   /* a window's own right click is the window's */
        }
        event.preventDefault();
        menu.classList.add("open");
        /* Pulled back onto the screen when it would hang off an edge,
         * which is what a menu at the foot or the right has to do. */
        const width = menu.offsetWidth;
        const height = menu.offsetHeight;
        const room = document.getElementById("desktop")
            .getBoundingClientRect();
        let x = event.clientX;
        let y = event.clientY;

        if (x + width > room.width) {
            x = Math.max(0, room.width - width);
        }
        if (y + height > room.height - 26) {
            y = Math.max(0, room.height - 26 - height);
        }
        menu.style.left = x + "px";
        menu.style.top = y + "px";
    });

document.addEventListener("click", () => {
    document.getElementById("desktop-menu").classList.remove("open");
});

paintDesktopIcons();
