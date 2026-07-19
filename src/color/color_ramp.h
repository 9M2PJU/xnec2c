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

#ifndef COLOR_RAMP_H
#define COLOR_RAMP_H  1

#include "../common.h"

/**
 * Value_to_Color() - Map a scalar value to an RGB color
 * @red:   output red channel [0..1]
 * @grn:   output green channel [0..1]
 * @blu:   output blue channel [0..1]
 * @val:   input value
 * @max:   normalization maximum
 *
 * Maps val/max through a magenta-blue-cyan-green-yellow-red rainbow ramp.
 * Values outside [0..max] are clamped.
 */
void Value_to_Color(
    double *red, double *grn, double *blu,
    double val, double max);

/**
 * color_from_value() - Map a scalar to an rgb_f_t via the rainbow ramp
 * @val: value to map (0..max)
 * @max: normalization maximum; returns black when max <= 0
 *
 * Single point of truth for the Value_to_Color -> rgb_f_t conversion used
 * by the radiation-pattern prerender.
 */
static inline rgb_f_t
color_from_value(double val, double max)
{
  rgb_f_t c;
  double r, g, b;
  Value_to_Color(&r, &g, &b, val, max);
  c.r = (float)r;
  c.g = (float)g;
  c.b = (float)b;
  return c;
}

#endif
