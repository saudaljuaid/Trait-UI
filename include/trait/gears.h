/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRAIT_GEARS_H
#define TRAIT_GEARS_H

#include <trait/surface.h>

/*
 * The root window.
 *
 * An X session with nothing running on it shows the weave twm and fvwm
 * come up on, and for a while this desktop did too.  It shows glxgears
 * now, which is the other thing a bare X session has always had on it
 * and the one people recognise.  The geometry is Brian Paul's, projected
 * ahead of time into src/trait_gears_art.h; this draws it at whatever
 * size the surface turns out to be.
 */
void trait_gears_draw(struct trait_surface *surface, struct trait_rect clip);

#endif /* TRAIT_GEARS_H */
