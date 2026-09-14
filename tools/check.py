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


def check_desktop(page):
    """pcmanfm --desktop puts the home folder and the trash there, and
    both open what they name."""
    if page.eval_on_selector_all("#desktop-icons .desktop-icon",
                                 "(e) => e.length") != 2:
        fails("the desktop carries no icons")
        return
    #
    # THE LABELS HAVE TO BE READABLE ON THIS WALLPAPER.  The profile's
    # desktop_fg is #ffffff with a #000000 shadow, which is right for its
    # own dark wallpaper and invisible on this white one, so the pair is
    # inverted.  A copy so faithful you cannot read it is not a copy worth
    # having, and this is the assertion that keeps it that way.
    #
    ink = page.eval_on_selector(
        "#desktop-icons .desktop-icon span",
        "(el) => getComputedStyle(el).color")
    if ink.replace(" ", "") != "rgb(0,0,0)":
        fails("the desktop labels are %s over a white wallpaper" % ink)

    page.dblclick('.desktop-icon:has-text("Trash")')
    page.wait_for_timeout(300)
    if page.input_value(".files-toolbar .location") != "trash:///":
        fails("the Trash icon opened somewhere that is not the trash")
    page.click(".titlebar button.close")
    page.wait_for_timeout(150)


def check_desktop_menu(page):
    page.mouse.click(600, 300, button="right")
    page.wait_for_timeout(150)
    if not page.is_visible("#desktop-menu.open"):
        fails("a right click on the wallpaper dropped no menu")
        return
    page.click('#desktop-menu .row:has-text("Open in Terminal")')
    page.wait_for_timeout(250)
    if page.eval_on_selector_all(".window", "(e) => e.length") != 1:
        fails("the desktop menu's terminal row opened nothing")
        return

    # Maximise fills the work area - the screen less the panel - and
    # restores to where the window was.
    before = page.eval_on_selector(".window", """(el) => {
        const r = el.getBoundingClientRect();
        return { x: Math.round(r.x), y: Math.round(r.y),
                 w: Math.round(r.width) };
    }""")
    page.click(".titlebar button.maximize")
    page.wait_for_timeout(200)
    big = page.eval_on_selector(".window", """(el) => {
        const r = el.getBoundingClientRect();
        return { w: Math.round(r.width), h: Math.round(r.height) };
    }""")
    if big["w"] != 1024 or big["h"] != 768 - PANEL_HEIGHT:
        fails("a maximised window is %dx%d where the work area is %dx%d"
              % (big["w"], big["h"], 1024, 768 - PANEL_HEIGHT))
    page.click(".titlebar button.maximize")
    page.wait_for_timeout(200)
    after = page.eval_on_selector(".window", """(el) => {
        const r = el.getBoundingClientRect();
        return { x: Math.round(r.x), y: Math.round(r.y),
                 w: Math.round(r.width) };
    }""")
    if after != before:
        fails("restoring put the window at %s where it had been at %s"
              % (after, before))
    page.click(".titlebar button.close")
    page.wait_for_timeout(150)


SETTINGS_TABS = ["Widget", "Icon Theme", "Window Border", "Desktop",
                 "Panel", "Other"]


