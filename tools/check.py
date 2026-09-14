#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Drive the desktop in a real browser and check that it does what it draws.

    python3 tools/check.py

Every check here has been broken on purpose and watched to fail; the
failure messages are quoted in the commit that added it.  A check that
survives the removal of the thing it checks is worse than no check.
"""
import sys
from pathlib import Path

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parent.parent
CHROME = Path("/opt/pw-browsers/chromium-1194/chrome-linux/chrome")

# LXDE's own default profile, which is what the panel is a copy of.
PANEL_HEIGHT = 26
PLUGIN_ORDER = ["menu", "launchbar", "wincmd", "pager", "taskbar", "cpu",
                "volume", "tray", "dclock", "launchbar"]

failures = []


def fails(message):
    failures.append(message)
    print("check: " + message, file=sys.stderr)


def check_panel(page):
    box = page.eval_on_selector("#panel", """(el) => {
        const r = el.getBoundingClientRect();
        return { h: r.height, bottom: r.bottom, top: r.top };
    }""")
    if round(box["h"]) != PANEL_HEIGHT:
        fails("the panel is %g pixels tall where the profile says %d"
              % (box["h"], PANEL_HEIGHT))
    if round(box["bottom"]) != 768:
        fails("the panel's foot is at %g rather than the screen's edge"
              % box["bottom"])


def check_order(page):
    """edge=bottom and the plugin list are the whole shape of the panel, so
    a copy that has them in another order is a different panel."""
    got = page.eval_on_selector_all("#panel > .plugin", """(els) =>
        els.map((el) => el.className.replace('plugin', '').trim())""")
    if got != PLUGIN_ORDER:
        fails("the panel runs %s where the profile runs %s"
              % (" ".join(got), " ".join(PLUGIN_ORDER)))


def check_clock(page):
    text = page.inner_text("#clock")
    if len(text) != 5 or text[2] != ":" or not text.replace(":", "").isdigit():
        fails("the clock reads %r, which is not %%R" % text)


def check_cpu(page):
    """The graph has to hold green BEFORE the first interval tick.

    Asking merely whether it holds green does not test what it is meant
    to: the sampler runs every 250ms and fills the graph on its own
    eventually, so the check went on passing with the priming loop turned
    off.  What the priming is for is the FIRST moment - a monitor that is
    black until the interval has filled it is a monitor with nothing in it
    for the first half-minute, which is how this shipped once.  So the
    caller reads it inside one tick of load.
    """
    green = page.evaluate("""() => {
        const c = document.getElementById('cpu-canvas');
        const d = c.getContext('2d').getImageData(0, 0, c.width, c.height);
        let lit = 0;
        for (let i = 0; i < d.data.length; i += 4) {
            if (d.data[i + 1] > 200 && d.data[i] < 60 &&
                    d.data[i + 2] < 60) { lit += 1; }
        }
        return lit;
    }""")
    if green == 0:
        fails("the cpu monitor holds no green at all, so it is a black "
              "rectangle rather than a graph")
    return green


def check_terminal(page):
    page.click('[data-launch="terminal"]')
    page.wait_for_timeout(200)
    if page.eval_on_selector_all(".window", "(e) => e.length") != 1:
        fails("the terminal launcher opened no window")
        return
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 1:
        fails("an open window put no button on the taskbar")

    #
    # A WORD THE PROMPT CANNOT ALSO BE SAYING, and a whole row rather than
    # a substring.  The first version echoed "phipia" and searched the
    # screen for it - which the prompt contains, because the host is
    # called phipia - so an echo that printed nothing went on passing.
    #
    page.click(".terminal-body")
    page.keyboard.type("echo zzmarker")
    page.keyboard.press("Enter")
    page.wait_for_timeout(120)
    rows = page.eval_on_selector_all(
        ".terminal-body div div", "(e) => e.map((r) => r.textContent)")
    if "zzmarker" not in rows:
        fails("`echo zzmarker` put no row of its own on the screen, so the "
              "prompt swallows what is typed at it")

    page.keyboard.type("nosuchcommand")
    page.keyboard.press("Enter")
    page.wait_for_timeout(120)
    if "command not found" not in page.inner_text(".terminal-body"):
        fails("an unknown command was accepted in silence")


def check_wincmd(page):
    """Button1=iconify in the profile: it minimises, and pressing it again
    puts back what it took down."""
    page.click("#wincmd")
    page.wait_for_timeout(150)
    if page.eval_on_selector_all(".window:not([hidden])",
                                 "(e) => e.length") != 0:
        fails("wincmd left a window on screen")
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 1:
        fails("minimising a window took its button off the taskbar, so "
              "there is no way to get it back")
    page.click("#wincmd")
    page.wait_for_timeout(150)
    if page.eval_on_selector_all(".window:not([hidden])",
                                 "(e) => e.length") != 1:
        fails("wincmd did not put back the window it had minimised")


def check_close(page):
    page.click(".titlebar button.close")
    page.wait_for_timeout(150)
    if page.eval_on_selector_all(".window", "(e) => e.length") != 0:
        fails("the close button left the window standing")
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 0:
        fails("a closed window kept its taskbar button, which is a button "
              "for a window that is not there")


def check_menu(page):
    """The menu button's menu is what the profile says it holds: the
    applications, a rule, Run, a rule, Logout.  A button that opened
    nothing would be the button that lies this panel is not allowed."""
    page.click("#menu-button")
    page.wait_for_timeout(150)
    if not page.is_visible("#menu-popup.open"):
        fails("the menu button opened nothing")
        return
    labels = page.eval_on_selector_all(
        "#menu-popup > .menu-item > span:not(.arrow)",
        "(e) => e.map((s) => s.textContent)")
    for wanted in ("Run...", "Logout"):
        if wanted not in labels:
            fails("the menu has no %r row, which the profile names"
                  % wanted)
    box = page.eval_on_selector("#menu-popup",
                                "(el) => el.getBoundingClientRect().bottom")
    panel_top = page.eval_on_selector(
        "#panel", "(el) => el.getBoundingClientRect().top")
    if box > panel_top + 1:
        fails("the menu runs down over the panel rather than opening "
              "upwards off it")

    # A category's own list has to appear under the pointer, or the arrow
    # on the row is an arrow to nowhere.
    page.hover("#menu-popup > .menu-item")
    page.wait_for_timeout(150)
    if not page.is_visible("#menu-popup .submenu"):
        fails("resting on a category opened no list, so its arrow points "
              "at nothing")
    page.keyboard.press("Escape")
    page.mouse.click(500, 300)
    page.wait_for_timeout(120)


def check_run_box(page):
    """Run has to RUN something, and to say so when it cannot."""
    page.click("#menu-button")
    page.wait_for_timeout(120)
    page.click("#menu-popup >> text=Run...")
    page.wait_for_timeout(150)
    if not page.is_visible(".dialog"):
        fails("the Run row opened no dialog")
        return
    page.fill(".dialog input", "nosuchthing")
    page.click(".dialog >> text=OK")
    page.wait_for_timeout(120)
    if "no such program" not in page.inner_text(".dialog .body"):
        fails("Run accepted a program that does not exist in silence")
    page.fill(".dialog input", "terminal")
    page.click(".dialog >> text=OK")
    page.wait_for_timeout(200)
    if page.eval_on_selector_all(".window", "(e) => e.length") != 1:
        fails("Run typed at with `terminal` opened no terminal")
    page.click(".titlebar button.close")
    page.wait_for_timeout(120)


def check_volume(page):
    """lxpanel's speaker opens a slider, and the icon reads the level."""
    before = page.get_attribute("#volume img", "src")
    page.click("#volume")
    page.wait_for_timeout(150)
    if not page.is_visible("#volume-popup.open"):
        fails("the speaker opened no slider")
        return
    page.click("#volume-popup .mute")
    page.wait_for_timeout(120)
    after = page.get_attribute("#volume img", "src")
    if after == before:
        fails("muting did not change the speaker's mark, so the panel "
              "cannot say whether it is muted")
    page.click("#volume-popup .mute")
    page.mouse.click(500, 300)
    page.wait_for_timeout(120)


