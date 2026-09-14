#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The desktop's wallpaper: one colour, one light, and nothing else.

The desktop this project copies shipped a dark blue-grey field with a soft
light low behind the distribution's mark.  The mark is the part that is not
wanted here, so what is left is the field and the light - which is as
minimal as a wallpaper gets while still being a wallpaper rather than a
flat fill.

    python3 tools/make-wallpaper.py assets/wallpaper/wallpaper.png

Everything below is arithmetic on two colours, so the file it writes is
this project's own and owes nothing to anybody's artwork.
"""
import struct
import sys
import zlib
import binascii
from pathlib import Path

WIDTH = 1920
HEIGHT = 1200

# The two ends of the field, dark at the corners and lifted in the middle.
EDGE = (0x1C, 0x1F, 0x27)
CORE = (0x3E, 0x46, 0x59)

# Where the light sits and how far it reaches, as fractions of the frame.
LIGHT_X = 0.5
LIGHT_Y = 0.46
LIGHT_REACH = 0.72


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def smooth(t):
    """Smoothstep, so the light has no visible edge where it runs out."""
    t = 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)
    return t * t * (3.0 - 2.0 * t)


def main(out):
    cx = WIDTH * LIGHT_X
    cy = HEIGHT * LIGHT_Y
    reach = max(WIDTH, HEIGHT) * LIGHT_REACH
    raw = bytearray()
    for y in range(HEIGHT):
        raw.append(0)
        dy = (y - cy)
        for x in range(WIDTH):
            dx = (x - cx)
            # An ellipse rather than a circle: the frame is wider than it
            # is tall and a round light in it reads as a spotlight.
            d = ((dx * dx) / 1.6 + dy * dy) ** 0.5
            t = smooth(1.0 - d / reach)
            raw += bytes(mix(EDGE, CORE, t))
    ihdr = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)

    def chunk(tag, body):
        return (struct.pack(">I", len(body)) + tag + body +
                struct.pack(">I", binascii.crc32(tag + body) & 0xFFFFFFFF))

    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_bytes(
        b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) +
        chunk(b"IDAT", zlib.compress(bytes(raw), 9)) +
        chunk(b"IEND", b""))
    print("wrote %s (%dx%d)" % (out, WIDTH, HEIGHT))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1
         else "assets/wallpaper/wallpaper.png")