def check_settings(page):
    """A settings window whose switches do nothing is the largest possible
    version of a control that does not do what it is drawn as, so each
    check here picks a setting and then looks at the DESKTOP."""
    page.evaluate("() => launch('files')")
    page.evaluate("() => launch('settings')")
    page.wait_for_timeout(300)
    if not page.is_visible(".gtk-notebook"):
        fails("Settings opened no notebook")
        return
    tabs = page.eval_on_selector_all(
        ".gtk-tabs .tab", "(e) => e.map((t) => t.textContent)")
    if tabs != SETTINGS_TABS:
        fails("Settings runs %s rather than %s"
              % (" ".join(tabs), " ".join(SETTINGS_TABS)))

    # A widget theme reaches every window, not just the one it was set in.
    page.click(".gtk-list .item:text-is('Adwaita-dark')")
    page.wait_for_timeout(200)
    bg = page.eval_on_selector(".files-view",
                               "(el) => getComputedStyle(el).backgroundColor")
    if bg.replace(" ", "") == "rgb(255,255,255)":
        fails("picking a dark theme left the file manager behind it white,"
              " so the theme reached only the window it was set in")
    page.click(".gtk-list .item:text-is('Clearlooks')")
    page.wait_for_timeout(150)

    # The panel's height, from the panel's own page.
    page.click(".gtk-tabs .tab:text-is('Panel')")
    page.wait_for_timeout(120)
    page.select_option('select[data-setting="panel-height"]', "36")
    page.wait_for_timeout(200)
    tall = page.eval_on_selector(
        "#panel", "(el) => Math.round(el.getBoundingClientRect().height)")
    if tall != 36:
        fails("setting the panel to 36 left it %d tall" % tall)
    page.select_option('select[data-setting="panel-height"]', "26")
    page.wait_for_timeout(150)

    # The clock format, which is the profile's ClockFmt.
    page.select_option(
        'select[data-setting="clock-format"]', "%T")
    page.wait_for_timeout(200)
    said = page.inner_text("#clock")
    if len(said) != 8 or said.count(":") != 2:
        fails("the clock reads %r under %%T, which wants two colons"
              % said)
    page.select_option(
        'select[data-setting="clock-format"]', "%R")
    page.wait_for_timeout(150)

    # Moving the bar to the top has to take the work area with it.
    page.select_option(
        'select[data-setting="panel-edge"]', "top")
    page.wait_for_timeout(250)
    top = page.eval_on_selector(
        "#panel", "(el) => Math.round(el.getBoundingClientRect().top)")
    if top != 0:
        fails("the panel moved to the top and its top edge is at %d" % top)
    icons_top = page.eval_on_selector(
        "#desktop-icons",
        "(el) => Math.round(el.getBoundingClientRect().top)")
    if icons_top < PANEL_HEIGHT:
        fails("the panel moved to the top and the desktop icons stayed "
              "under it, at %d" % icons_top)
    page.select_option(
        'select[data-setting="panel-edge"]', "bottom")
    page.wait_for_timeout(200)

    page.evaluate("""() => windows.slice().forEach(closeWindow)""")
    page.wait_for_timeout(150)


def check_taskmgr(page):
    """lxtask lists what is running, and ending a task ends it."""
    page.evaluate("() => launch('terminal')")
    page.evaluate("() => launch('files')")
    page.evaluate("() => launch('taskmgr')")
    page.wait_for_timeout(400)
    rows = page.eval_on_selector_all(
        ".gtk-tree .line > div:first-child",
        "(e) => e.map((c) => c.textContent)")
    #
    # The COMMAND, not the title.  The first cut listed "user@phipia: ~"
    # and "user", which are what the title bars say - a task manager whose
    # command column carries window titles is a window list wearing a
    # task manager's headers.
    #
    for wanted in ("phipia-session", "lxterminal", "pcmanfm", "lxtask"):
        if wanted not in rows:
            fails("the task list has no %r row; it carries %s"
                  % (wanted, ", ".join(rows)))
    if len(rows) != 4:
        fails("the task list holds %d rows for one shell and three "
              "windows" % len(rows))

    # Ending a task ends it, and the row goes with it.
    page.click(".gtk-tree .line:has-text('lxterminal')")
    page.wait_for_timeout(120)
    page.click(".gtk-actions button:text-is('End Task')")
    page.wait_for_timeout(300)
    rows = page.eval_on_selector_all(
        ".gtk-tree .line > div:first-child",
        "(e) => e.map((c) => c.textContent)")
    if "lxterminal" in rows:
        fails("End Task left the task in the list")
    if page.eval_on_selector_all(".window:has(.terminal-body)",
                                 "(e) => e.length") != 0:
        fails("End Task took the row away and left the window standing")

    # Ending the session is refused OUT LOUD rather than ignored.
    page.click(".gtk-tree .line:has-text('phipia-session')")
    page.wait_for_timeout(120)
    page.click(".gtk-actions button:text-is('End Task')")
    page.wait_for_timeout(250)
    if not page.is_visible(".dialog:has-text('phipia-session')"):
        fails("ending the session was ignored rather than refused, so the "
              "button looks broken rather than guarded")
    page.evaluate("""() => {
        document.querySelectorAll('.dialog').forEach((d) => d.remove());
        windows.slice().forEach(closeWindow);
    }""")
    page.wait_for_timeout(150)