def check_lock(page):
    """A lock you could click past would be a picture of a lock, so it has
    to cover the panel as well as the desktop."""
    page.click('[data-launch="lock"]')
    page.wait_for_timeout(200)
    if not page.is_visible("#lockscreen.open"):
        fails("the lock button locked nothing")
        return
    over = page.evaluate("""() => {
        const p = document.getElementById('panel')
            .getBoundingClientRect();
        const at = document.elementFromPoint(p.left + 8, p.top + 13);
        return at && at.closest('#lockscreen') !== null;
    }""")
    if not over:
        fails("the lock screen leaves the panel reachable underneath it")
    page.fill("#lockscreen input", "anything")
    page.keyboard.press("Enter")
    page.wait_for_timeout(150)
    if page.is_visible("#lockscreen.open"):
        fails("the lock screen would not unlock")


# pcmanfm's own LXDE profile: win_width=640, win_height=480, view_mode=icon,
# show_hidden=0, sort=name;ascending;
FILES_WIDTH = 640
FILES_HEIGHT = 480
FILES_MENUS = ["File", "Edit", "View", "Bookmarks", "Tools", "Help"]


def check_files(page):
    page.click('[data-launch="files"]')
    page.wait_for_timeout(250)
    if not page.is_visible(".files-body"):
        fails("the Files launcher opened no file manager")
        return

    size = page.eval_on_selector(".files-body", """(el) => {
        const r = el.closest('.window').getBoundingClientRect();
        return { w: Math.round(r.width), h: Math.round(r.height) };
    }""")
    if size["w"] != FILES_WIDTH or size["h"] != FILES_HEIGHT:
        fails("Files opened %dx%d where pcmanfm's profile says %dx%d"
              % (size["w"], size["h"], FILES_WIDTH, FILES_HEIGHT))

    menus = page.eval_on_selector_all(
        ".files-menubar > .m",
        "(e) => e.map((m) => m.firstChild.textContent.trim())")
    if menus != FILES_MENUS:
        fails("the menu bar runs %s rather than %s"
              % (" ".join(menus), " ".join(FILES_MENUS)))

    # Back is dead at the start of history, and Up is dead at the root.
    if not page.is_disabled('.files-toolbar button[title="Back"]'):
        fails("Back is live on a window that has been nowhere yet")

    # Entering a folder, and the location bar following it.
    page.dblclick('.files-entry:has-text("Documents")')
    page.wait_for_timeout(200)
    if page.input_value(".files-toolbar .location") != \
            "/home/user/Documents":
        fails("double-clicking Documents did not enter it")
    if page.eval_on_selector_all(".files-entry", "(e) => e.length") == 0:
        fails("Documents came up empty, so nothing was read")

    page.click('.files-toolbar button[title="Back"]')
    page.wait_for_timeout(200)
    if page.input_value(".files-toolbar .location") != "/home/user":
        fails("Back did not return to where the window came from")
    if page.is_disabled('.files-toolbar button[title="Forward"]'):
        fails("Forward is dead after a Back, so the history only runs one "
              "way")

    # A place navigates AND marks itself.
    page.click('.files-places .place:has-text("Filesystem")')
    page.wait_for_timeout(200)
    if page.input_value(".files-toolbar .location") != "/":
        fails("the Filesystem place went nowhere")
    if not page.is_visible('.files-places .place.current:has-text('
                           '"Filesystem")'):
        fails("the place that is open is not marked as the current one")
    if not page.is_disabled('.files-toolbar button[title="Up"]'):
        fails("Up is live at the root, where there is nothing above")

    # A file's size, reported when it is picked.
    page.click('.files-places .place:has-text("user")')
    page.wait_for_timeout(200)
    page.click('.files-entry:has-text("README.txt")')
    page.wait_for_timeout(150)
    said = page.inner_text(".files-status")
    if "README.txt" not in said or "KiB" not in said:
        fails("picking a file reported %r rather than its name and size"
              % said.strip())

    # show_hidden=0, until the View menu says otherwise.
    names = page.eval_on_selector_all(
        ".files-entry span", "(e) => e.map((s) => s.textContent)")
    if ".bashrc" in names:
        fails("a dotfile is listed with show_hidden=0")
    page.click('.files-menubar .m:has-text("View")')
    page.wait_for_timeout(120)
    page.click('.files-menubar .drop .row:has-text("Show Hidden")')
    page.wait_for_timeout(200)
    names = page.eval_on_selector_all(
        ".files-entry span", "(e) => e.map((s) => s.textContent)")
    if ".bashrc" not in names:
        fails("Show Hidden showed nothing hidden")

    # A path that is not there puts the old one back rather than blanking.
    page.fill(".files-toolbar .location", "/no/such/place")
    page.press(".files-toolbar .location", "Enter")
    page.wait_for_timeout(200)
    if page.input_value(".files-toolbar .location") != "/home/user":
        fails("typing a path that does not exist took the window "
              "somewhere")

    page.click(".titlebar button.close")
    page.wait_for_timeout(150)


