#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render tools/wallpaper.html to assets/wallpaper/wallpaper.png.

    python3 tools/make-wallpaper.py [out.png] [width] [height]

The wallpaper is vector art plus a seeded scatter, drawn by the same
browser the screenshots come from.  Seeded, so the PNG this writes is the
same file every time rather than a lucky throw.
"""
import sys
from pathlib import Path

from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parent.parent
CHROME = Path("/opt/pw-browsers/chromium-1194/chrome-linux/chrome")


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1
               else "assets/wallpaper/wallpaper.png")
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 1920
    height = int(sys.argv[3]) if len(sys.argv) > 3 else 1080

    out.parent.mkdir(parents=True, exist_ok=True)
    with sync_playwright() as play:
        browser = play.chromium.launch(
            executable_path=str(CHROME) if CHROME.exists() else None)
        page = browser.new_page(viewport={"width": width, "height": height},
                                device_scale_factor=1)
        page.goto((ROOT / "tools" / "wallpaper.html").as_uri())
        page.wait_for_timeout(500)
        page.screenshot(path=str(out))
        browser.close()
    print("wrote %s (%dx%d)" % (out, width, height))


if __name__ == "__main__":
    main()
