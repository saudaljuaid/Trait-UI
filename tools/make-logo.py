#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Turn the OpenRFS mark into the ASCII the terminal prints.

    python3 tools/make-logo.py [mark.png] [out.h] [columns] [rows]

gfetch puts the mark beside the facts, the way every fetch has since
screenfetch, and a terminal has no pixels to put a PNG in - it has
characters.  So the mark is reduced here, once, and the C side prints
the lines.

THE SOURCE IS THE DRAWING, not the trimmed mark beside it.  The mark
that the website builds has had its white lifted to transparency, and
the fish's EYES are white - so in that file they are holes, and a
reduction of it comes out with a blank-faced fish.  The drawing still
has them, and which white is an eye and which is the paper around the
fish is answered by a flood fill from the border: paper is the white you
can reach from the edge, an eye is the white you cannot.

NOT A LUMINANCE RAMP.  The obvious ' .:-=+*#%@' ramp turns this mark to
mush: the body is one flat red, so every pixel of it lands on the same
rung and the fish comes out as a featureless blob.  What makes the
drawing legible is that it has FOUR THINGS IN IT - the paper, the black
outline, the red body and the pink tongue - so each of those gets a
character and the reduction is a classification rather than a ramp.

Characters are about twice as tall as they are wide, so the rows are
asked for separately rather than derived from the width; asking for a
square grid gives you a fish squashed to half its height.
"""
import sys
from pathlib import Path

from PIL import Image

PAPER = " "      # transparent: outside the drawing altogether
OUTLINE = "#"    # the black line the whole drawing is built from
BODY = "+"       # the red
TONGUE = ":"     # the pink
EYE = "o"        # the white of the eyes, which is NOT the paper

# What each class is INKED in, as a code the C side indexes with.
INK = {PAPER: "0", OUTLINE: "1", BODY: "2", TONGUE: "3", EYE: "4"}

# The share of a block a small feature needs to take it.  The eyes are
# not on this list: once the paper is told from the eye whites they are
# big enough to win their own blocks outright, and giving them a floor
# as well swallowed the pupils.  The tongue is a thin curl and does not.
FLOOR = ((TONGUE, 0.34),)

MAX_LINE = 64    # a fetch has to leave room for the facts beside it


def main():
    src = Path(sys.argv[1] if len(sys.argv) > 1
               else "assets/logo/openrfs-logo-source.jpeg")
    out = Path(sys.argv[2] if len(sys.argv) > 2 else "src/trait_logo.h")
    columns = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    rows = int(sys.argv[4]) if len(sys.argv) > 4 else 18

    image = Image.open(src).convert("RGB")
    pixels = image.load()
    wide, tall = image.size

    def white(x, y):
        r, g, b = pixels[x, y]
        return r > 218 and g > 218 and b > 218

    # OUTSIDE IS THE WHITE YOU CAN REACH FROM THE EDGE.  Everything else
    # that is white - the eyes, the teeth - is inside the drawing, and
    # the only thing that tells them apart is whether the paper connects
    # them to the border.
    outside = bytearray(wide * tall)
    queue = []
    for x in range(wide):
        for y in (0, tall - 1):
            if white(x, y) and not outside[y * wide + x]:
                outside[y * wide + x] = 1
                queue.append((x, y))
    for y in range(tall):
        for x in (0, wide - 1):
            if white(x, y) and not outside[y * wide + x]:
                outside[y * wide + x] = 1
                queue.append((x, y))
    while queue:
        x, y = queue.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < wide and 0 <= ny < tall \
                    and not outside[ny * wide + nx] and white(nx, ny):
                outside[ny * wide + nx] = 1
                queue.append((nx, ny))

    def classify(x, y):
        """Which of the five things in the drawing this pixel is."""
        r, g, b = pixels[x, y]
        light = (r * 299 + g * 587 + b * 114) // 1000
        if outside[y * wide + x]:
            return PAPER
        if light > 218:
            return EYE
        if light < 70:
            return OUTLINE
        if light > 165:
            return TONGUE
        return BODY

    # Trim to the ink, which is now everything that is not outside.
    left, right, top, bottom = wide, 0, tall, 0
    for y in range(tall):
        for x in range(wide):
            if not outside[y * wide + x]:
                left = min(left, x)
                right = max(right, x + 1)
                top = min(top, y)
                bottom = max(bottom, y + 1)
    if right <= left or bottom <= top:
        sys.exit("REFUSED: %s is all paper" % src.name)

    # PLURALITY OVER THE BLOCK, not the colour of its average - and a
    # floor under the two small things.
    #
    # Resampling first and classifying after is what loses features: an
    # eye is three cells across and every one of them averages white,
    # black pupil and dark outline into a mid grey that lands on the
    # body.  Classifying every source pixel and then asking what the
    # block is MOSTLY fixes that for anything the size of a cell.
    #
    # It is not enough on its own.  The white of an eye is a ring about
    # half a cell wide between a black pupil and a black rim, so it is
    # never the majority of anything and a strict plurality erases both
    # eyes - which is to say a face is made of features smaller than a
    # cell, and a rule that only reports majorities cannot draw one.  So
    # the eye and the tongue win their block at a share rather than a
    # majority, and the shares are here where they can be argued with.
    sums = {OUTLINE: [0, 0, 0, 0], BODY: [0, 0, 0, 0],
            TONGUE: [0, 0, 0, 0], EYE: [0, 0, 0, 0]}
    lines = []
    inks = []
    for y in range(rows):
        row = []
        ink_row = []
        for x in range(columns):
            tally = {}
            x0 = left + x * (right - left) // columns
            x1 = max(x0 + 1, left + (x + 1) * (right - left) // columns)
            y0 = top + y * (bottom - top) // rows
            y1 = max(y0 + 1, top + (y + 1) * (bottom - top) // rows)
            for sy in range(y0, y1):
                for sx in range(x0, x1):
                    cell = classify(sx, sy)
                    tally[cell] = tally.get(cell, 0) + 1
                    if cell in sums:
                        r, g, b = pixels[sx, sy]
                        sums[cell][0] += r
                        sums[cell][1] += g
                        sums[cell][2] += b
                        sums[cell][3] += 1
            total = sum(tally.values())
            cell = max(tally, key=lambda key: tally[key])
            for small, share in FLOOR:
                if tally.get(small, 0) >= total * share:
                    cell = small
                    break
            row.append(cell)
            ink_row.append(INK[cell])
        text_row = "".join(row).rstrip()
        lines.append(text_row)
        inks.append("".join(ink_row)[:len(text_row)])

    def mean(cell):
        total = sums[cell]
        if total[3] == 0:
            sys.exit("REFUSED: nothing in the drawing came out %s" % cell)
        return tuple(channel // total[3] for channel in total[:3])

    if not any(line.strip() for line in lines):
        sys.exit("REFUSED: the reduction came out blank")
    if max(len(line) for line in lines) > MAX_LINE:
        sys.exit("REFUSED: %d columns leaves no room for the facts"
                 % columns)

    text = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/*",
        " * GENERATED by tools/make-logo.py - do not edit.",
        " *",
        " * The OpenRFS mark at %u by %u characters, reduced from"
        % (columns, rows),
        " * %s.  See assets/logo/SOURCE.txt for" % src.name,
        " * whose drawing it is.",
        " */",
        "#ifndef TRAIT_LOGO_H",
        "#define TRAIT_LOGO_H",
        "",
        "#define TRAIT_LOGO_ROWS %uU" % len(lines),
        "#define TRAIT_LOGO_COLUMNS %uU" % columns,
        "",
        "/*",
        " * The colours are the DRAWING'S, averaged over every cell that",
        " * came out body, tongue or eye.  The outline is not here: it is",
        " * black in the drawing, and black on a black terminal is a line",
        " * you cannot see, so the C side inks it in the terminal's own",
        " * foreground and says so.",
        " */",
        "#define TRAIT_LOGO_BODY 0x%02X%02X%02XU" % mean(BODY),
        "#define TRAIT_LOGO_TONGUE 0x%02X%02X%02XU" % mean(TONGUE),
        "#define TRAIT_LOGO_EYE 0x%02X%02X%02XU" % mean(EYE),
        "",
        "static const char *const trait_logo[TRAIT_LOGO_ROWS] = {",
    ]
    for line in lines:
        text.append('    "%s",' % line.replace("\\", "\\\\")
                    .replace('"', '\\"'))
    text += [
        "};",
        "",
        "/* One code per character of the row above: 0 the terminal's own",
        " * ink, 1 the outline, 2 the body, 3 the tongue. */",
        "static const char *const trait_logo_ink[TRAIT_LOGO_ROWS] = {",
    ]
    for line in inks:
        text.append('    "%s",' % line)
    text += ["};", "", "#endif /* TRAIT_LOGO_H */"]
    out.write_text("\n".join(text) + "\n")
    print("wrote %s: %u rows, widest %u characters"
          % (out, len(lines), max(len(line) for line in lines)))


if __name__ == "__main__":
    main()
