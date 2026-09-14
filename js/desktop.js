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
function paintClock() {
    const now = new Date();
    const hh = String(now.getHours()).padStart(2, "0");
    const mm = String(now.getMinutes()).padStart(2, "0");
    document.getElementById("clock").textContent = hh + ":" + mm;
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

function paintTaskbar() {
    const bar = document.getElementById("taskbar");

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

/* Dragging by the title bar, which is the only place Openbox lets you. */
function makeDraggable(win, handle) {
    handle.addEventListener("mousedown", (event) => {
        if (event.button !== 0 || event.target.tagName === "BUTTON") {
            return;
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
    const win = { title: spec.title, icon: spec.icon, frame: frame,
                  active: true, minimised: false };

    frame.className = "window";
    frame.style.left = spec.x + "px";
    frame.style.top = spec.y + "px";
    frame.style.width = spec.width + "px";
    frame.style.height = spec.height + "px";

    bar.className = "titlebar";
    label.className = "title";
    label.textContent = spec.title;
    bar.appendChild(label);
    [["iconify", "_", () => setMinimised(win, true)],
     ["maximize", "\u25A1", () => { /* one size, so this is a no-op */ }],
     ["close", "\u2715", () => closeWindow(win)]].forEach(
        ([kind, mark, act]) => {
            if (kind === "maximize") {
                return;   /* not drawn: there is nothing behind it */
            }
            const button = document.createElement("button");

            button.className = kind;
            button.textContent = mark;
            button.title = kind === "close" ? "Close" : "Minimise";
            button.addEventListener("click", act);
            bar.appendChild(button);
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
    const plain = { files: ["File Manager",
                            "assets/icons/nuoveXT2/file-manager.png"],
                    browser: ["Web Browser",
                              "assets/icons/nuoveXT2/browser.png"] }[what];
    if (!plain) {
        return;
    }
    const body = document.createElement("div");

    body.style.cssText = "flex:1 1 auto;background:#ededed;color:#333;" +
        "padding:10px;font-size:13px";
    body.textContent = plain[0];
    openWindow({ title: plain[0], icon: plain[1], x: 120 + step,
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
