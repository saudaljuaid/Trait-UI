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
            windows.forEach((other) => { other.active = false; });
            win.active = true;
            paintTaskbar();
        });
        bar.appendChild(button);
    });
}

/* ----------------------------------------------------------------- launch */

const LAUNCHERS = {
    files: { title: "File Manager",
             icon: "assets/icons/nuoveXT2/file-manager.png" },
    browser: { title: "Web Browser",
               icon: "assets/icons/nuoveXT2/browser.png" },
    terminal: { title: "Terminal",
                icon: "assets/icons/nuoveXT2/terminal.png" }
};

function launch(what) {
    const spec = LAUNCHERS[what];

    if (!spec) {
        return;
    }
    windows.forEach((other) => { other.active = false; });
    windows.push({ title: spec.title, icon: spec.icon, active: true });
    paintTaskbar();
}

/* ------------------------------------------------------------------ start */

document.querySelectorAll("[data-launch]").forEach((button) => {
    button.addEventListener("click", () => {
        launch(button.getAttribute("data-launch"));
    });
});

document.getElementById("wincmd").addEventListener("click", () => {
    windows.forEach((win) => { win.active = false; });
    paintTaskbar();
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