def check_synaptic(page):
    """A store whose Apply did nothing would be a shop window, so this
    installs a package and then looks at the MENU."""
    page.evaluate("() => launch('synaptic')")
    page.wait_for_timeout(300)
    if not page.is_visible(".synaptic"):
        fails("the package manager did not open")
        return
    if not page.is_disabled(".files-toolbar button:text-is('Apply')"):
        fails("Apply is live with nothing marked")

    # leafpad is not installed, so it is not in the menu.
    page.click("#menu-button")
    page.wait_for_timeout(150)
    before = page.eval_on_selector_all(
        "#menu-popup .submenu .menu-item span",
        "(e) => e.map((s) => s.textContent)")
    page.keyboard.press("Escape")
    page.mouse.click(900, 400)
    page.wait_for_timeout(120)
    if "Text Editor" in before:
        fails("a package that is not installed is already in the menu")

    #
    # ONE WINDOW, NOT THREE.  The first version of this called
    # launch('synaptic') again between steps, so a second package manager
    # opened on top of the first and every click after it landed on a
    # window underneath - which Playwright reported as another element
    # intercepting the press.
    #
    page.dblclick(".gtk-tree .line:has-text('leafpad')")
    page.wait_for_timeout(150)
    if page.is_disabled(".files-toolbar button:text-is('Apply')"):
        fails("marking a package left Apply dead")
    page.click(".files-toolbar button:text-is('Apply')")
    page.wait_for_timeout(300)

    page.click("#menu-button")
    page.wait_for_timeout(200)
    page.hover("#menu-popup .menu-item")
    page.wait_for_timeout(150)
    after = page.eval_on_selector_all(
        "#menu-popup .submenu .menu-item span",
        "(e) => e.map((s) => s.textContent)")
    page.mouse.click(900, 400)
    page.wait_for_timeout(120)
    if "Text Editor" not in after:
        fails("installing leafpad did not put it in the menu, so Apply "
              "is a button that marks and does not install")

    # And the application it installed actually runs.
    page.evaluate("() => launch('leafpad')")
    page.wait_for_timeout(250)
    if not page.is_visible(".leafpad-page"):
        fails("the installed text editor opened nothing")
    else:
        page.fill(".leafpad-page", "hello")
        page.wait_for_timeout(150)
        if "*" not in page.inner_text(".window:has(.leafpad-page) .title"):
            fails("typing into the editor left its title unmarked, so it "
                  "cannot say whether there is anything to save")
        page.evaluate("""() => {
            const win = windows.filter((w) => w.command === 'leafpad')[0];
            if (win) { closeWindow(win); }
        }""")
        page.wait_for_timeout(150)

    # An essential package refuses removal out loud.
    page.dblclick(".gtk-tree .line:has-text('lxpanel')")
    page.wait_for_timeout(200)
    if not page.is_visible(".dialog:has-text('lxpanel')"):
        fails("marking the panel for removal was allowed, or refused in "
              "silence")
    page.evaluate("""() => {
        document.querySelectorAll('.dialog').forEach((d) => d.remove());
        windows.slice().forEach(closeWindow);
    }""")
    page.wait_for_timeout(150)


def check_calculator(page):
    """It has to add up.  A keypad that printed digits and never answered
    would be a picture of a calculator."""
    page.evaluate("""() => {
        PACKAGES.filter((p) => p.name === 'galculator')[0].installed = true;
        rebuildMenu();
        launch('galculator');
    }""")
    page.wait_for_timeout(300)
    if not page.is_visible(".calc-pad"):
        fails("the calculator did not open")
        return
    for key in ["7", "+", "8", "="]:
        page.click(".calc-key:text-is('%s')" % key)
        page.wait_for_timeout(60)
    if page.inner_text(".calc-screen").strip() != "15":
        fails("7 + 8 came out as %r"
              % page.inner_text(".calc-screen").strip())
    page.click(".calc-key:text-is('C')")
    for key in ["5", "\u00F7", "0", "="]:
        page.click(".calc-key:text-is('%s')" % key)
        page.wait_for_timeout(60)
    if "divide" not in page.inner_text(".calc-screen"):
        fails("dividing by nothing printed %r rather than saying it "
              "cannot" % page.inner_text(".calc-screen").strip())
    #
    # PUT IT BACK.  This check installs galculator to open it, and the
    # catalogue is one object for the whole run - leaving it installed
    # turned a later check's "install it" into "remove it", and the
    # notification it looked for never came.
    #
    page.evaluate("""() => {
        windows.slice().forEach(closeWindow);
        PACKAGES.filter((p) => p.name === 'galculator')[0]
            .installed = false;
        rebuildMenu();
    }""")
    page.wait_for_timeout(150)


def check_tooltip(page):
    """GTK2's tip, in Clearlooks' own tooltip_bg_color - not the browser's
    dark rounded box, which would be the one thing on this desktop that is
    not this desktop."""
    page.hover("#wincmd")
    page.wait_for_timeout(750)
    if not page.is_visible("#tooltip.open"):
        fails("resting on a panel button raised no tip")
        return
    paint = page.eval_on_selector(
        "#tooltip", "(el) => getComputedStyle(el).backgroundColor")
    if paint.replace(" ", "") != "rgb(245,245,181)":
        fails("the tip is painted %s where Clearlooks says #F5F5B5"
              % paint)
    if page.get_attribute("#wincmd", "title") is not None:
        fails("the button still carries a title=, so the browser draws "
              "its own tip over ours")
    page.mouse.move(500, 300)
    page.wait_for_timeout(200)
    if page.is_visible("#tooltip.open"):
        fails("the tip stayed up after the pointer left")


