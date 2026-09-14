# Trait OS

A copy of the Debian LXDE desktop — the panel, its plugins and its icons —
with this project's own mark on the menu button and a minimal wallpaper in
place of the distribution's.

Open `index.html`. Nothing is built and nothing is installed.

## What it is a copy of

The panel is not drawn from a screenshot. It is laid out from LXDE's own
default panel profile, `/etc/xdg/lxpanel/LXDE/panels/panel`, shipped in
Debian's `lxde-common`:

```
Global { edge=bottom  height=26  fontcolor=#ffffff  background=1 }
space 2 | menu | launchbar(pcmanfm, x-www-browser) | space 4 | wincmd |
space 4 | pager | space 4 | taskbar(expand=1) | cpu | volume | tray |
dclock(%R) | launchbar(screenlock)
```

Every number on that line is honoured rather than approximated, and the
plugins appear in that order. The clock is `%R` — hours and minutes,
24-hour — because that is what the profile asks for.

The CPU plugin is `lxpanel/plugins/cpu/cpu.c`'s own geometry: a 40 by 26
widget, `BORDER_SIZE` 2, a black ground, and `gdk_color_parse("green")`,
which is X11 green and therefore `#00FF00` exactly.

## The assets are the real ones

The icons, the panel's background gradient and the window-manager mark are
the actual files out of Debian's packages, vendored byte for byte, with
their licences recorded in `assets/icons/SOURCE.txt`:

- `lxde-icon-theme` 0.5.1-2.1 (nuoveXT2) — LGPL-3+
- `lxde-common` 0.99.2-4 — GPL-2+
- `lxpanel-data` 0.11.1-2 — GPL-2+

The panel background is a 1×26 strip tiled across, which is how lxpanel
draws it; it is used as the file rather than reproduced as a CSS gradient,
because a hand-written ramp would be a near-miss of something only 26
pixels tall.

Two things here are this project's own and say so in the `SOURCE.txt`
beside them: the mark on the menu button (`assets/logo/trait.svg`) and
the wallpaper, which `tools/make-wallpaper.py` generates from two colours
and one soft light.

**The mark sits low-contrast on the dark panel** — it is a mid grey on a
near-black bar, where LXDE's own mark is light grey. That is the mark's
own colour and it has not been repainted here; lightening it is one fill
value in `assets/logo/trait.svg`.

## Every button on it does what it is drawn as

The menu button opens the menu the profile describes — the applications
by freedesktop category, a rule and `Run...` — upwards off the panel,
because the panel is at the foot of the screen. A category with nothing in
it is not drawn; LXDE does not draw one either.

`Run...` runs this desktop's own programs and says `no such program` for
anything else. The speaker opens a slider and its mark changes when you
mute it. The padlock covers the whole screen, panel included — a lock you
could click past would be a picture of a lock.

**The profile's last row and last launcher are `logout`, and neither is
here.** A page has no session to end. What stood there closed every window
and called that logging out, which is a control doing something other than
what it is named — the same fault as a button that does nothing, wearing a
more convincing coat. The rule applies to the rows copied from the profile
too, or it is a rule about new work only. `tools/check.py` asserts the
absence, so it cannot come back one launcher at a time.

## The desktop

`pcmanfm --desktop` draws it, and the profile says how the names are
painted — `desktop_fg=#ffffff` with `desktop_shadow=#000000`. That is
right for the dark wallpaper the profile ships with and **invisible on
this one**, which is white, so the pair is inverted rather than dropped:
black ink with a white shadow, the same mechanism the other way up. A
copy so faithful you cannot read it is not a copy worth having, and
`check.py` asserts the labels are black so it stays that way.

The home folder and the trash sit there and both open what they name. A
right click drops pcmanfm's desktop menu, with the rows it cannot carry
out dimmed rather than left out — the same rule the file manager's menus
follow.

