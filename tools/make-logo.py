#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Turn the OpenRFS mark into the coloured ASCII gfetch prints.

    python3 tools/make-logo.py [art.txt] [mark.png] [out.h]

TWO SOURCES, AND THEY DO DIFFERENT HALVES OF THE JOB.

The CHARACTERS come from assets/logo/openrfs-logo.txt, which is the
output of asciiart.eu's converter run over assets/logo/openrfs-mark-flat
.png - the settings are recorded in assets/logo/SOURCE.txt.  That tool
does the thing this file used to do badly: it has edge detection and a
proper gradient, and it draws a fish you can see the fins on.  What was
here before classified each block into one of five things and drew a
character per class, which is a reduction that cannot render an edge
running diagonally through a cell.

The COLOURS are still computed here, because the converter has none to
give: it emits text.  So the mark is classified on the same grid the
art is laid out on - paper, outline, body, tongue, eye - and each cell's
class picks its ink.  The inks themselves are the mean of every source
pixel that fell into each class, so the fish is drawn in the fish's own
two reds rather than in something that looks about right.

WHICH WHITE IS AN EYE.  Outside the drawing and inside the eyes are both
white, and only one of them is paper.  A flood fill from the border
answers it: paper is the white you can reach from the edge.
"""
import sys
from pathlib import Path

from PIL import Image

PAPER = " "
OUTLINE = "#"
BODY = "+"
TONGUE = ":"
EYE = "o"

INK = {PAPER: 0, OUTLINE: 1, BODY: 2, TONGUE: 3, EYE: 4}

# WHAT COLOUR A CELL IS: whichever of the four holds most of it, with a
# floor under the three small ones.
#
# The mean of the cell snapped to the nearest colour was tried first and
# is worse, which is not what you would guess.  A cell straddling the
# fish's edge averages red and white into a pale pink, pink is one of
# the four, and the fish comes out speckled with tongue along every
# boundary.  A vote has no such midpoint to fall into.
#
# The floors are here because the three small things are all thinner
# than a cell: the black line round the outside, the tongue, and the
# whites of the eyes each lose every vote they are in and the fish comes
# out one flat red.  The numbers are where they can be argued with -
# the tongue's is a quarter because at the size the mark is drawn at
# the best cell it has is 0.297 of one, and a third would lose it.
FLOOR = ((EYE, 0.30), (TONGUE, 0.25), (OUTLINE, 0.45))

MAX_LINE = 64    # a fetch has to leave room for the facts beside it


def main():
    art = Path(sys.argv[1] if len(sys.argv) > 1
               else "assets/logo/openrfs-logo.txt")
    src = Path(sys.argv[2] if len(sys.argv) > 2
               else "assets/logo/openrfs-mark-flat.png")
    out = Path(sys.argv[3] if len(sys.argv) > 3 else "src/trait_logo.h")

    # U+2588 is the character the converter's "Black and White" set
    # draws with, and it is not in a Misc-Fixed face - tools/make-font.py
    # synthesises it at the code after the last, because a solid
    # rectangle is not a typeface.  The art carries it as itself; the
    # table below carries it as that code.
    BLOCK_IN = "\u2588"
    BLOCK_OUT = chr(127)
    lines = [line.rstrip("\n").replace(BLOCK_IN, BLOCK_OUT) for line in
             art.read_text().rstrip("\n").split("\n")]
    if not lines or not any(line.strip() for line in lines):
        sys.exit("REFUSED: %s has no art in it" % art.name)
    rows = len(lines)
    columns = max(len(line) for line in lines)
    if columns > MAX_LINE:
        sys.exit("REFUSED: %d columns leaves no room for the facts"
                 % columns)

    image = Image.open(src).convert("RGB")
    pixels = image.load()
    wide, tall = image.size

    def white(x, y):
        r, g, b = pixels[x, y]
        return r > 218 and g > 218 and b > 218

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
        r, g, b = pixels[x, y]
        light = (r * 299 + g * 587 + b * 114) // 1000
        if outside[y * wide + x]:
            return PAPER
        if light > 218:
            return EYE
        if light < 70:
            return OUTLINE
        # THE TWO REDS, AND THE NUMBER BETWEEN THEM IS MEASURED.
        #
        # Over this mark the body's pixels sit at a luminance of about
        # 110 to 130 and the tongue's at about 150 to 170, so the line
        # between them is 140.  It was 165 for a while, which is inside
        # the tongue's own range: two thirds of the tongue was called
        # body, the fish came out with no tongue at all, and the only
        # cells that survived were the antialiased rim of a pupil
        # against a white eye - a grey as light as the tongue is.
        #
        # Hence the second test.  The tongue is PINK: its red channel
        # runs well clear of its other two, and a grey's does not.
        if light >= 140 and r - min(g, b) > 40:
            return TONGUE
        return BODY

    # THE SAME GRID THE ART IS ON.  The converter maps the whole image
    # to its grid, so this does too; anything else and the colours sit
    # beside the shapes rather than on them.
    sums = {OUTLINE: [0, 0, 0, 0], BODY: [0, 0, 0, 0],
            TONGUE: [0, 0, 0, 0], EYE: [0, 0, 0, 0]}
    # First the class of every cell, then a pass over the grid, then the
    # ink: the second pass needs to see a cell's neighbours, which the
    # first one has not finished making yet.
    grid = []
    for row in range(rows):
        line = []
        for column in range(columns):
            tally = {}
            x0 = column * wide // columns
            x1 = max(x0 + 1, (column + 1) * wide // columns)
            y0 = row * tall // rows
            y1 = max(y0 + 1, (row + 1) * tall // rows)
            for sy in range(y0, y1):
                for sx in range(x0, x1):
                    cell = classify(sx, sy)
                    tally[cell] = tally.get(cell, 0) + 1
                    r, g, b = pixels[sx, sy]
                    if cell in sums:
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
            line.append(cell)
        grid.append(line)

    # THERE IS NO BLACK IN IT, and that is the whole difference between
    # a fish and a mess at this size.
    #
    # The drawing is built out of black: the outside edge, the mouth,
    # the fins, the rims of the eyes.  A cell here is thirty-odd pixels
    # of the mark, so every one of those lines is thinner than the cell
    # it lands in - and a black cell on a black terminal is a bite taken
    # out of the fish wherever a line runs through it.  Keeping only the
    # ones that bound something was tried, and so was keeping only the
    # ones that make a pupil; both leave a mass of black around the eyes,
    # which is where this drawing's lines are thickest.
    #
    # So the line is not drawn, with ONE exception: black that borders
    # the white of an eye is a pupil, and an eye without one is a white
    # block.  Everywhere else the line becomes the body it runs through -
    # including round the outside, where it is not needed at all, since
    # the fish is red and the terminal is black and a silhouette does
    # not have to draw its own edge.
    for row in range(rows):
        for column in range(columns):
            if grid[row][column] != OUTLINE:
                continue
            pupil = False
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                y, x = row + dy, column + dx
                if 0 <= y < rows and 0 <= x < columns \
                        and grid[y][x] == EYE:
                    pupil = True
            if not pupil:
                grid[row][column] = BODY

    inks = []
    for row in range(rows):
        codes = []
        for column in range(columns):
            cell = grid[row][column]
            drawn = lines[row][column] if column < len(lines[row]) else " "
            if drawn == " ":
                # Nothing is drawn there, so nothing is inked there.
                codes.append(0)
            elif cell == PAPER:
                # The converter drew a block over pixels the classifier
                # calls paper - the edge running diagonally through the
                # cell.  The art is right about there being something
                # there, so it is drawn, and it is drawn as the body.
                codes.append(INK[BODY])
            else:
                codes.append(INK[cell])
        inks.append("".join(str(code) for code in
                            codes[:len(lines[row])]))

    def mean(cell):
        total = sums[cell]
        if total[3] == 0:
            sys.exit("REFUSED: nothing in the drawing came out %s" % cell)
        return tuple(channel // total[3] for channel in total[:3])

    text = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/*",
        " * GENERATED by tools/make-logo.py - do not edit.",
        " *",
        " * The characters are %s, which is asciiart.eu's" % art.name,
        " * converter run over %s." % src.name,
        " * The colours are computed from that same image on the same",
        " * grid.  assets/logo/SOURCE.txt records the settings and whose",
        " * drawing it is.",
        " */",
        "#ifndef TRAIT_LOGO_H",
        "#define TRAIT_LOGO_H",
        "",
        "#define TRAIT_LOGO_ROWS %uU" % rows,
        "#define TRAIT_LOGO_COLUMNS %uU" % columns,
        "",
        "/*",
        " * The colours are the DRAWING'S, averaged over every pixel",
        " * that came out body, tongue or eye.  The outline is not here:",
        " * it is black in the drawing, and black on a black terminal is",
        " * a line you cannot see, so the C side inks it in the",
        " * terminal's own foreground and says so.",
        " */",
        "#define TRAIT_LOGO_BODY 0x%02X%02X%02XU" % mean(BODY),
        "#define TRAIT_LOGO_TONGUE 0x%02X%02X%02XU" % mean(TONGUE),
        "#define TRAIT_LOGO_EYE 0x%02X%02X%02XU" % mean(EYE),
        "",
        "static const char *const trait_logo[TRAIT_LOGO_ROWS] = {",
    ]
    for line in lines:
        text.append('    "%s",' % "".join(
            "\\177" if ch == BLOCK_OUT else
            ("\\\\" if ch == "\\" else ('\\"' if ch == '"' else ch))
            for ch in line))
    text += [
        "};",
        "",
        "/* One code per character of the row above: 0 the terminal's own",
        " * ink, 1 the outline, 2 the body, 3 the tongue, 4 an eye. */",
        "static const char *const trait_logo_ink[TRAIT_LOGO_ROWS] = {",
    ]
    for line in inks:
        text.append('    "%s",' % line)
    text += ["};", "", "#endif /* TRAIT_LOGO_H */"]
    out.write_text("\n".join(text) + "\n")
    print("wrote %s: %u rows, widest %u characters, inked from %s"
          % (out, rows, columns, src.name))


if __name__ == "__main__":
    main()
