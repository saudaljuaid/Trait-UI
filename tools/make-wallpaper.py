#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Turn the wallpaper into the raw RGB24 dump the C desktop loads.

    python3 tools/make-wallpaper.py [source.png] [out.bin] [WxH]

THERE IS NO IMAGE DECODER IN THE SHELL, on purpose: a desktop that needs
one to put up a background needs one in the kernel.  So this runs once,
ahead of time - scale to COVER the screen, crop the overhang evenly off
both sides, and write straight bytes, row-major, top to bottom, no
header.  The loader reads exactly width * height * 3 and nothing else.
"""
import sys
from pathlib import Path

from PIL import Image


def main():
    source = Path(sys.argv[1] if len(sys.argv) > 1
                  else "assets/wallpaper/wallpaper.png")
    out = Path(sys.argv[2] if len(sys.argv) > 2
               else "assets/wallpaper/wallpaper.bin")
    size = sys.argv[3] if len(sys.argv) > 3 else "1280x800"
    width, height = (int(v) for v in size.split("x"))

    image = Image.open(source).convert("RGB")
    scale = max(width / image.width, height / image.height)
    grown = image.resize((max(width, int(round(image.width * scale))),
                          max(height, int(round(image.height * scale)))),
                         Image.LANCZOS)
    left = (grown.width - width) // 2
    top = (grown.height - height) // 2
    cropped = grown.crop((left, top, left + width, top + height))

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(cropped.tobytes())
    print(f"wrote {out}: {width}x{height} RGB24 "
          f"({width * height * 3} bytes) from {source.name}")


if __name__ == "__main__":
    main()
