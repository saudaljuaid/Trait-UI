#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Draw the file manager's icons, in the palette, at the sizes it uses.

    python3 tools/make-icons.py [out_dir] [header]

These were Debian's - the nuoveXT2 set that LXDE ships, vendored under
assets/icons and recorded in its SOURCE.md. They are shaded, rounded and
antialiased, which was right while the desktop was LXDE and is wrong now
that everything else on the screen is one of sixteen flat colours: the
icons were the last thing left that was not.

Drawn, not scaled down from one master. A folder at 16 pixels is not a
folder at 48 with the detail removed; it is a different drawing of the
same idea, and the 16 here is laid out on its own grid so its edges land
on whole pixels.

Every colour is an index into the same sixteen tools/render.c paints
the console with.
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

BLACK, BLUE, GREEN, CYAN = 0x000000, 0x0000AA, 0x00AA00, 0x00AAAA
RED, MAGENTA, BROWN, GREY = 0x9E1B1B, 0xAA00AA, 0xAA5500, 0xAAAAAA
DARK, HIBLUE, HIGREEN, HICYAN = 0x555555, 0x5555FF, 0x55FF55, 0x55FFFF
HIRED, HIMAG, YELLOW, WHITE = 0xE8564B, 0xFF55FF, 0xFFFF55, 0xFFFFFF

SIZES = (16, 48)


def rgba(colour, alpha=255):
    return ((colour >> 16) & 0xFF, (colour >> 8) & 0xFF, colour & 0xFF,
            alpha)


