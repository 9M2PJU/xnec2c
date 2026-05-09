/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  The official website and doumentation for xnec2c is available here:
 *    https://www.xnec2c.org/
 */

#ifndef VIEW_PRESETS_H
#define VIEW_PRESETS_H 1

/*
 * Named preset angles and drag sensitivity constants for view_t.
 *
 * All angles are in NEC2 convention:
 *   WR = azimuth in XY plane from +X axis (degrees)
 *   WI = elevation from XY plane toward +Z (degrees)
 */

/* Default orientation applied at view creation and on Home button */
#define VIEW_DEFAULT_WR   45.0
#define VIEW_DEFAULT_WI   45.0

/* Axis-on presets for X/Y/Z buttons */
#define VIEW_PRESET_X_WR   0.0
#define VIEW_PRESET_X_WI   0.0
#define VIEW_PRESET_Y_WR  90.0
#define VIEW_PRESET_Y_WI   0.0
#define VIEW_PRESET_Z_WR   0.0
#define VIEW_PRESET_Z_WI  90.0

/* Divisor for constrained drag sensitivity.
 * Constrained drag subtracts (pixel_delta / VIEW_DRAG_DIVISOR) degrees
 * per event. */
#define VIEW_DRAG_DIVISOR 8

/* Free-drag axis-angle rate, radians per pixel.
 * Free mode pre-multiplies R by the per-event rotation; the rate is
 * tunable independently of VIEW_DRAG_DIVISOR. */
#define VIEW_FREE_DRAG_RAD_PER_PX 0.01f

/* Pole epsilon: squared magnitude of XY plane projection below which
 * azimuth (WR) is undefined (viewer looking along the Z axis). */
#define VIEW_POLE_EPS_SQ 1.0e-12f

/* Idempotence epsilons for mutator guards.  view_set_angles() and
 * view_set_zoom() feed back into spin-button value-changed handlers;
 * without an epsilon check the Build/Extract round-trip drifts in
 * the last float bit and the feedback path never converges. */
#define VIEW_ANGLE_EPS 1.0e-6
#define VIEW_ZOOM_EPS  1.0e-6f

#endif /* VIEW_PRESETS_H */
