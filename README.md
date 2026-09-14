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

## Looking at it

```
python3 tools/shot.py build/desktop.png 1024 768   # render it
python3 tools/crop.py build/desktop.png out.png X Y W H SCALE
```

`shot.py` drives a real browser, so the PNG is what the page actually
does rather than what the markup meant.
