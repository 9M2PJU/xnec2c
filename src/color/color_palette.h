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

#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H  1

#include "../common.h"
#include "color_srgb.h"

/* Lookup-table resolution shared by every palette kind */
#define PALETTE_LUT_SIZE  256

/*-----------------------------------------------------------------------
 * Palette kind axis: each kind carries exactly one visual meaning.
 * RAMP encodes magnitude intensity, DIVERGING a signed quantity, and
 * CYCLIC a phase angle; projections select the kind, never the user.
 *----------------------------------------------------------------------*/
typedef enum
{
  PALETTE_RAMP = 0,   /* magnitude intensity: magenta→blue→cyan→green→yellow→red */
  PALETTE_DIVERGING,  /* signed quantity: negative→neutral→positive */
  PALETTE_CYCLIC,     /* phase angle: wraps at the ends, distinct from RAMP */
  PALETTE_NUM
} palette_kind_t;

/* One resolved palette: linear-light lookup entries for the brightness
 * multiply and the wrap behavior for phase. */
typedef struct
{
  rgb_f_t  lut_lin[PALETTE_LUT_SIZE];  /* linear-light entries for value scaling */
  gboolean cyclic;                     /* wrap the index for phase coordinates */
} palette_t;

/**
 * palette_registry_init() - Build every palette LUT from theme colors
 *
 * Interpolates each kind's stop colors from the theme's gradient roles into
 * PALETTE_LUT_SIZE linear-light entries.  The ramp stops reproduce the
 * Value_to_Color sweep so the Amplitude projection under the legacy theme
 * is visually unchanged.  Idempotent: call again after theme_registry_init()
 * reloads the INI overlay.
 */
void palette_registry_init(void);

/**
 * color_palette_generation() - Read the palette LUT content generation
 *
 * Advances once per palette_registry_init(), so edge-gated consumers
 * recompute after a theme-driven LUT rebuild.
 */
uint32_t color_palette_generation(void);

/**
 * palette_get() - Resolve one palette by kind
 * @kind: palette kind selection
 *
 * Emits BUG and returns the ramp palette for an out-of-range kind.
 */
const palette_t *palette_get(palette_kind_t kind);

/**
 * palette_lookup_scaled() - Read a hue and scale it in linear light
 * @p:     palette to read
 * @coord: hue position; clamped or wrapped per the palette kind
 * @v:     brightness in [0, 1], applied in linear light
 *
 * Multiplies the linear-light LUT entry by v and re-encodes to sRGB, so
 * the brightness carrier maps onto the rendered sRGB scale.
 */
rgb_f_t palette_lookup_scaled(const palette_t *p, double coord, double v);

#endif