Windows maximise from the title-bar button or a double click on the bar,
to the **work area** — the screen less the panel, which is the space
`setpartialstrut=1` in the panel's profile reserves — and restore to
exactly where they were.

## The browser

The panel's second launcher is `lxde-x-www-browser.desktop`, and it used
to open a grey rectangle with the words *Web Browser* on it — the last
control on this desktop that did not do what it was drawn as.

There is no network behind this desktop, so it browses the pages this
desktop ships: local documents about the thing you are looking at, with
Back, Forward, Reload and Home working and links between them that
follow. An address that is not here gets a *Not found* page, the way a
browser does. A browser of local documents is a browser; a rectangle with
a name on it is not.

## Keys

LXDE binds these in Openbox's `rc.xml`, and each one is a thing this
desktop can actually do — a binding for something it cannot would be the
keyboard's version of a button that lies.

| | |
|---|---|
| `Super+E` | the file manager |
| `Super+R` | the Run box |
| `Super+D` | show the desktop |
| `Ctrl+Alt+T` | a terminal |
| `Ctrl+Alt+L` | lock the screen |
| `Alt+F4` | close the focused window |
| `Alt+Tab` | cycle windows |

## The window manager

Openbox's shape, and Openbox's behaviour:

- **Eight resize grips** round every frame, because Openbox resizes from
  any edge and any corner. They are invisible; the cursor is the whole
  affordance, which is what a border of that era gives you.
- **Alt+Tab** cycles the way Openbox does — holding Alt keeps the list up,
  each Tab moves down it, letting go raises what is picked. It lists every
  desktop's windows, and raising one that is elsewhere goes there.
- **Maximise** to the work area, from the button or a double click on the
  bar. A maximised window has no grips and does not drag.
- **A window menu** on a right click of the title bar, carrying the rows
  the window manager owns rather than the application: minimise,
  maximise, **Send to Desktop**, close. Send to Desktop is the half of
  the workspace feature the pager cannot reach — the pager moves you,
  this moves the window. The desktop a window is already on is dimmed
  rather than dropped, so the list keeps its length.

## Two desktops that are real

The pager is a pager only because the desktops behind it are. A window
belongs to the one it was opened on; switching shows that desktop's
windows and hides the rest; and the **task list follows**, because the
panel's own profile says `ShowAllDesks=0` — the taskbar lists the desktop
you are on and not the others.

Each cell draws one rectangle per window, scaled from where the window
actually is, which is what `lxpanel`'s pager draws. Two cells that merely
changed colour could not tell you which desktop your work is on, which is
the entire point of a pager.

## The tray

It holds what applications put in it, and nothing else. It was empty, and
that was honest — a tray with a decoration in it is not a tray.

The first thing with a reason to be there is the package manager, which
shows an icon while marks are waiting to be applied: a state the machine
is really in, that outlives the window, and that you would otherwise have
to reopen the window to find out about. Clicking it raises that window.
The icon goes when the marks do.

## Notifications

`notification-daemon`'s bubble, which is what Debian's LXDE shows for a
libnotify message: an icon, a bold summary, a body, and a timeout. It
sits at the **top right**, not over the panel — a notification that
covered the clock and the tray would be doing the one thing a
notification must not.

Only something that actually happened raises one: a package installed or
removed, a task ended. A desktop that announced its own existence would
be interrupting you to say nothing, and a check asserts none is up at
startup.

## Tooltips

GTK2's, in Clearlooks' own colours — `tooltip_bg_color:#F5F5B5` with
`tooltip_fg_color:#000000`, the pale yellow note every GTK2 desktop of
that era has, after GTK's own 500ms delay. The text is moved out of
`title=` the first time an element is pointed at, so the browser does not
draw its dark rounded box on top of ours — that box would be the one
thing on this desktop that is not this desktop.

A right click on the panel drops `lxpanel`'s own menu, above the bar, and
**Add / Remove Panel Items** takes a plugin off the bar and puts it back.
The menu, the launchers and the task list are not offered: between them
they are how you reach anything at all, and a panel you could strip to
nothing would be a panel you could not get back. The dialog says that
rather than silently leaving three rows out.