class Pen:
    """Draws on a unit square, so one routine serves both sizes."""

    def __init__(self, size):
        self.size = size
        self.im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)
        # One pixel at 16, three at 48: an outline that scales with the
        # icon stops being an outline and becomes a border.
        self.line = 1 if size <= 24 else 2

    def px(self, u):
        return int(round(u * self.size))

    def box(self, x0, y0, x1, y1, fill, outline=BLACK):
        self.d.rectangle([self.px(x0), self.px(y0),
                          self.px(x1) - 1, self.px(y1) - 1],
                         fill=rgba(fill),
                         outline=rgba(outline) if outline is not None
                         else None,
                         width=self.line)

    def poly(self, points, fill, outline=BLACK):
        self.d.polygon([(self.px(x), self.px(y)) for x, y in points],
                       fill=rgba(fill),
                       outline=rgba(outline) if outline is not None
                       else None,
                       width=self.line)

    def line_h(self, x0, x1, y, colour):
        thickness = max(1, self.size // 16)
        self.d.rectangle([self.px(x0), self.px(y),
                          self.px(x1) - 1, self.px(y) + thickness - 1],
                         fill=rgba(colour))


def folder(p):
    """A tab and a body. The tab is what makes it a folder at 16px."""
    p.box(0.06, 0.24, 0.44, 0.36, YELLOW)
    p.box(0.06, 0.32, 0.94, 0.82, YELLOW)


def page(p, mark=None):
    p.poly([(0.20, 0.10), (0.66, 0.10), (0.80, 0.26),
            (0.80, 0.90), (0.20, 0.90)], WHITE)
    # The folded corner, which is the whole reason a page reads as a page.
    p.poly([(0.66, 0.10), (0.80, 0.26), (0.66, 0.26)], GREY)
    if mark is not None:
        mark(p)


def text_lines(p):
    for y in (0.40, 0.52, 0.64, 0.76):
        p.line_h(0.30, 0.70, y, DARK)


def image(p):
    p.box(0.10, 0.20, 0.90, 0.80, HICYAN)
    p.poly([(0.18, 0.72), (0.40, 0.42), (0.56, 0.62),
            (0.66, 0.50), (0.82, 0.72)], GREEN, outline=None)
    p.box(0.66, 0.28, 0.78, 0.40, YELLOW)


def audio(p):
    p.box(0.22, 0.62, 0.44, 0.84, HIMAG)
    p.box(0.58, 0.54, 0.80, 0.76, HIMAG)
    p.box(0.40, 0.14, 0.46, 0.68, HIMAG)
    p.box(0.76, 0.14, 0.82, 0.60, HIMAG)
    p.box(0.40, 0.14, 0.82, 0.24, HIMAG)


def executable(p):
    """A gear, because that is what an executable has always been."""
    p.box(0.16, 0.16, 0.84, 0.84, HIGREEN)
    p.box(0.38, 0.38, 0.62, 0.62, BLACK, outline=None)


def drive(p):
    p.box(0.08, 0.30, 0.92, 0.70, DARK)
    p.box(0.68, 0.44, 0.80, 0.56, HIGREEN)


def home(p):
    p.poly([(0.50, 0.10), (0.94, 0.48), (0.06, 0.48)], RED)
    p.box(0.18, 0.46, 0.82, 0.90, RED)
    p.box(0.40, 0.62, 0.60, 0.90, BLACK)


def desktop(p):
    p.box(0.08, 0.20, 0.92, 0.68, BLUE)
    p.box(0.18, 0.28, 0.82, 0.60, HIBLUE, outline=None)
    p.box(0.38, 0.70, 0.62, 0.80, DARK)
    p.box(0.24, 0.80, 0.76, 0.88, DARK)


def trash(p):
    p.box(0.20, 0.24, 0.80, 0.90, GREY)
    p.box(0.12, 0.14, 0.88, 0.26, DARK)
    p.box(0.40, 0.06, 0.60, 0.16, DARK)


ICONS = {
    "folder": folder,
    "text-x-generic": lambda p: page(p, text_lines),
    "application-x-executable": executable,
    "image-x-generic": image,
    "audio-x-generic": audio,
    "drive-harddisk": drive,
    "user-home": home,
    "user-desktop": desktop,
    "user-trash": trash,
}


def emit(header, art):
    """The same table shape make-app-icons.py wrote, filled differently.

    The 16 is not the 48 resampled. Both are drawn, so neither is the
    other one blurred - which is the whole reason there are two.
    """
    names = sorted(ICONS)
    out = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/*",
        " * GENERATED by tools/make-icons.py - do not edit.",
        " *",
        " * Drawn rather than vendored. Every pixel is transparent or one",
        " * of the sixteen colours tools/render.c paints with, and each",
        " * size is its own drawing rather than a resample of a bigger",
        " * one.",
        " *",
        " * Channels are packed 0x00RRGGBB.",
        " */",
        "#ifndef TRAIT_FILES_ART_H",
        "#define TRAIT_FILES_ART_H",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        f"#define TRAIT_FILES_ART_SIZES {len(SIZES)}U",
        "",
        f"static const uint32_t trait_files_art_size"
        f"[TRAIT_FILES_ART_SIZES] = {{",
        "    " + ", ".join(f"{s}U" for s in SIZES),
        "};",
        "",
    ]
    for name in names:
        ident = name.replace("-", "_")
        for size in SIZES:
            im = Image.open(art / f"{name}-{size}.png").convert("RGBA")
            pixels = list(im.get_flattened_data())
            out.append(f"/* {name} at {size}, drawn */")
            for plane, width in (("pixels", 8), ("alpha", 16)):
                out.append(
                    f"static const "
                    f"{'uint32_t' if plane == 'pixels' else 'uint8_t'} "
                    f"trait_files_art_{ident}_{size}_{plane}"
                    f"[{size} * {size}] = {{")
                row = []
                for r, g, b, a in pixels:
                    if plane == "pixels":
                        row.append(f"0x00{r:02X}{g:02X}{b:02X}U,")
                    else:
                        row.append(f"{a:4d},")
                    if len(row) == width:
                        out.append("    " + " ".join(row))
                        row = []
                if row:
                    out.append("    " + " ".join(row))
                out.append("};")
            out.append("")

    out += [
        "struct trait_files_art_entry {",
        "    const char *name;",
        "    const uint32_t *pixels[TRAIT_FILES_ART_SIZES];",
        "    const uint8_t *alpha[TRAIT_FILES_ART_SIZES];",
        "};",
        "",
        "static const struct trait_files_art_entry trait_files_art[] = {",
    ]
    for name in names:
        ident = name.replace("-", "_")
        px = ", ".join(f"trait_files_art_{ident}_{s}_pixels"
                       for s in SIZES)
        al = ", ".join(f"trait_files_art_{ident}_{s}_alpha" for s in SIZES)
        out.append(f'    {{ "{name}", {{ {px} }}, {{ {al} }} }},')
    out += [
        "};",
        "",
        "#define TRAIT_FILES_ART_COUNT "
        "(sizeof(trait_files_art) / sizeof(trait_files_art[0]))",
        "",
        "#endif",
    ]
    header.write_text("\n".join(out) + "\n")


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "assets/icons/drawn")
    out.mkdir(parents=True, exist_ok=True)
    palette = {rgba(c) for c in (BLACK, BLUE, GREEN, CYAN, RED, MAGENTA,
                                 BROWN, GREY, DARK, HIBLUE, HIGREEN,
                                 HICYAN, HIRED, HIMAG, YELLOW, WHITE)}
    problems = []

    for name, draw in ICONS.items():
        for size in SIZES:
            pen = Pen(size)
            draw(pen)
            # Every pixel is either transparent or one of the sixteen.
            # Pillow's outline width can leave a blended corner, and one
            # blended corner is the icon not being flat any more.
            flat = pen.im.copy()
            pixels = flat.load()
            for y in range(size):
                for x in range(size):
                    r, g, b, a = pixels[x, y]
                    if a < 128:
                        pixels[x, y] = (0, 0, 0, 0)
                        continue
                    if (r, g, b, 255) not in palette:
                        near = min(palette,
                                   key=lambda c: (c[0] - r) ** 2
                                   + (c[1] - g) ** 2 + (c[2] - b) ** 2)
                        pixels[x, y] = near
            flat.save(out / f"{name}-{size}.png")
        ink = flat.getchannel("A").getbbox()
        if ink is None:
            problems.append(f"{name} drew nothing")

    if problems:
        for problem in problems:
            print("REFUSED: " + problem)
        return 1

    header = Path(sys.argv[2] if len(sys.argv) > 2
                  else "src/trait_files_art.h")
    emit(header, out)
    print(f"wrote {len(ICONS) * len(SIZES)} icons to {out} and "
          f"{header}, {len(palette)} colours, no blending")
    return 0


if __name__ == "__main__":
    sys.exit(main())
