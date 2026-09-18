# OpenRFS — the desktop

The system is OpenRFS. The C prefix below is still `trait_`, which is
what it was called while it was being written; renaming a thousand
symbols would change no pixel and break every patch in flight, so the
name lives in what the desktop SAYS - `uname`, the prompt, the window
titles, gfetch - and the symbols keep theirs.

## The look

X11, the way fvwm and twm leave it, drawn in sixteen colours.

![a session](build/session.png)

The root is one flat colour, which is what fvwm comes up on and what
OpenBSD therefore comes up on. There is no wallpaper image and nothing
here decodes one.

glxgears runs in a window, because glxgears is a program. Not a
screenshot of one: `tools/make-gears.py` takes the radii, tooth counts
and view angles out of Brian Paul's `glxgears.c` and projects them, and
`src/gears.c` fills the outlines at whatever size the window turns out
to be. Each gear is two of the sixteen colours, the face the light
reaches and the sides it does not, because flat shading is what is left
when the shades run out.

`gfetch` prints the mark and the facts, the way every fetch has since
screenfetch. The mark is the OpenRFS fish reduced to characters from the
owner's own drawing; the facts are read from the things they name - the
theme from the theme, the font from the generated face's metrics, the
package count from the package manager - so there is no figure in it
that cannot be traced.

The terminal is see-through, which is the pseudo-transparency an X
terminal has always had: no compositor and no alpha channel, just a
window that is drawn after what is behind it and mixes with what it
finds. One switch on the Desktop page makes it opaque.

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
| A flat root, an icon at the foot per iconified window, a titled menu | fvwm, which is what `startx` gives you on OpenBSD |
| The text: Misc-Fixed 6x13, 7x14, 9x18 and 8x16 | `xfonts-cyrillic`, the KOI8-R builds of the X11 bitmap faces — see `assets/fonts/SOURCE.txt` |
| The mark gfetch prints | the owner's own drawing — see `assets/logo/SOURCE.txt` |
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
include/trait/window.h     Openbox's frame
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
python3 tools/make-logo.py assets/logo/openrfs-mark-512.png src/trait_logo.h 32 15
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