## The Files app

`pcmanfm`'s shape, from `pcmanfm`'s own LXDE profile
(`/etc/xdg/pcmanfm/LXDE/pcmanfm.conf`):

```
win_width=640  win_height=480  view_mode=icon  show_hidden=0
sort=name;ascending;
```

so the window opens at 640×480 in icon view with dotfiles hidden and
folders before files. Menu bar, a toolbar of Back / Forward / Up / Reload
/ Home with the location bar after them, a Places side pane, the icon
view, and a two-field status bar — and all of it works. `View` switches
to **Detailed List**, pcmanfm's other view, with its own columns: Name,
Description, Size, Modified. A folder's cell under Size is empty, because
a folder's size is not a number a file manager reports. Back and Forward
keep a real history, Up is dead at the root, a Places row navigates and
marks itself, and typing a path that is not there puts the old one back.

The chrome is **Clearlooks**, which is what Debian's LXDE draws GTK2
with, and its palette is the one line of its own `gtkrc`
(`gtk2-engines`, `usr/share/themes/Clearlooks/gtk-2.0/gtkrc`):

```
base_color:#ffffff  fg_color:#000000  text_color:#1A1A1A  bg_color:#EDECEB
selected_bg_color:#86ABD9  selected_fg_color:#ffffff
```

The three shades it derives — `bg[PRELIGHT] = shade(1.02, bg)`,
`bg[ACTIVE] = shade(0.9, bg)`, `base[PRELIGHT] = shade(0.95, bg)` — are
computed rather than guessed.

The status bar's right-hand field is **not** free space. `pcmanfm` puts
the filesystem's free space there; there is no filesystem under this one,
so it reports what is actually in the folder. The first cut printed
`Free space: 3.2 GiB (Total: 7.4 GiB)` under a comment claiming it was a
figure the desktop could stand behind — it was invented, and a status bar
that makes up a number is worse than one that leaves the field out.

`File` creates a folder or a blank file, and refuses a name that is
already in use rather than quietly overwriting what is there.

A right click on a file or folder drops pcmanfm's context menu — Open,
Rename, Delete, Properties — and the two rows that change things change
them: this window has a filesystem of its own. Renaming a folder carries
its contents with it; a rename that left them behind would be a new empty
folder wearing the old name. Properties reports a **folder** by what is
in it rather than by a size, because a folder's size is not a number a
file manager can give you.

**The editor and the file manager share one filesystem.** A `.txt` file
opened from pcmanfm opens *at* that file rather than starting an empty
editor; `Save As` writes it back, and the file appears in pcmanfm at the
size it actually is. Saving into a folder that does not exist says so
rather than losing the file somewhere nobody can see.

Its mark, `assets/logo/files.svg`, is this project's own: a drawer with
fanned papers, drawn in the same geometry as the Trait OS mark — the same
shear, the same rounding — and grey rather than a bright folder, because
the desktop it sits on is the grey Debian one.

## Settings

LXDE has no control panel: look and feel is `lxappearance`, the desktop's
own settings are `pcmanfm`'s, and the bar's are `lxpanel`'s. They are one
notebook here, a page per program — Widget, Icon Theme, Window Border,
Desktop, Panel, Other.

**Every control on it changes something.** A settings window whose
switches do nothing is the largest possible version of a control that
does not do what it is drawn as, so a setting this desktop cannot carry
out is not offered. The Icon Theme page lists one theme because one is
what is installed; `lxappearance` lists what is on the machine and so
does this. The Other page says outright why there is no mouse-cursor
theme and no toolbar style.

Picking a widget theme repaints **every window at once**, not just the
one it was set in. Moving the panel to the top takes the work area with
it, so the desktop icons and any maximised window move too.

## The Task Manager