def check_panel_menu(page):
    page.click("#panel", button="right", position={"x": 400, "y": 13})
    page.wait_for_timeout(200)
    if not page.is_visible("#panel-menu.open"):
        fails("a right click on the panel dropped no menu")
        return
    box = page.eval_on_selector(
        "#panel-menu", "(el) => el.getBoundingClientRect().bottom")
    top = page.eval_on_selector(
        "#panel", "(el) => el.getBoundingClientRect().top")
    if box > top + 1:
        fails("the panel's menu runs down over the panel")
    page.mouse.click(500, 300)
    page.wait_for_timeout(120)


def check_desktops(page):
    """The pager is a pager only if the desktops are real: a window
    belongs to one, switching hides the others, and the task list follows
    because the profile says ShowAllDesks=0."""
    page.evaluate("() => launch('terminal')")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all("#pager .desk .desk-win",
                                 "(e) => e.length") != 1:
        fails("the pager draws no rectangle for an open window, so a cell "
              "cannot say which desktop the work is on")
    page.click("#pager .desk:nth-child(2)")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all(".window:not([hidden])",
                                 "(e) => e.length") != 0:
        fails("switching desktop left the other desktop's window on "
              "screen")
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 0:
        fails("the task list still carries the other desktop's window, "
              "with ShowAllDesks=0 in the profile saying otherwise")
    # A window opened here belongs here.
    page.evaluate("() => launch('files')")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 1:
        fails("a window opened on the second desktop did not appear in "
              "its task list")
    page.click("#pager .desk:nth-child(1)")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all(".window:not([hidden]) .terminal-body",
                                 "(e) => e.length") != 1:
        fails("switching back did not bring the first desktop's window "
              "with it")
    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.wait_for_timeout(150)


def check_resize_and_switch(page):
    page.evaluate("() => launch('terminal')")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all(".window .grip", "(e) => e.length") != 8:
        fails("a window has no resize grips, so it can only ever be the "
              "size it opened at")
    before = page.eval_on_selector(".window", """(el) => {
        const r = el.getBoundingClientRect();
        return Math.round(r.width);
    }""")
    box = page.eval_on_selector(".window .grip.e", """(el) => {
        const r = el.getBoundingClientRect();
        return { x: r.x + r.width / 2, y: r.y + r.height / 2 };
    }""")
    page.mouse.move(box["x"], box["y"])
    page.mouse.down()
    page.mouse.move(box["x"] + 60, box["y"], steps=6)
    page.mouse.up()
    page.wait_for_timeout(200)
    after = page.eval_on_selector(
        ".window", "(el) => Math.round(el.getBoundingClientRect().width)")
    if after <= before:
        fails("dragging the right grip 60 pixels left the window %d wide, "
              "the same as before" % after)

    # Alt+Tab raises the one underneath, which is what cycling means.
    page.evaluate("() => launch('files')")
    page.wait_for_timeout(250)
    page.keyboard.down("Alt")
    page.keyboard.press("Tab")
    page.wait_for_timeout(200)
    if not page.is_visible("#switcher.open"):
        fails("Alt+Tab raised no switcher")
        page.keyboard.up("Alt")
        return
    page.keyboard.up("Alt")
    page.wait_for_timeout(250)
    raised = page.evaluate(
        "() => windows.filter((w) => w.active)[0].command")
    if raised != "lxterminal":
        fails("Alt+Tab raised %r rather than the window under the top one"
              % raised)
    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.wait_for_timeout(150)


def check_browser(page):
    """The panel's second launcher opened a grey rectangle with the words
    "Web Browser" on it.  It browses now, and a browser that cannot follow
    a link is not one."""
    page.evaluate("() => launch('browser')")
    page.wait_for_timeout(300)
    if not page.is_visible(".browser-view"):
        fails("the browser launcher opened no browser")
        return
    if page.input_value(".files-toolbar .location") != "about:home":
        fails("the browser opened somewhere other than its home page")
    page.click(".browser-view a:has-text('The panel')")
    page.wait_for_timeout(250)
    if page.input_value(".files-toolbar .location") != "about:panel":
        fails("following a link went nowhere")
    if "ShowAllDesks" in page.inner_text(".browser-view"):
        pass
    page.click('.files-toolbar button[title="Back"]')
    page.wait_for_timeout(200)
    if page.input_value(".files-toolbar .location") != "about:home":
        fails("Back did not return to the page the link was followed from")
    page.fill(".files-toolbar .location", "about:nowhere")
    page.press(".files-toolbar .location", "Enter")
    page.wait_for_timeout(200)
    if "Not found" not in page.inner_text(".browser-view"):
        fails("an address that is not here was answered with a blank page")
    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.wait_for_timeout(150)


