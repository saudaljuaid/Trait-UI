#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render tools/banner.html to assets/logo/logout-banner.png.

    python3 tools/make-banner.py [out.png] [scale]

lxsession-logout's banner is 352x125 and the dialog sizes it with
width:100%, so this renders at 2x by default and lets the browser scale
it down - a banner that is soft on the one dialog that shows a wordmark
is the kind of thing nobody fixes later.
"""
import sys
from pathlib import Path

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parent.parent
CHROME = Path("/opt/pw-browsers/chromium-1194/chrome-linux/chrome")
WIDTH, HEIGHT = 352, 125


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1
               else "assets/logo/logout-banner.png")
    scale = int(sys.argv[2]) if len(sys.argv) > 2 else 2

    out.parent.mkdir(parents=True, exist_ok=True)
    with sync_playwright() as play:
        browser = play.chromium.launch(
            executable_path=str(CHROME) if CHROME.exists() else None)
        page = browser.new_page(viewport={"width": WIDTH, "height": HEIGHT},
                                device_scale_factor=scale)
        page.goto((ROOT / "tools" / "banner.html").as_uri())
        page.wait_for_timeout(400)
        page.screenshot(path=str(out))
        browser.close()
    print("wrote %s (%dx%d)" % (out, WIDTH * scale, HEIGHT * scale))


if __name__ == "__main__":
    main()