`lxtask`'s shape: a menu bar, a summary of the machine, and one list with
Command, User, CPU%, RSS and PID. `File → End Task` ends the selected
task; ending `trait-session` is refused out loud, because it is the
desktop itself.

**What it reports is measured or it is not reported.** CPU% comes from
the same probe the panel's monitor uses, shared between what is running.
RSS comes from `performance.memory` where the browser offers it and reads
`-` where it does not. PID is a number this desktop hands out when a
window opens — which is what a process id is — and it is not a number
from any operating system.

## The package manager

Debian's is Synaptic, and this is its shape: a toolbar of Reload / Mark
All Upgrades / Apply, a section pane, a package list with a status
column, a description pane, and a status bar counting what is listed and
what is marked.

**It actually installs.** Double-clicking a package marks it, `Apply`
carries the marks out, and an installed package appears in the menu and
can be run — so installing `leafpad` puts Text Editor in Accessories and
removing it takes it out. `Run...` refuses a package that is in the
catalogue but not installed, the way a shell would.

The catalogue is therefore exactly as long as the list of applications
this desktop can really provide. There is no point listing Firefox when
installing it would install nothing. The two that can be installed are
`leafpad` — a menu bar and a white page, which is all Leafpad is — and
`galculator`, a calculator that adds up and says *cannot divide by zero*
rather than printing `Infinity`.

An essential package refuses removal and says why: taking the panel away
would leave a desktop with no way to put it back.

## The terminal

`assets/icons/nuoveXT2/terminal.png` is nuoveXT2's own mark — a black
screen with a `>_` prompt on it, which is the era's terminal icon rather
than a modern flat one, and it came out of the package unaltered.

The window it opens matches: `#000000`, a light grey foreground, a
monospace face and a block cursor, in an Openbox frame — a one-pixel
border, the label at the left of the title bar and the buttons at the
right. It answers `help`, `echo`, `date`, `uname`, `whoami`, `pwd`, `ls`
and `clear`, and says `command not found` for anything else. A prompt
that swallowed what you typed would be a picture of a terminal.

There is no maximise button on the frame, because there is nothing behind
one. Nothing here is drawn as a control that does not do what it is drawn
as.

## Integrating Trait OS into Phipia OS

Phipia OS is the other half of this: a desktop shell written in freestanding
C11 against Sapote's kernel interfaces — `src/kernel/*.c`, `include/sapote/*.h`,
built with `-ffreestanding -nostdlib -msoft-float`, and previewed by a host
harness that writes PNG frames.

**Start with what is not true.** Trait OS is HTML, CSS and JavaScript. Phipia
OS has no browser, no DOM and no allocator you would want to hand a layout
engine; there is no build flag that makes `index.html` run on it. Anyone who
tells you to "just embed a webview" is describing a different operating
system. What Trait OS is *for* Phipia OS is a **reference implementation you
can run** — a desktop that answers the mouse, so every measurement you port is
taken off something working rather than off a screenshot.

There are three routes, and they compose.

### 1. Use it as the spec, and copy the numbers

Every layout constant here was read out of a Debian package rather than
guessed, and they are the same constants Phipia OS needs. Take them from the
source, not from this page:

| What | Where it is here | Where it came from |
| --- | --- | --- |
| Panel height, edge, font colour | `css/desktop.css`, `--panel-height` | `lxde-common` → `/etc/xdg/lxpanel/LXDE/panels/panel` |
| Plugin order | `index.html`, `#panel` children | the same profile |
| CPU widget 40×26, border 2, `#00FF00` | `js/desktop.js`, the CPU probe | `lxpanel/plugins/cpu/cpu.c` |
| Clearlooks palette | `css/files.css`, the `:root` vars | `gtk2-engines` → `Clearlooks/gtk-2.0/gtkrc` |
| File-manager window 640×480, icon view | `js/files.js` header comment | `/etc/xdg/pcmanfm/LXDE/pcmanfm.conf` |

