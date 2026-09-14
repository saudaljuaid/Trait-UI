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
        check_terminal(page)
        check_wincmd(page)
        check_close(page)
        browser.close()

    if failures:
        print("%d check(s) failed" % len(failures), file=sys.stderr)
        return 1
    print("proof: a 26-pixel panel running %s, a clock at %%R, %d lit "
          "pixels in the cpu graph, and a terminal that answers what is "
          "typed at it" % (" ".join(PLUGIN_ORDER), green))
    return 0


if __name__ == "__main__":
    sys.exit(main())
