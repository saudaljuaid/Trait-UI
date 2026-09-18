# Trait OS

## The look

X11, the way fvwm and twm leave it, drawn in sixteen colours.

![a session](build/session.png)

The root is a 50% weave - every other pixel, light grey against black.
X has drawn it that way since before there were wallpapers; it has a
period of two, so it costs nothing and has no resolution. There is no
wallpaper image any more.

No panel. You reach the menu by pressing the root, and it opens where
the pointer is. The panel still works and is one press of the Settings
row away - it is off, not gone.

The palette is the IBM sixteen, which is what `tools/render.c` paints
the console and the installer with. Red is the brand's `#9E1B1B` rather
than IBM's `#AA0000`, in all three. Nothing is shaded and no value is
computed from another: the title bar is one flat colour because a
gradient needs colours that are not in the palette to get from one end
to the other.

The font is thresholded at 128, so a glyph is one bit deep. A letter
with forty shades along its edge would be the only thing on the screen
not made of those sixteen.


A desktop shell in freestanding C, drawing on a linear framebuffer.

It is a copy of the Debian LXDE desktop — lxpanel's bar, pcmanfm, lxtask,
lxterminal, synaptic and Openbox's window frame — and every number in it
carries the Debian file it was read out of. Nothing here is measured off a
screenshot.

```sh
make -C tools run        # build, render frames into build/
```

No libc, no toolkit, no dependency but a pointer to some pixels.

## What it is a copy of

The panel is not drawn from a picture. It is laid out from LXDE's own
default panel profile, `/etc/xdg/lxpanel/LXDE/panels/panel`, shipped in
Debian's `lxde-common` 0.99.2-4:

```
Global { edge=bottom  height=26  fontcolor=#ffffff  background=1 }
space 2 | menu | launchbar | space 4 | wincmd | space 4 | pager |
space 4 | taskbar(expand=1) | cpu | volume | tray | dclock(%R) | launchbar
```

Every number on that line is honoured rather than approximated, and the
plugins appear in that order. The bar lays out from **both ends**, because
the profile does: left-packed up to the taskbar, right-packed back from
the clock, and `taskbar(expand=1)` takes the gap. That is why the clock
does not move when a window opens, and the panel's self-test asks exactly
that question.

| What | Where it came from |
| --- | --- |
| The bar's 26 background rows | `lxpanel-data` 0.11.1-2, `images/background.png` (1×26, tiled) |
| CPU widget 40×26, border 2, `#00FF00` | `lxpanel` 0.11.1, `plugins/cpu/cpu.c` — `gdk_color_parse("green")` is X11 green, so that value exactly |
| The Clearlooks palette | `gtk2-engines`, `Clearlooks/gtk-2.0/gtkrc` |
| File manager 640×480, icon view, `sort=name;ascending` | `lxde-common`, `/etc/xdg/pcmanfm/LXDE/pcmanfm.conf` |
| Desktop label ink and halo | the same profile — `desktop_fg=#ffffff`, `desktop_shadow=#000000` |
| Task list shows one desktop | the profile's `ShowAllDesks=0` |

The background strip's twenty-six bytes are in the source rather than a
hand-written ramp. The bright line at row 1, the step at row 12 and the
lift at row 25 are what make the bar read as lxpanel's, and a ramp is a
near-miss of something twenty-six pixels tall.

## Layout

```
include/trait/surface.h    a 32-bit surface and a clipped plot
include/trait/theme.h      the palette, as runtime state
include/trait/font.h       text as coverage — three sizes and a mono face
include/trait/input.h      pointer and key events
include/trait/window.h     Openbox's frame
include/trait/panel.h      lxpanel's bar
include/trait/menu.h       the applications menu
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
python3 tools/make-font.py <ttf> <px> src/trait_font_<px>.h trait_font_<px>
python3 tools/make-wallpaper.py [source.png] [out.bin] [WxH]
python3 tools/make-app-icons.py <dir> <out.h> --prefix <p> --sizes 16,48
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

- `assets/c-panel/`, `assets/c-files/`, `assets/icons/` — the real Debian
  icons, vendored byte for byte: `lxde-icon-theme` 0.5.1-2.1 (nuoveXT2,
  LGPL-3+) and `lxpanel-data` 0.11.1-2 (GPL-2+).
- `assets/logo/onion.png` and `assets/wallpaper/wallpaper.png` — the
  project owner's own artwork.

## What is not here

- No main loop on real hardware. `trait_shell_run()` takes its events
  from a callback, and the harness is the only caller so far; a keyboard
  and mouse would be another.
- No Properties dialog in the file manager — the menu row is drawn
  dimmed rather than faked.
- The Icon theme row in Settings changes nothing, because one icon set is
  installed and a chooser with one entry is a control with nothing to
  choose.