def check_shortcuts(page):
    """A binding for something the desktop cannot do would be the
    keyboard's version of a button that lies, so each of these is checked
    against what it claims to start."""
    page.keyboard.press("Control+Alt+t")
    page.wait_for_timeout(300)
    if page.eval_on_selector_all(".terminal-body", "(e) => e.length") != 1:
        fails("Ctrl+Alt+T opened no terminal")
    page.keyboard.press("Alt+F4")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all(".window", "(e) => e.length") != 0:
        fails("Alt+F4 left the focused window standing")
    page.keyboard.press("Meta+e")
    page.wait_for_timeout(300)
    if page.eval_on_selector_all(".files-view", "(e) => e.length") != 1:
        fails("Super+E opened no file manager")
    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.wait_for_timeout(150)


def check_logout_banner(page):
    """lxsession-logout puts a banner across the top of its dialog, at
    352 by 125.  The mechanism is copied; the identity on it is not."""
    page.click('[data-launch="logout"]')
    page.wait_for_timeout(250)
    if not page.is_visible(".dialog .logout-banner"):
        fails("the logout dialog carries no banner")
        return
    src = page.get_attribute(".dialog .logout-banner", "src")
    if "lxpanel" in src:
        fails("the logout banner is LXDE's own, so this desktop's dialog "
              "carries another project's name")
    page.click(".dialog >> text=Cancel")
    page.wait_for_timeout(150)


def check_window_menu(page):
    """Openbox drops a menu on a right click of the title bar, and its
    rows are the window manager's rather than the application's.  Send to
    Desktop is the half of the workspace feature the pager cannot reach:
    the pager moves you, this moves the window."""
    page.evaluate("() => launch('terminal')")
    page.wait_for_timeout(250)
    page.click(".window .titlebar", button="right",
               position={"x": 60, "y": 10})
    page.wait_for_timeout(200)
    if not page.is_visible("#window-menu"):
        fails("a right click on the title bar dropped no window menu")
        return
    rows = page.eval_on_selector_all(
        "#window-menu .row", "(e) => e.map((r) => r.textContent)")
    if "Send to Desktop 2" not in rows:
        fails("the window menu cannot send the window anywhere; it "
              "carries %s" % ", ".join(rows))
    if not page.is_visible("#window-menu .row.off:text-is("
                           "'Send to Desktop 1')"):
        fails("the desktop the window is already on is offered as "
              "somewhere to send it")
    page.click("#window-menu .row:text-is('Send to Desktop 2')")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 0:
        fails("the window was sent to the other desktop and stayed in "
              "this desktop's task list")
    page.click("#pager .desk:nth-child(2)")
    page.wait_for_timeout(250)
    if page.eval_on_selector_all("#taskbar .task", "(e) => e.length") != 1:
        fails("the window was sent to desktop 2 and is not there")
    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.click("#pager .desk:nth-child(1)")
    page.wait_for_timeout(200)


def check_list_view(page):
    """view_mode=icon is the profile's default; Detailed List is
    pcmanfm's other view and the View menu switches to it."""
    page.evaluate("() => launch('files')")
    page.wait_for_timeout(300)
    if page.eval_on_selector_all(".files-entry", "(e) => e.length") == 0:
        fails("the file manager did not open in icon view")
    page.click(".files-menubar .m:text-is('View')")
    page.wait_for_timeout(120)
    page.click(".files-menubar .drop .row:text-is('Detailed List View')")
    page.wait_for_timeout(250)
    heads = page.eval_on_selector_all(
        ".files-tree .head > div", "(e) => e.map((c) => c.textContent)")
    if heads != ["Name", "Description", "Size", "Modified"]:
        fails("the list view runs %s rather than pcmanfm's columns"
              % ", ".join(heads))
    # A folder has no size, and an empty cell says so better than a nought.
    sizes = page.eval_on_selector_all(
        ".files-tree .line", """(e) => e.map((l) => [
            l.children[1].textContent, l.children[2].textContent])""")
    for kind, size in sizes:
        if kind == "Folder" and size.strip() != "":
            fails("a folder is given a size of %r" % size)
            break
    page.click(".files-menubar .m:text-is('View')")
    page.wait_for_timeout(120)
    page.click(".files-menubar .drop .row:text-is('Icon View')")
    page.wait_for_timeout(200)
    if page.eval_on_selector_all(".files-entry", "(e) => e.length") == 0:
        fails("switching back to icon view showed nothing")
    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.wait_for_timeout(150)


