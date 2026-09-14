/*
 * THE TWO APPLICATIONS THE PACKAGE MANAGER CAN INSTALL.
 *
 * They are in the catalogue because they can actually be provided.  A
 * store listing a program it could not install would be a shop window,
 * so the catalogue is exactly as long as this file plus what is already
 * on the desktop.
 */
"use strict";

/* ---------------------------------------------------------- leafpad */

/*
 * Leafpad is LXDE's text editor and is about as plain as a program gets:
 * a menu bar over a white page and nothing else - no toolbar, no status
 * bar, no tabs.  That plainness is the design, so it is kept.
 */
function makeLeafpadWindow(openPath) {
    const body = document.createElement("div");
    const menubar = document.createElement("div");
    const page = document.createElement("textarea");
    const state = { path: null, name: "Untitled", dirty: false };

    body.className = "files-body files";
    menubar.className = "files-menubar";
    page.className = "leafpad-page";
    page.spellcheck = false;

    function retitle() {
        const frame = body.closest(".window");

        if (frame) {
            const label = frame.querySelector(".titlebar .title");

            if (label) {
                label.textContent = (state.dirty ? "*" : "") + state.name +
                    " - Leafpad";
            }
        }
    }

    /*
     * OPEN AND SAVE REACH THE SAME FILESYSTEM THE FILE MANAGER SHOWS.
     * They were dimmed while there was nothing behind them; there is now,
     * so a file saved here appears in pcmanfm, at the size it actually
     * is, and one opened here is the one pcmanfm lists.
     */
    function loadPath(path) {
        const text = readFile(path);

        if (text === null) {
            return false;
        }
        page.value = text;
        state.path = path;
        state.name = path.slice(path.lastIndexOf("/") + 1);
        state.dirty = false;
        retitle();
        return true;
    }

    function askPath(title, action, start, then) {
        const dialog = makeDialog(title, 340);
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
    }

    function save(as) {
        if (state.path && !as) {
            writeFile(state.path, page.value);
            state.dirty = false;
            retitle();
            return;
        }
        askPath("Save As", "Save",
            state.path || "/home/user/Untitled.txt",
            (path) => {
                if (path === "" || path[0] !== "/") {
                    return "Give a full path, starting with /";
                }
                if (!writeFile(path, page.value)) {
                    return path.slice(0, path.lastIndexOf("/")) +
                        ": no such folder";
                }
                state.path = path;
                state.name = path.slice(path.lastIndexOf("/") + 1);
                state.dirty = false;
                retitle();
                return true;
            });
    }

    [["File", [["New", () => { page.value = ""; state.path = null;
                               state.name = "Untitled";
                               state.dirty = false; retitle(); }],
               ["Open", () => askPath("Open", "Open", "/home/user/",
                   (path) => loadPath(path) ? true :
                       path + ": no such file")],
               ["Save", () => save(false)],
               ["Save As", () => save(true)],
               null,
               ["Quit", () => closeFilesWindow(body)]]],
     ["Edit", [["Select All", () => page.select()],
               ["Cut", () => document.execCommand("cut")],
               ["Copy", () => document.execCommand("copy")],
               ["Paste", null]]],
     ["Search", [["Find", () => {
         const dialog = makeDialog("Find", 300);
         const wrap = document.createElement("div");
         const field = document.createElement("input");

         wrap.className = "body";
         field.type = "text";
         field.placeholder = "Find what";
         wrap.appendChild(field);
         dialog.appendChild(wrap);
         field.addEventListener("keydown", (event) => {
             if (event.key !== "Enter") {
                 return;
             }
             const at = page.value.indexOf(field.value);

             if (at >= 0) {
                 page.focus();
                 page.setSelectionRange(at, at + field.value.length);
                 dialog.remove();
             } else {
                 field.style.background = "#FFECEC";
             }
         });
         field.focus();
     }]]],
     ["Options", [["Word Wrap", () => {
         page.style.whiteSpace =
             page.style.whiteSpace === "pre" ? "pre-wrap" : "pre";
     }]]],
     ["Help", [["About", () => {
         const dialog = makeDialog("About Leafpad", 300);
         const text = document.createElement("div");

         text.className = "body";
         text.innerHTML = "<b>Leafpad</b><br>A menu bar and a page.";
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

            /* Paste is dimmed: there is no clipboard a page may read
             * without asking.  Dimmed rather than hidden, so the menu
             * keeps its shape. */
            cell.className = row[1] ? "row" : "row off";
            cell.textContent = row[0];
            if (row[1]) {
                cell.addEventListener("click", (event) => {
                    event.stopPropagation();
                    item.classList.remove("open");
                    row[1]();
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

    page.addEventListener("input", () => {
        if (!state.dirty) {
            state.dirty = true;
            retitle();
        }
    });

    body.appendChild(menubar);
    body.appendChild(page);
    if (openPath) {
        loadPath(openPath);
    }
    return body;
}

/* -------------------------------------------------------- galculator */

/*
 * A calculator that adds up.  The keypad is the one a basic calculator
 * has and every key on it works; there is no scientific mode, because a
 * mode button that switched to a pad of keys that did nothing would be
 * worse than not offering it.
 */
const CALC_KEYS = [
    ["C", "±", "÷", "×"],
    ["7", "8", "9", "−"],
    ["4", "5", "6", "+"],
    ["1", "2", "3", "="],
    ["0", ".", "", ""]
];

function makeGalculatorWindow() {
    const body = document.createElement("div");
    const screen = document.createElement("div");
    const pad = document.createElement("div");
    const state = { shown: "0", left: null, op: null, fresh: true };

    body.className = "files-body files calc";
    screen.className = "calc-screen";
    pad.className = "calc-pad";

    function show() {
        screen.textContent = state.shown;
    }

    function digit(d) {
        if (state.fresh) {
            state.shown = d === "." ? "0." : d;
            state.fresh = false;
        } else if (d === "." && state.shown.indexOf(".") >= 0) {
            return;
        } else {
            state.shown += d;
        }
        show();
    }

    function resolve() {
        const right = Number(state.shown);

        if (state.op === null || state.left === null) {
            return right;
        }
        if (state.op === "+") {
            return state.left + right;
        }
        if (state.op === "−") {
            return state.left - right;
        }
        if (state.op === "×") {
            return state.left * right;
        }
        /* Dividing by nothing has no answer, and saying so beats
         * printing Infinity. */
        return right === 0 ? null : state.left / right;
    }

    function press(key) {
        if (key === "") {
            return;
        }
        if (key >= "0" && key <= "9") {
            digit(key);
            return;
        }
        if (key === ".") {
            digit(".");
            return;
        }
        if (key === "C") {
            state.shown = "0";
            state.left = null;
            state.op = null;
            state.fresh = true;
            show();
            return;
        }
        if (key === "±") {
            state.shown = state.shown.startsWith("-") ?
                state.shown.slice(1) : "-" + state.shown;
            show();
            return;
        }
        const answer = resolve();

        if (answer === null) {
            state.shown = "cannot divide by zero";
            state.left = null;
            state.op = null;
            state.fresh = true;
            show();
            return;
        }
        if (key === "=") {
            state.shown = String(answer);
            state.left = null;
            state.op = null;
        } else {
            state.left = answer;
            state.op = key;
            state.shown = String(answer);
        }
        state.fresh = true;
        show();
    }

    CALC_KEYS.forEach((row) => {
        row.forEach((key) => {
            const button = document.createElement("button");

            button.className = key === "" ? "calc-key blank" : "calc-key";
            button.textContent = key;
            button.disabled = key === "";
            button.addEventListener("click", () => press(key));
            pad.appendChild(button);
        });
    });

    body.appendChild(screen);
    body.appendChild(pad);
    show();
    return body;
}