def main():
    with sync_playwright() as play:
        browser = play.chromium.launch(
            executable_path=str(CHROME) if CHROME.exists() else None)
        page = browser.new_page(viewport={"width": 1024, "height": 768})
        page.goto((ROOT / "index.html").as_uri())
        # Under one 250ms tick: whatever is in the graph now was put there
        # by the priming at load and by nothing else.
        page.wait_for_timeout(120)
        green = check_cpu(page)

        page.wait_for_timeout(800)
        check_panel(page)
        check_order(page)
        check_clock(page)
        check_menu(page)
        check_run_box(page)
        check_volume(page)
        check_lock(page)
        check_files(page)
        check_terminal(page)
        check_wincmd(page)
        check_close(page)
        browser.close()

    if failures:
        print("%d check(s) failed" % len(failures), file=sys.stderr)
        return 1
    print("proof: a 26-pixel panel running %s, a clock at %%R, %d lit "
          "pixels in the cpu graph, a menu that opens upwards off the "
          "panel, a Run box that runs, a speaker that says when it is "
          "muted, a lock that covers the panel, and a terminal that "
          "answers what is typed at it, and a 640x480 file manager "
          "that navigates" % (" ".join(PLUGIN_ORDER), green))
    return 0


if __name__ == "__main__":
    sys.exit(main())