def check_item_menu(page):
    """pcmanfm's context menu on a file, and the two rows that change it.
    Rename and Delete have a filesystem of their own to change, so they
    change it - a row that quietly did nothing would be worse than none."""
    page.evaluate("() => launch('files')")
    page.wait_for_timeout(300)
    page.click(".files-entry:has-text('README.txt')", button="right")
    page.wait_for_timeout(250)
    rows = page.eval_on_selector_all(
        "#window-menu .row", "(e) => e.map((r) => r.textContent)")
    if rows != ["Open", "Rename", "Delete", "Properties"]:
        fails("the file menu carries %s" % ", ".join(rows))
        return

    # Properties says what it is, where it is and how big.
    page.click("#window-menu .row:text-is('Properties')")
    page.wait_for_timeout(250)
    facts = page.eval_on_selector_all(
        ".dialog .body b", "(e) => e.map((x) => x.textContent)")
    if facts != ["Name", "Type", "Location", "Size", "Modified"]:
        fails("Properties reports %s" % ", ".join(facts))
    page.click(".dialog .titlebar button.close")
    page.wait_for_timeout(150)

    # A folder is reported by what is IN it: a folder has no size.
    page.click(".files-entry:has-text('Documents')", button="right")
    page.wait_for_timeout(250)
    page.click("#window-menu .row:text-is('Properties')")
    page.wait_for_timeout(250)
    facts = page.eval_on_selector_all(
        ".dialog .body b", "(e) => e.map((x) => x.textContent)")
    if "Contents" not in facts:
        fails("a folder's properties report %s, so it is being given a "
              "size" % ", ".join(facts))
    page.click(".dialog .titlebar button.close")
    page.wait_for_timeout(150)

    #
    # Rename changes the name, and a folder keeps what is inside it.
    #
    # AND IT IS PUT BACK.  The filesystem is one object for the whole
    # session, so a check that renames Documents and walks away leaves
    # every later check looking for a folder that is not there - which is
    # exactly what happened, as a thirty-second wait for
    # .files-entry:has-text("Documents").
    #
    page.click(".files-entry:has-text('Documents')", button="right")
    page.wait_for_timeout(250)
    page.click("#window-menu .row:text-is('Rename')")
    page.wait_for_timeout(200)
    page.fill(".dialog input", "Papers")
    page.press(".dialog input", "Enter")
    page.wait_for_timeout(250)
    names = page.eval_on_selector_all(
        ".files-entry span", "(e) => e.map((x) => x.textContent)")
    if "Papers" not in names or "Documents" in names:
        fails("renaming a folder left the listing at %s"
              % ", ".join(names))
    page.dblclick(".files-entry:has-text('Papers')")
    page.wait_for_timeout(250)
    inside = page.eval_on_selector_all(
        ".files-entry span", "(e) => e.map((x) => x.textContent)")
    if "report.txt" not in inside:
        fails("the renamed folder came up empty, so the rename left its "
              "contents behind under the old name")

    # Delete, on a file inside it rather than on the folder itself.
    page.click(".files-entry:has-text('letter.txt')", button="right")
    page.wait_for_timeout(250)
    page.click("#window-menu .row:text-is('Delete')")
    page.wait_for_timeout(200)
    page.click(".dialog button:text-is('Delete')")
    page.wait_for_timeout(250)
    inside = page.eval_on_selector_all(
        ".files-entry span", "(e) => e.map((x) => x.textContent)")
    if "letter.txt" in inside:
        fails("Delete left the file in the listing")

    page.click('.files-toolbar button[title="Up"]')
    page.wait_for_timeout(250)
    page.click(".files-entry:has-text('Papers')", button="right")
    page.wait_for_timeout(250)
    page.click("#window-menu .row:text-is('Rename')")
    page.wait_for_timeout(200)
    page.fill(".dialog input", "Documents")
    page.press(".dialog input", "Enter")
    page.wait_for_timeout(250)

    page.evaluate("() => windows.slice().forEach(closeWindow)")
    page.wait_for_timeout(150)


EDITOR = ".window:has(.leafpad-page)"


