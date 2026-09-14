#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render the desktop in a real browser and write a PNG of it.

    python3 tools/shot.py build/desktop.png [width] [height]

The reference this is checked against is a 1024x768 screen, so that is the
default.  A screenshot is the only honest way to look at a copy of a
desktop: the HTML says what was meant and the PNG says what happened.
"""
import sys
from pathlib import Path

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parent.parent

# The container ships Chromium already; naming it here rather than letting
# Playwright go looking keeps this from trying to download one.
CHROME = Path("/opt/pw-browsers/chromium-1194/chrome-linux/chrome")


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "build/desktop.png")
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 1024
    height = int(sys.argv[3]) if len(sys.argv) > 3 else 768

    out.parent.mkdir(parents=True, exist_ok=True)
    with sync_playwright() as play:
        browser = play.chromium.launch(
            executable_path=str(CHROME) if CHROME.exists() else None)
        page = browser.new_page(viewport={"width": width, "height": height},
                                device_scale_factor=1)
        page.goto((ROOT / "index.html").as_uri())
        page.wait_for_timeout(1200)
        page.screenshot(path=str(out))
        browser.close()
    print("wrote %s (%dx%d)" % (out, width, height))


if __name__ == "__main__":
    main()
