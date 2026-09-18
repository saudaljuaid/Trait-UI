# OpenRFS — the desktop

The system is OpenRFS. The C prefix below is still `trait_`, which is
what it was called while it was being written; renaming a thousand
symbols would change no pixel and break every patch in flight, so the
name lives in what the desktop SAYS - `uname`, the prompt, the window
titles, gfetch - and the symbols keep theirs.

## The look

X11, the way fvwm and twm leave it, drawn in sixteen colours.

![a session](build/session.png)

The root is the X root **weave**. Before any window manager runs, the X
server fills its own root window with a four-by-four tile it builds in
`MakeRootTile()` — `_back_msb[4] = { 0x11, 0x44, 0x22, 0x88 }`, one white
pixel every fourth over black. It is the oldest thing on an X screen, it
is what a bare OpenBSD session comes up on because nothing paints over
it, and it is not a wallpaper: there is no image here and nothing decodes
one. It was a flat grey for a while, which is what the weave looks like
from across a room — and which a see-through window cannot prove anything
against, since a flat colour mixed with black is a flat colour.

The frames are fvwm's, at fvwm's own sizes, out of the file a fresh
OpenBSD install reads — `xenocara/app/fvwm/sample.fvwmrc/system.fvwmrc`:

```
Style "*"  BorderWidth 7, HandleWidth 7
Style "*"  Color #bebebe/darkred
HilightColor #bebebe blue
MenuStyle #4d4d4d #bebebe #e7e7e7 -adobe-times-bold-r-*-*-12-* fvwm
```

Seven pixels of border, a focused window in blue and every other one in
dark red, a centred title, square buttons flush at the end of the bar,
and a line across each corner where a drag stops changing one dimension
and starts changing two. The border is the resize grip — `RESIZE_GRIP` in
`shell.c` **is** `TRAIT_BORDER`, so what you drag is what you see; a
modern frame hides that region and fvwm never did.

Every bevel is two colours computed from its ground, and they are not
chosen: `tools/make-relief.py` is fvwm's `GetHilite()` and `GetShadow()`
from `libs/ColorUtils.c`, transcribed and run over each ground at build
time. It runs in Python rather than in C because the medium-brightness
branch converts to HLS in double precision and this desktop is built
`-msoft-float`: a fixed-point re-derivation would be this project's
arithmetic rather than fvwm's.

glxgears runs in a window, because glxgears is a program. Not a
screenshot of one: `tools/make-gears.py` takes the radii, tooth counts
and view angles out of Brian Paul's `glxgears.c` and projects them, and
`src/gears.c` fills the outlines at whatever size the window turns out
to be. Each gear is two of the sixteen colours, the face the light
reaches and the sides it does not, because flat shading is what is left
when the shades run out.