In Phipia OS these become `#define`s beside the ones already in
`src/kernel/taskbar.c` and `src/kernel/desktop.c`. Port the *numbers*; do not
port the CSS, because Phipia OS composites in Q16.16 fixed point and a CSS
gradient has no meaning there.

### 2. Feed the assets straight into Phipia OS's own generators

This is the part that is genuinely mechanical. Phipia OS already turns images
into C at build time, and Trait OS's own art is in the formats those tools
take. From a Phipia OS checkout, with this repository beside it:

```sh
UI=../phipia-ui          # or wherever you cloned Trait OS

# The marks - trait.svg and files.svg - as the planes the shell
# draws.  The FILLED generator, not the glyph one: see below.
python3 tools/make-shell-icons.py "$UI/assets/logo" \
    src/kernel/trait_icons.h trait

# The vendored Debian icons are PNGs, not SVGs, so they go through
# whatever path your tree already uses for bitmaps - neither SVG
# generator will take them.
ls "$UI/assets/icons/nuoveXT2/48"

# The wallpaper, as the raw RGB24 dump the preview harness loads.  The
# harness canvas is 1280x800 and has no PNG decoder, so this runs ahead
# of time and writes straight bytes, no header.
python3 tools/make-wallpaper.py "$UI/assets/wallpaper/wallpaper.png" \
    tools/preview/wallpaper.bin
```

These commands are run, not guessed: the block above is what produced
`trait at 16: 145 lit ... trait at 32: 520 lit` from a Phipia OS checkout
with this repository beside it.

Three cautions, all of which bite silently:

- **`make-glyphs.py` strokes; `make-shell-icons.py` fills.** The glyph
  generator computes coverage as distance-to-nearest-path against half the
  stroke width, which is exact for Lucide (`fill="none" stroke-width="2"`)
  and wrong for a filled silhouette — reach for it by habit and `trait.svg`
  comes out a hairline outline. Both marks here are filled, so both go
  through the shell-icon generator.
- **Put the paint on the `<svg>`, never on a `<g>`.** `tools/svgpath.py`
  reads `fill`, `stroke` and `stroke-width` off the root element or off
  each shape, and inherits nothing from a group in between. The first run
  of the command above rasterized the mark to **0 lit pixels at every
  size** — a silently empty icon — because `trait.svg` carried its paint on
  a `<g>`, which every browser renders correctly. The asset here has been
  moved to the root element (the rendered mark is byte-identical, checked
  by screenshotting `#menu-button` before and after), so it works now; any
  mark you add later has to follow the same rule, or fix `svgpath.py` to
  walk groups.

- **The wallpaper is regenerated, not copied.** `assets/wallpaper/wallpaper.png`
  is itself rendered from `tools/wallpaper.html` with a seeded scatter
  (mulberry32 at 20260914). If you want a different size or crop for Phipia
  OS, re-run `tools/make-wallpaper.py` *here* first, then feed the result to
  the one over there. Two files with the same name doing different jobs is
  the one confusing thing about this pipeline.

### 3. Diff the frames

Both projects render PNGs of the same desktop, which is the only honest way
to check a port:

```sh
# Phipia OS
make -C tools/preview run

# Trait OS
python3 tools/shot.py build/desktop.png 1280 800
```

Render both at the same size and compare. A panel that is a pixel too tall or
a green that is `#00FF01` shows up here and nowhere else.

### What to carry over, in order

The pieces below are the ones worth porting, roughly easiest first. Each names
the file to read here.

1. **The panel and its plugin order** — `index.html`, `css/desktop.css`. Fixed
   geometry, no state; it is the whole identity of the desktop.
2. **The clock and the CPU graph** — `js/desktop.js`. Both are small, both are
   exact, and the CPU graph needs its history primed at start or it is a
   black rectangle while it fills — 36 columns at one per sample. That bug is
   in this repository's history too, and the fix (`primeCpu()`) is five
   lines.
3. **The window frame and its eight grips** — `css/desktop.css`, `addGrips()`.
   Phipia OS has its own window manager already; take the *metrics*.
