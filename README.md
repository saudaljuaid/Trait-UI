# Phipia UI

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
dclock(%R) | launchbar(screenlock, logout)
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
beside them: the mark on the menu button (`assets/logo/phipia.svg`) and
the wallpaper, which `tools/make-wallpaper.py` generates from two colours
and one soft light.

**The mark sits low-contrast on the dark panel** — it is a mid grey on a
near-black bar, where LXDE's own mark is light grey. That is the mark's
own colour and it has not been repainted here; lightening it is one fill
value in `assets/logo/phipia.svg`.

## Every button on it does what it is drawn as

The menu button opens the menu the profile describes — the applications
by freedesktop category, a rule, `Run...`, a rule, `Logout` — upwards off
the panel, because the panel is at the foot of the screen. A category
with nothing in it is not drawn; LXDE does not draw one either.

`Run...` runs this desktop's own programs and says `no such program` for
anything else. The speaker opens a slider and its mark changes when you
mute it. The padlock covers the whole screen, panel included — a lock you
could click past would be a picture of a lock. `Logout` closes every
window.

Four of `lxsession-logout`'s six buttons — shut down, reboot, suspend,
hibernate — are **not** offered, because a page cannot do them and the
dialog says so rather than drawing a button that lies.

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
| `Ctrl+Alt+Delete` | log out |
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

## Tooltips

GTK2's, in Clearlooks' own colours — `tooltip_bg_color:#F5F5B5` with
`tooltip_fg_color:#000000`, the pale yellow note every GTK2 desktop of
that era has, after GTK's own 500ms delay. The text is moved out of
`title=` the first time an element is pointed at, so the browser does not
draw its dark rounded box on top of ours — that box would be the one
thing on this desktop that is not this desktop.

A right click on the panel drops `lxpanel`'s own menu, above the bar.

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
view, and a two-field status bar — and all of it works. Back and Forward
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

Its mark, `assets/logo/files.svg`, is this project's own: a drawer with
fanned papers, drawn in the same geometry as the Phipia mark — the same
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
task; ending `phipia-session` is refused out loud, because it is the
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

`check.py` drives the same browser and asserts what the panel claims:
that it is 26 pixels, that its plugins are in the profile's order, that
the clock is `%R`, that the CPU graph holds green **before the first
interval tick** (which is what the priming at load is for), and that the
terminal answers what is typed at it.

Every check in it has been broken on purpose and watched to fail. Two of
them did not fail the first time and were rewritten:

- *the CPU graph* — asking merely whether it holds green passes with the
  priming turned off, because the sampler fills it on its own within a
  few seconds. It is read inside one tick of load now.
- *the terminal* — the first version echoed the word `phipia` and
  searched the screen for it, which the prompt already contains, because
  the host is called `phipia`. It echoes a word the prompt cannot be
  saying, and matches a whole row rather than a substring.

Two bugs the checks found that nothing looked wrong about:

- `#windows` is `inset: 0`, so with no window open it laid an invisible
  sheet over the whole desktop and swallowed every click on a desktop
  icon. The icons simply did not answer. `pointer-events: none` on the
  layer, `auto` on each frame.
- The status bar's free-space figure, described above.

## The wallpaper

White, with the mark and wordmark at the upper left and a run of
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
