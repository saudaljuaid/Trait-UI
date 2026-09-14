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
