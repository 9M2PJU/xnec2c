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

#ifndef COLOR_SRGB_H
#define COLOR_SRGB_H  1

#include "../common.h"

/* sRGB transfer inlines shared by the palette and animation subtrees; the
 * brightness multiply runs in linear light, so both consume these. */

/**
 * srgb_channel_to_linear() - Decode one sRGB channel to linear light
 * @c: sRGB-encoded channel in [0, 1]
 */
static inline float
srgb_channel_to_linear(float c)
{
  return (c <= 0.04045f) ? c / 12.92f
       : powf((c + 0.055f) / 1.055f, 2.4f);
}

/**
 * linear_channel_to_srgb() - Encode one linear-light channel to sRGB
 * @l: linear-light channel in [0, 1]
 */
static inline float
linear_channel_to_srgb(float l)
{
  return (l <= 0.0031308f) ? 12.92f * l
       : 1.055f * powf(l, 1.0f / 2.4f) - 0.055f;
}

/**
 * rgb_linear_scale() - Scale an sRGB color by a factor in linear light
 * @c: sRGB color
 * @v: brightness factor in [0, 1]
 */
static inline rgb_f_t
rgb_linear_scale(rgb_f_t c, double v)
{
  rgb_f_t out;

  out.r = linear_channel_to_srgb((float)(srgb_channel_to_linear(c.r) * v));
  out.g = linear_channel_to_srgb((float)(srgb_channel_to_linear(c.g) * v));
  out.b = linear_channel_to_srgb((float)(srgb_channel_to_linear(c.b) * v));
  return out;
}

#endif
