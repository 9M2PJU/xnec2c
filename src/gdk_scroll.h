/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  The official website and doumentation for xnec2c is available here:
 *    https://www.xnec2c.org/
 */

#ifndef GDK_SCROLL_H
#define GDK_SCROLL_H

#include <gtk/gtk.h>

/* Resolved scroll motion for one frame, normalised across discrete wheel
 * notches and smooth trackpad frames.  direction is never GDK_SCROLL_SMOOTH:
 * a smooth frame is reduced to the dominant axis sign and its magnitude is
 * carried in step. */
typedef struct {
  gboolean            active;     /* FALSE: stop frame or unknown */
  GdkScrollDirection  direction;  /* UP/DOWN/LEFT/RIGHT, never SMOOTH */
  double              step;       /* 1.0 discrete; |delta| smooth */
} scroll_step_t;

/* Capture-phase controller closure for one spin.  wheel_grain == 0.0 defers
 * to the spin adjustment's step-increment (see spin_notch_scroll_cb); a
 * non-zero value overrides the per-notch grain, used by the rotation spins. */
typedef struct {
  double wheel_grain;
} spin_scroll_ctx_t;

scroll_step_t scroll_step_from_deltas(GdkEvent *event);
void scroll_install_spin_notches(GtkSpinButton *spin, double wheel_grain);
void scroll_install_all_spins(GtkBuilder *builder);

#endif /* GDK_SCROLL_H */
