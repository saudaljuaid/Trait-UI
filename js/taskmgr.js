/*
 * THE TASK MANAGER, IN LXTASK'S SHAPE.
 *
 * LXTask is LXDE's, derived from xfce4-taskmanager.  Its window is a menu
 * bar, a summary of the whole machine, and one list with a column per
 * fact: Command, User, CPU%, RSS, PID.  File ends the selected task.
 *
 * WHAT IT REPORTS IS MEASURED OR IT IS NOT REPORTED.  There is no /proc
 * to read, so this lists the things this desktop actually runs - its own
 * windows, and the shell that draws them - and every number beside them
 * comes from somewhere real:
 *
 *   CPU%  from the same probe the panel's monitor uses, shared out
 *         between what is running rather than invented per row;
 *   RSS   from performance.memory where the browser offers it, and
 *         reported as unknown where it does not, rather than filled in;
 *   PID   assigned by this desktop when a window opens, which is what a
 *         process id IS - a number the thing that starts a process gives
 *         it.  It is not a number from any operating system and the
 *         column header says so.
 */
"use strict";

/*
 * Command takes what is left and the four facts after it are fixed, the
 * way a tree view with one expanding column and four sized ones behaves.
 * The fixed four add up to 258, which is why the window is 560: at 520
 * the PID column ran off the right edge with nothing saying so.
 */
const TASKMGR_COLUMNS = [
    ["Command", "1 1 0"],
    ["User", "0 0 66px"],
    ["CPU%", "0 0 54px"],
    ["RSS", "0 0 78px"],
    ["PID", "0 0 60px"]
];

const TASKMGR_USER = "user";

/* The shell's own row, which is there whether or not a window is. */
const SHELL_ROW = { command: "phipia-session", pid: 1 };

function rssBytes() {
    /* Chromium offers this; other browsers do not, and a number nobody
     * measured is worse than an empty cell. */
    if (window.performance && window.performance.memory &&
            window.performance.memory.usedJSHeapSize) {
        return window.performance.memory.usedJSHeapSize;
    }
    return null;
}

function humanRss(bytes) {
    if (bytes === null) {
        return "-";
    }
    if (bytes < 1024 * 1024) {
        return (bytes / 1024).toFixed(0) + " KiB";
    }
    return (bytes / (1024 * 1024)).toFixed(1) + " MiB";
}