4. **Clearlooks** — `css/files.css`. The palette plus the three derived shades
   (`shade(1.02)`, `shade(0.9)`, `shade(0.95)`) that GTK computes at runtime
   and which have to be precomputed for a kernel.
5. **The file manager** — `js/files.js`. The largest piece, and the one with a
   real model under it: a filesystem, a clipboard, multi-select, and dialogs
   that change what they claim to change.

### The rule that made this port-able at all

Nothing in Trait OS is drawn as a control that does not do the thing it is
drawn as. A dimmed row is dimmed because the state it needs is missing, not
because it was never wired up. That is why the C side can be written against
this: when you port a menu, the behaviour to port is *in* the menu, and
`tools/check.py` will tell you what it is — 34 checks carrying 155
assertions, driven through a real browser, every one of them broken on
purpose and watched to fail before it was trusted.

## Looking at it

```
python3 tools/shot.py build/desktop.png 1024 768   # render it
python3 tools/crop.py build/desktop.png out.png X Y W H SCALE
```

`shot.py` drives a real browser, so the PNG is what the page actually
does rather than what the markup meant. Anything after the size is a
launcher to press first, so `... 1024 768 terminal` photographs the
desktop with a terminal open.

```
python3 tools/check.py                             # and check it
```

`check.py` drives the same browser and asserts what the desktop claims —
**19 checks, 126 assertions**, over the panel, the file manager, the
editor, Settings, the task manager, the package manager, the browser, the
window manager and the desktop itself. It asserts, among other things:
that it is 26 pixels, that its plugins are in the profile's order, that
the clock is `%R`, that the CPU graph holds green **before the first
interval tick** (which is what the priming at load is for), and that the
terminal answers what is typed at it.

Every check in it has been broken on purpose and watched to fail, and the
failure message is quoted in the commit that added it.

**Several did not fail the first time, and those are the interesting
ones.** A check that survives the removal of the thing it checks is worse
than no check:

- *the CPU graph* — asking merely whether it holds green passes with the
  priming turned off, because the sampler fills it on its own within a
  few seconds. It is read inside one tick of load now.
- *the terminal* — the first version echoed the host's own name and
  searched the screen for it, which the prompt already contains, because
  the prompt is `user@` that name. It echoes a word the prompt cannot be
  saying, and matches a whole row rather than a substring.
- *the task manager's rows* — asking whether a disabled command is still
  in the row answers itself, because dropping it would leave its
  neighbour standing in the same slot. It measures that the neighbour has
  not moved, to the pixel.
- *Save As* — asking whether the dialog **says** "no such folder" passes
  when the save succeeded and the dialog closed, which is the exact
  failure it was meant to catch. It asks whether the box is still there
  first.

And several were faults in the checks themselves, not the code: one
opened a second package manager and clicked a window underneath, one
renamed a folder a later check needed, one left a package installed that
turned a later "install it" into "remove it", and two used selectors that
matched the same widget in the wrong window.

Two bugs the checks found that nothing looked wrong about:

- `#windows` is `inset: 0`, so with no window open it laid an invisible
  sheet over the whole desktop and swallowed every click on a desktop
  icon. The icons simply did not answer. `pointer-events: none` on the
  layer, `auto` on each frame.
- The status bar's free-space figure, described above.

## The wallpaper

A cool grey-blue (`#C4CBD3`), with the mark and wordmark at the upper left and a run of
thin-line towers climbing along the foot. It is this project's own art,
rendered from `tools/wallpaper.html` by the same browser, with a seeded
scatter so the file comes out identical every time:

```
python3 tools/make-wallpaper.py assets/wallpaper/wallpaper.png 1920 1080
```

It is composed for a 4:3 crop as well as its own 16:9 — the desktop draws
it with `background-size: cover`, so a 4:3 screen loses 171 pixels off
each side, and both the mark and the tallest tower sit inside that
margin.