def check_editor_files(page):
    """The editor and the file manager have to be looking at the same
    filesystem: a file saved in one appears in the other, at the size it
    actually is."""
    page.evaluate("""() => {
        PACKAGES.filter((p) => p.name === 'leafpad')[0].installed = true;
        rebuildMenu();
        launch('files');
    }""")
    page.wait_for_timeout(300)
    page.dblclick(".files-entry:has-text('README.txt')")
    page.wait_for_timeout(150)
    page.click(".files-entry:has-text('README.txt')", button="right")
    page.wait_for_timeout(250)
    page.click("#window-menu .row:text-is('Open')")
    page.wait_for_timeout(350)
    if not page.is_visible(".leafpad-page"):
        fails("Open on a text file started no editor")
        return
    #
    # OPENED AT THE FILE, not merely opened: an editor that came up empty
    # would have started the program and not opened the document.
    #
    if "Phipia" not in page.input_value(".leafpad-page"):
        fails("the editor opened empty, so it started the program without "
              "opening the file")

    # Saving somewhere new puts it in the file manager, at its real size.
    page.fill(".leafpad-page", "one two three\n")
    page.wait_for_timeout(120)
    #
    # SCOPED TO THE EDITOR'S OWN WINDOW.  The file manager underneath has
    # a File menu too, and ".files-menubar .m" matched its one first - the
    # click landed on a window that was not on top and Playwright sat
    # retrying it.
    #
    page.click(EDITOR + " .files-menubar .m:text-is('File')")
    page.wait_for_timeout(120)
    page.click(EDITOR + " .files-menubar .drop .row:text-is('Save As')")
    page.wait_for_timeout(200)
    page.fill(".dialog input", "/home/user/Documents/fresh.txt")
    page.press(".dialog input", "Enter")
    page.wait_for_timeout(250)
    if page.is_visible(".dialog"):
        fails("Save As would not take a path inside a folder that exists")
        return
    size = page.evaluate(
        "() => FS['/home/user/Documents'].files['fresh.txt']")
    if size != 14:
        fails("the file manager records fresh.txt as %r where the text "
              "written is 14 bytes" % size)

    # Saving somewhere that is not there says so rather than losing it.
    #
    # SCOPED TO THE EDITOR'S OWN WINDOW.  The file manager underneath has
    # a File menu too, and ".files-menubar .m" matched its one first - the
    # click landed on a window that was not on top and Playwright sat
    # retrying it.
    #
    page.click(EDITOR + " .files-menubar .m:text-is('File')")
    page.wait_for_timeout(120)
    page.click(EDITOR + " .files-menubar .drop .row:text-is('Save As')")
    page.wait_for_timeout(200)
    page.fill(".dialog input", "/no/such/folder/x.txt")
    page.press(".dialog input", "Enter")
    page.wait_for_timeout(250)
    #
    # THE DIALOG HAS TO STILL BE THERE.  Asking whether it SAYS "no such
    # folder" passes when the save succeeded and the dialog closed, which
    # is the failure this is meant to catch: the text is only reachable
    # while the box is open.
    #
    if not page.is_visible(".dialog"):
        fails("Save As took a path in a folder that does not exist and "
              "closed, so the file went somewhere nobody can see")
    elif "no such folder" not in page.inner_text(".dialog .body"):
        fails("Save As refused a bad folder without saying why")
    else:
        page.click(".dialog button:text-is('Cancel')")
    page.wait_for_timeout(150)

    page.evaluate("""() => {
        windows.slice().forEach(closeWindow);
        delete FS['/home/user/Documents'].files['fresh.txt'];
        PACKAGES.filter((p) => p.name === 'leafpad')[0].installed = false;
        rebuildMenu();
    }""")
    page.wait_for_timeout(150)


def check_notifications(page):
    """A bubble for something that actually happened, and nothing else.
    It must not cover the panel, which is the one thing a notification
    must never do."""
    #
    # Cleared first, not asserted empty: an earlier check installs a
    # package and its bubble is still up - a notification lasts several
    # seconds by design.  Whether the desktop announces itself unprompted
    # is asked once at the start of the run instead, where it means
    # something.
    #
    page.evaluate("""() => document.getElementById('notifications')
        .textContent = ''""")
    page.evaluate("() => launch('synaptic')")
    page.wait_for_timeout(300)
    page.dblclick(".gtk-tree .line:has-text('galculator')")
    page.wait_for_timeout(150)
    page.click(".files-toolbar button:text-is('Apply')")
    page.wait_for_timeout(350)
    if page.eval_on_selector_all(".notification", "(e) => e.length") != 1:
        fails("installing a package raised no notification")
        return
    said = page.inner_text(".notification")
    if "galculator" not in said:
        fails("the notification does not say what was installed: %r"
              % said.strip())
    box = page.eval_on_selector(
        ".notification", "(el) => el.getBoundingClientRect().bottom")
    panel_top = page.eval_on_selector(
        "#panel", "(el) => el.getBoundingClientRect().top")
    if box > panel_top:
        fails("the notification runs down over the panel, covering the "
              "clock and the tray")
    page.click(".notification")
    page.wait_for_timeout(350)
    if page.eval_on_selector_all(".notification", "(e) => e.length") != 0:
        fails("clicking a notification did not dismiss it")
    page.evaluate("""() => {
        windows.slice().forEach(closeWindow);
        PACKAGES.filter((p) => p.name === 'galculator')[0]
            .installed = false;
        rebuildMenu();
    }""")
    page.wait_for_timeout(150)