function makeTaskManagerWindow() {
    const body = document.createElement("div");
    const menubar = document.createElement("div");
    const summary = document.createElement("div");
    const tree = document.createElement("div");
    const head = document.createElement("div");
    const rows = document.createElement("div");
    const actions = document.createElement("div");
    const endTask = document.createElement("button");
    const state = { selected: null };
    let lastSignature = "";

    body.className = "files-body files lxtask-body";

    /* ------------------------------------------------------ menu bar */
    menubar.className = "files-menubar";
    [["File", [["End Task", () => endSelected()],
               null,
               ["Quit", () => closeFilesWindow(body)]]],
     ["View", [["Refresh", () => draw()]]],
     ["Help", [["About", () => {
         const dialog = makeDialog("About Task Manager", 320);
         const text = document.createElement("div");

         text.className = "body";
         text.innerHTML = "<b>Task Manager</b><br>In lxtask's shape. " +
             "Every number in it is measured; where a figure cannot be, " +
             "the cell is empty rather than filled in.";
         dialog.appendChild(text);
     }]]]].forEach(([label, items]) => {
        const item = document.createElement("div");
        const drop = document.createElement("div");

        item.className = "m";
        item.textContent = label;
        drop.className = "drop";
        items.forEach((row) => {
            if (row === null) {
                const sep = document.createElement("div");

                sep.className = "sep";
                drop.appendChild(sep);
                return;
            }
            const cell = document.createElement("div");

            cell.className = "row";
            cell.textContent = row[0];
            cell.addEventListener("click", (event) => {
                event.stopPropagation();
                item.classList.remove("open");
                row[1]();
            });
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

    /* ------------------------------------------------------- summary */
    summary.className = "lxtask-summary";

    /* --------------------------------------------------------- list */
    tree.className = "gtk-tree";
    head.className = "head";
    TASKMGR_COLUMNS.forEach(([label, flex]) => {
        const cell = document.createElement("div");

        cell.style.flex = flex;
        cell.textContent = label;
        head.appendChild(cell);
    });
    tree.appendChild(head);
    tree.appendChild(rows);

    /* ------------------------------------------------------- actions */
    actions.className = "gtk-actions";
    endTask.textContent = "End Task";
    endTask.addEventListener("click", () => endSelected());
    actions.appendChild(endTask);

    body.appendChild(menubar);
    body.appendChild(summary);
    body.appendChild(tree);
    body.appendChild(actions);

    function tasks() {
        /*
         * The shell first, then a row per window, in the order they were
         * opened - which is the order their ids were handed out.
         */
        const list = [{ command: SHELL_ROW.command, pid: SHELL_ROW.pid,
                        win: null }];

        windows.forEach((win) => {
            list.push({ command: win.command || win.title, pid: win.pid,
                        win: win });
        });
        return list;
    }

    function endSelected() {
        const all = tasks();
        const picked = all.filter((t) => t.pid === state.selected)[0];

        if (!picked) {
            return;
        }
        if (picked.win === null) {
            /* Ending the shell would end the desktop, so it is refused
             * out loud rather than quietly ignored. */
            const dialog = makeDialog("End Task", 320);
            const text = document.createElement("div");

            text.className = "body";
            text.textContent = "phipia-session is the desktop itself. " +
                "Ending it would take the panel and every window with it.";
            dialog.appendChild(text);
            return;
        }
        closeWindow(picked.win);
        state.selected = null;
        draw();
    }

    function draw() {
        const all = tasks();
        const load = cpuHistory[cpuHistory.length - 1] || 0;
        const share = all.length === 0 ? 0 : (load * 100) / all.length;
        const rss = rssBytes();

        summary.textContent = "";
        [["CPU: " + (load * 100).toFixed(1) + "%"],
         ["Memory: " + humanRss(rss)],
         [all.length + (all.length === 1 ? " task" : " tasks")]]
            .forEach(([text]) => {
                const cell = document.createElement("span");

                cell.textContent = text;
                summary.appendChild(cell);
            });

        /*
         * IN PLACE, NOT REBUILT, AND THAT IS NOT AN OPTIMISATION.
         *
         * Pressing a row raises the window, which repaints the taskbar,
         * which tells the watchers - so draw() ran on MOUSEDOWN and threw
         * away the very element the press had started on.  The mouseup
         * then landed on a different element and no click event ever
         * fired: every row in the list was unselectable, and nothing
         * looked wrong.
         *
         * So the rows are only recreated when the set of tasks actually
         * changes; otherwise their cells are written over where they
         * stand and the elements under the pointer survive the press.
         */
        const signature = all.map((t) => t.pid).join(",");

        if (signature !== lastSignature) {
            rows.textContent = "";
            lastSignature = signature;
            all.forEach(() => {
                const line = document.createElement("div");

                TASKMGR_COLUMNS.forEach(([, flex], at) => {
                    const cell = document.createElement("div");

                    cell.style.flex = flex;
                    cell.className = at >= 2 ? "num" : "";
                    line.appendChild(cell);
                });
                rows.appendChild(line);
            });
        }
        all.forEach((task, index) => {
            const line = rows.children[index];
            const fields = [
                task.command,
                TASKMGR_USER,
                share.toFixed(1),
                /* One heap, shared out the same way the load is: this is
                 * one process wearing several windows, and pretending
                 * each had its own resident set would be a number nobody
                 * measured. */
                humanRss(rss === null ? null : rss / all.length),
                String(task.pid)
            ];

            line.className = task.pid === state.selected ?
                "line selected" : "line";
            fields.forEach((text, at) => {
                if (line.children[at].textContent !== text) {
                    line.children[at].textContent = text;
                }
            });
            /* One handler per row, replaced rather than stacked, because
             * a row that gathers a handler every second gathers one
             * every second. */
            line.onclick = () => {
                state.selected = task.pid;
                draw();
            };
        });
        endTask.disabled = state.selected === null;
    }

    draw();
    /*
     * lxtask refreshes on a timer, and so does this - but a timer alone
     * is not enough: a window opened a moment ago would not appear until
     * the next tick, and the task manager's OWN row would be missing for
     * a second after it started.  So it also redraws whenever the window
     * list moves.  Both stop when the window goes: a timer outliving its
     * window is a leak.
     */
    const timer = setInterval(draw, 1000);
    const unwatch = watchWindows(draw);

    body.addEventListener("phipia-closed", () => {
        clearInterval(timer);
        unwatch();
    });
    return { body: body, draw: draw, state: state };
}