`gfetch` prints the mark and the facts, the way every fetch has since
screenfetch. The mark is the OpenRFS fish, and it is CAPTURED rather
than computed: characters and colours together, out of
[coddy.tech's ASCII art generator](https://coddy.tech/tools/ascii-art-generator)
run over the owner's drawing in image mode with Color on, Invert off and
the width at exactly 45. `assets/logo/SOURCE.txt` records every setting
so it can be redone.

It used to be built in two halves and the halves were the problem. The
characters came from asciiart.eu's "Black and White" set, and reading
that page's own source settles what that set is:

```js
blockelement: "\u2588"
```

One character, plus a space for the gradient's other end. It is a
**silhouette**, thresholded at pure white — so every feature the mark
has had to be put back afterwards by classifying the drawing into paper,
outline, body, tongue and eye, and inking each cell by a vote. That is
why there was a classifier, why it carried five hand-tuned thresholds,
and why two of those were measured wrong at different times with nothing
to catch it.

The generator that replaced it does both halves at once — a character
per cell out of a real gradient, a colour per cell out of the source
pixels — so there is nothing left here to classify and nothing to
average. `tools/make-logo.py` reduces the capture's 251 distinct colours
to a palette of fifteen, because a terminal cell carries one byte of
attribute and not three, and emits the characters, the indices and the
palette. The two the fish is a face because of — the white of an eye and
the blue inside it — come through as cells like any other.

The harness checks it the only way that means anything: it counts how
many of the fifteen reach the framebuffer and fails unless all of them
do. A mark that arrives with fifteen colours and leaves with four has
been flattened somewhere between the table and the glass.

The facts are read from the things they name — the theme from the theme,
the font from the generated face's metrics, the package count from the
package manager — so there is no figure in it that cannot be traced.

The terminal is see-through, which is the pseudo-transparency an X
terminal has always had: no compositor and no alpha channel, just a
window that is drawn after what is behind it and mixes with what it
finds. One switch on the Desktop page makes it opaque.

For a long time it was not see-through at all, and the reason is worth
writing down. The frame used to fill the whole window — border, title
bar and client together — and leave the application to paint over the
inside, which every application does as its first act. So by the time
the terminal read the framebuffer back, what was under it was not the
desktop but its own frame: one flat colour, everywhere, whatever the
window happened to be sitting on. Mixing black into a flat colour gives
you another flat colour. The terminal came out **lighter**, which is not
the same thing as transparent, and the check that was supposed to catch
this only asked that the ground be neither black nor the root — which a
third flat colour satisfies perfectly. The frame now draws four strips
around the client and leaves what is inside alone, and the check reads
the ground in two places, one over bare root and one over a window, and
fails unless they differ.

Its foreground follows its ground, twice over. There is no compositor to
hold the text opaque while the ground goes clear, so a sheer terminal
writes in white and an opaque one in lxterminal's own `#D3D7CF`. That
alone runs out at about 140 of 255: white cannot go any whiter, and now
that the window really does show what is behind it, a pale window behind
it lifts the ground to meet the ink. What buys the rest is a halo — the
text drawn once in black a pixel down and right before it is drawn in
white — which is what pcmanfm's desktop labels do over a wallpaper that
is light in one place and dark in another. It is the same problem and
the same answer, and it is what lets the ground sit at 90 of 255 and
stay readable over black, over the root and over a white window alike.

The launcher is dmenu's. A strip across the top of the screen: a prompt,
what you have typed, and the programs it matches laid out along the rest
of it with one selected. Typing narrows them, the arrows move along,
Tab copies the selection into the input, Return runs it. Every number in
it is dmenu's own — the strip is the font's height plus two, the padding
either side of a cell is half the font's height, the prompt is drawn in
the selected colours and the input in the normal ones, and a `>` says
the list did not end where the strip did.

It replaces a box in the middle of the screen that asked for a name and
could not tell you what there was, so the only way to use it was to
already know.

No panel, no dock, no tray, no clock. You reach the menu by pressing the
root and it opens where the pointer is; you reach a window by clicking it
or with Alt+Tab; what is running is in the Task Manager. The bar used to
be here and be switched off, which meant a thousand lines that nothing
reached. It is gone, and the Debian artwork that dressed it went with it.

The root comes up bare. ~/Desktop is one switch away in Settings.

Put a window down and it lands on the root as an icon, at the foot of
the screen, the way fvwm iconifies. That is where the taskbar went: a
minimised window that lives only in Alt+Tab is a window you have to
remember you have. The menu wears a title bar, which fvwm's does too.

No menu bars either. The file manager had File, Edit, View, Bookmarks,
Tools and Help across the top and none of them was hit-tested; so did the
Task Manager; so did a places pane, three sortable column headings that
sorted nothing, and a path drawn as a text entry you could not type in.
Fourteen pictures of controls, and they are gone. What is left answers a
press: the entries, the Task Manager's headings, Apply in the package
manager (which had a function behind it and no way to reach it until
now), and the four keyboard shortcuts the Settings page lists — three of
which were also just text until this pass.

The palette is the IBM sixteen, which is what `tools/render.c` paints
the console and the installer with. Red is the brand's `#9E1B1B` rather
than IBM's `#AA0000`, in all three. Nothing is shaded and no value is
computed from another: the title bar is one flat colour because a
gradient needs colours that are not in the palette to get from one end
to the other.

The font is Misc-Fixed — 6x13, 7x14 and 9x18, with 8x16 in the terminal —
which is a bitmap font, so a glyph is one bit deep by construction.

The icons are the gentoo file manager's, drawn by Johan Hanson in 1998 and
copied byte for byte. They are 16 by 15 and there is no larger version,
so the file manager opens in a list, which is what gentoo was.


A desktop shell in freestanding C, drawing on a linear framebuffer.

It started as a copy of the Debian LXDE desktop and the windows are still
that copy — pcmanfm, lxtask, lxterminal, synaptic and Openbox's frame —
with every number carrying the Debian file it was read out of. What sits
around them is not: the root is glxgears, the menu is twm's, and there is
no bar at all.

```sh
make -C tools run        # build, render frames into build/
```

No libc, no toolkit, no dependency but a pointer to some pixels.

## What it is a copy of

Nothing here is measured off a screenshot. Every value is read out of the
file that defines it, and the table says which file.

| What | Where it came from |
| --- | --- |
| The three gears, their radii, tooth counts and view angles | Mesa demos, `src/xdemos/glxgears.c`, Brian Paul — see `assets/gears/SOURCE.txt` |
| The frame: `BorderWidth 7`, `#bebebe/darkred`, `HilightColor #bebebe blue`, the menu's `#4d4d4d #bebebe #e7e7e7` | `xenocara/app/fvwm/sample.fvwmrc/system.fvwmrc`, which is what `startx` reads on OpenBSD |
| Every bevel's two relief colours | fvwm `libs/ColorUtils.c`, `GetHilite()` and `GetShadow()` — see `tools/make-relief.py` |
| The root weave | the X server itself, `dix/window.c`, `MakeRootTile()` |
| An icon at the foot per iconified window, a titled menu | fvwm |
| The launcher's strip, padding, colours, `>` and keys | `dmenu.c` and `dmenu.1`, suckless — `bh = drw->fonts->h + 2`, `lrpad = drw->fonts->h`, prompt in `SchemeSel` |
| The text: Misc-Fixed 6x13, 7x14, 9x18 and 8x16 | `xfonts-cyrillic`, the KOI8-R builds of the X11 bitmap faces — see `assets/fonts/SOURCE.txt` |
| The mark gfetch prints | the owner's own drawing, through coddy.tech's generator at width 45 with colour on — see `assets/logo/SOURCE.txt` |
| The icons | `gentoo` 0.20.7-4, `usr/share/gentoo/icons/`, Johan Hanson 1998 — see `assets/icons/gentoo/SOURCE.txt` |
| The Clearlooks palette | `gtk2-engines`, `Clearlooks/gtk-2.0/gtkrc` |
| File manager 640×480, `sort=name;ascending` | `lxde-common`, `/etc/xdg/pcmanfm/LXDE/pcmanfm.conf` |
| Desktop label ink and halo | the same profile — `desktop_fg=#ffffff`, `desktop_shadow=#000000` |

## Layout

```
include/trait/surface.h    a 32-bit surface and a clipped plot
include/trait/theme.h      the palette, as runtime state
include/trait/font.h       text as coverage — three sizes and a mono face
include/trait/input.h      pointer and key events
include/trait/window.h     fvwm's frame
include/trait/gears.h      glxgears, in a window
include/trait/menu.h       the root menu
include/trait/files.h      pcmanfm
include/trait/taskmgr.h    lxtask
include/trait/settings.h   the GTK notebook
include/trait/packages.h   synaptic
include/trait/terminal.h   lxterminal
include/trait/shell.h      what owns the windows and routes the events
```

Every module below the shell draws and models; none knows another exists.
The shell is the one place that knows there is more than one window, which
is why it is the one place that can say what a click on a given pixel
means.

## Two sets of flags, and the difference matters

The shell's own sources build **freestanding**:

```
-std=c11 -ffreestanding -nostdlib -msoft-float
-Wall -Wextra -Werror -Wpedantic -Wshadow -Wundef
-Wstrict-prototypes -Wmissing-prototypes
```

That is not decoration. It is what they will be built as on the metal, and
a warning that only appears there is a warning nobody sees. There is no
libc down there, so the numbers are formatted by hand and the PNG encoder
in `tools/` is the project's own. The harness — `tools/main.c` — is an
ordinary host program and is built as one; the Makefile keeps the two sets
of flags apart on purpose.

## Nothing is rasterised at runtime

There is no font server and no image decoder behind a framebuffer, so both
happen ahead of time:

```sh
python3 tools/make-font.py <font.pcf.gz> src/trait_font_<WxH>.h trait_font_<WxH>
python3 tools/make-gears.py src/trait_gears_art.h
python3 tools/make-logo.py assets/logo/openrfs-logo-2026.json src/trait_logo.h
python3 tools/make-relief.py
python3 tools/make-icons.py assets/icons/gentoo src/trait_files_art.h
```

A glyph the font does not carry draws nothing rather than a box, because a
box is a picture of a missing character pretending to be a character.

## The rule

**Nothing is drawn as a control that does not do the thing it is drawn
as.** A row that cannot act is dimmed, not live and silent; a setting this
shell cannot carry out is not offered; a name the Run box cannot run keeps
the box open and says why, because closing on it would look exactly like
having run it.

## The checks are the point

Each module carries a self-test that asks the question it would actually
fail at. The harness then drives the shell with real events from screen
coordinates — never by asking a module where its button is and then
calling the function behind it, which proves the function and not the
button. The run **fails the build** if any of twelve proofs does not hold.

A sample of what they caught:

| Failure | What was actually wrong |
| --- | --- |
| `Alt+Tab committed to the window that already had focus` | it enumerated by slot, not most-recently-used |
| `End Task left the row` | slot 0 mapped to pid 1 — the session, which refuses to be ended |
| `the menu button did not close it` | the opener was treated as a dismiss, so one press closed and reopened |
| `the rename did not take (the menu was about Downloads)` | the harness clicked a fixed offset that overshot an 18px list row |
| `terminal self-test failed` | the old check asserted the buffer caps at one screen — it described the *absence* of scrollback |

Three of those five were faults in the checks or the harness rather than
in the code. That is what a check is for: a test that only ever agrees
with the code is a second copy of the code.

## Assets

Every file under `assets/` names its origin and licence in a `SOURCE.txt`
beside it.

- `assets/icons/gentoo/` — the gentoo file manager's icons, vendored byte
  for byte: `gentoo` 0.20.7-4, Johan Hanson 1998, GPL-2+.
- `assets/logo/openrfs-logo-2026.jpeg` — the project owner's own drawing
  of the fish, and the only piece of artwork in this repository.

The onion mark, the `trait.svg` menu button, the drawn file-manager icon
and the wallpaper were all deleted: every one of them was the project
under an earlier name, and nothing had read any of them for some time —
the root has been the X server's own weave since the fvwm pass, so
there was nothing left to decode a wallpaper for.

## What is not here

- No main loop on real hardware. `trait_shell_run()` takes its events
  from a callback, and the harness is the only caller so far; a keyboard
  and mouse would be another.
- No Properties dialog in the file manager — the menu row is drawn
  dimmed rather than faked.
- The Icon theme row in Settings changes nothing, because one icon set is
  installed and a chooser with one entry is a control with nothing to
  choose.