def check_create(page):
    """Create Folder and Create Blank File were dimmed while this window
    could not make one.  It can, so they are live - and a name already
    taken is refused rather than quietly overwriting what is there."""
    page.evaluate("() => launch('files')")
    page.wait_for_timeout(300)
    page.click(".files-menubar .m:text-is('File')")
    page.wait_for_timeout(120)
    page.click(".files-menubar .drop .row:text-is('Create Folder')")
    page.wait_for_timeout(200)
    page.fill(".dialog input", "Desktop")
    page.press(".dialog input", "Enter")
    page.wait_for_timeout(200)
    if not page.is_visible(".dialog"):
        fails("Create Folder took a name that is already in the folder")
    else:
        page.fill(".dialog input", "Scratch")
        page.press(".dialog input", "Enter")
        page.wait_for_timeout(250)
    names = page.eval_on_selector_all(
        ".files-entry span", "(e) => e.map((x) => x.textContent)")
    if "Scratch" not in names:
        fails("Create Folder made nothing; the listing is %s"
              % ", ".join(names))
    page.dblclick(".files-entry:has-text('Scratch')")
    page.wait_for_timeout(250)
    if page.input_value(".files-toolbar .location") != \
            "/home/user/Scratch":
        fails("the folder that was just made cannot be entered")
    page.evaluate("""() => {
        windows.slice().forEach(closeWindow);
        const home = FS['/home/user'];
        const at = home.dirs.indexOf('Scratch');
        if (at >= 0) { home.dirs.splice(at, 1); }
        delete FS['/home/user/Scratch'];
    }""")
    page.wait_for_timeout(150)


def check_panel_items(page):
    """lxpanel's Add / Remove Panel Items.  Unticking one takes it off the
    bar; the three that are how you reach anything at all are not offered
    and the dialog says why."""
    page.click("#panel", button="right", position={"x": 400, "y": 13})
    page.wait_for_timeout(200)
    page.click("#panel-menu .row:text-is('Add / Remove Panel Items')")
    page.wait_for_timeout(250)
    if not page.is_visible(".dialog"):
        fails("Add / Remove Panel Items opened nothing")
        return
    offered = page.eval_on_selector_all(
        '.dialog input[data-plugin]',
        "(e) => e.map((b) => b.dataset.plugin)")
    for trap in ("menu", "taskbar", "launchbar"):
        if trap in offered:
            fails("%r can be taken off the panel, which would leave no "
                  "way to put it back" % trap)
    page.uncheck('.dialog input[data-plugin="dclock"]')
    page.wait_for_timeout(200)
    if page.is_visible("#clock"):
        fails("unticking the clock left it on the bar")
    page.check('.dialog input[data-plugin="dclock"]')
    page.wait_for_timeout(200)
    if not page.is_visible("#clock"):
        fails("ticking the clock again did not put it back")
    page.click(".dialog .titlebar button.close")
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
        # Nothing has happened yet, so nothing may have been announced.
        if page.eval_on_selector_all(".notification",
                                     "(e) => e.length") != 0:
            fails("the desktop raised a notification at startup, before "
                  "anything had happened")

        page.wait_for_timeout(800)
        check_panel(page)
        check_order(page)
        check_clock(page)
        check_menu(page)
        check_run_box(page)
        check_volume(page)
        check_lock(page)
        check_create(page)
        check_item_menu(page)
        check_editor_files(page)
        check_window_menu(page)
        check_list_view(page)
        check_browser(page)
        check_shortcuts(page)
        check_logout_banner(page)
        check_tooltip(page)
        check_panel_menu(page)
        check_panel_items(page)
        check_desktops(page)
        check_resize_and_switch(page)
        check_desktop(page)
        check_desktop_menu(page)
        check_files(page)
        check_settings(page)
        check_taskmgr(page)
        check_synaptic(page)
        check_notifications(page)
        check_calculator(page)
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
          "that navigates, over a desktop whose icons open what they "
          "name, a Settings that reaches every window, and a task "
          "manager that ends what it lists, and a package manager "
          "whose Apply puts things in the menu, on two desktops a "
          "pager really switches between, reached by Openbox's own "
          "keys, saying so when something happens"
          % (" ".join(PLUGIN_ORDER), green))
    return 0


if __name__ == "__main__":
    sys.exit(main())
