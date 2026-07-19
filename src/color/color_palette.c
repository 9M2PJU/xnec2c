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

/*
 * color_palette: theme-hooked palette lookup tables.
 *
 * One 256-entry linear-light LUT per palette kind, interpolated in sRGB
 * between the stop colors owned by the theme's dedicated THEME_ROLE_GRAD_*
 * roles, then linearized so the per-frame brightness multiply runs in
 * linear light.  Interpolation in sRGB reproduces the Value_to_Color
 * rainbow sweep exactly for the ramp kind.
 */
#include "color_palette.h"
#include "../themes/theme.h"
#include "../shared.h"

/* One kind's ordered stop roles: each palette interpolates between the
 * theme's gradient roles, so the ramp re-themes per selected theme.  A
 * cyclic kind closes the sweep back onto its first stop. */
typedef struct
{
  const theme_role_e *stops;    /* ordered stop roles resolved per theme */
  int                 n_stops;  /* stop count */
  gboolean            cyclic;   /* interpolate last stop back to first */
} palette_spec_t;

static const theme_role_e ramp_stops[] = {
  THEME_ROLE_GRAD_0, THEME_ROLE_GRAD_1, THEME_ROLE_GRAD_2,
  THEME_ROLE_GRAD_3, THEME_ROLE_GRAD_4, THEME_ROLE_GRAD_5,
};

/* Diverging poles are the ramp endpoints; the neutral center is the theme's
 * dedicated achromatic role so a signed null stays visible at the brightness
 * floor: a background-colored center has zero linear luminance on dark
 * themes and no multiplicative floor can lift it. */
static const theme_role_e diverging_stops[] = {
  THEME_ROLE_GRAD_0, THEME_ROLE_GRAD_MID, THEME_ROLE_GRAD_5,
};

static const palette_spec_t palette_specs[PALETTE_NUM] =
{
  [PALETTE_RAMP]      = { ramp_stops,      6, FALSE },
  [PALETTE_DIVERGING] = { diverging_stops, 3, FALSE },
  /* The cyclic wheel reuses the ramp stops and wraps 5 -> 0 for phase. */
  [PALETTE_CYCLIC]    = { ramp_stops,      6, TRUE },
};

static palette_t palettes[PALETTE_NUM];

/* LUT content generation, advanced by each registry (re)build; zero
 * until the first palette_registry_init() */
static uint32_t palette_generation;

/*-----------------------------------------------------------------------*/

/**
 * palette_build() - Fill one palette's LUTs from its stop roles
 * @p:    palette to fill
 * @spec: ordered stop roles and wrap behavior
 *
 * Interpolates linearly in sRGB between consecutive stop-role colors read
 * from the active theme; a cyclic spec wraps the last stop back onto the
 * first so entry 255 meets entry 0 seamlessly.  Stores each entry
 * linearized.
 */
static void
palette_build(palette_t *p, const palette_spec_t *spec)
{
  const theme_t *t = theme_active();
  int n_seg = spec->cyclic ? spec->n_stops : spec->n_stops - 1;
  int i;

  for( i = 0; i < PALETTE_LUT_SIZE; i++ )
  {
    double pos = (double)i * (double)n_seg / (double)PALETTE_LUT_SIZE;
    int seg = (int)pos;
    double f = pos - (double)seg;
    rgb_f_t a = t->colors[spec->stops[seg]];
    rgb_f_t b = t->colors[spec->stops[(seg + 1) % spec->n_stops]];
    rgb_f_t srgb;

    srgb.r = (float)((1.0 - f) * a.r + f * b.r);
    srgb.g = (float)((1.0 - f) * a.g + f * b.g);
    srgb.b = (float)((1.0 - f) * a.b + f * b.b);

    p->lut_lin[i].r = srgb_channel_to_linear(srgb.r);
    p->lut_lin[i].g = srgb_channel_to_linear(srgb.g);
    p->lut_lin[i].b = srgb_channel_to_linear(srgb.b);
  }

  p->cyclic = spec->cyclic;
}

/*-----------------------------------------------------------------------*/

  void
palette_registry_init(void)
{
  palette_kind_t k;

  for( k = 0; k < PALETTE_NUM; k++ )
    palette_build(&palettes[k], &palette_specs[k]);

  /* Advance the LUT content generation so edge-gated consumers recompute */
  palette_generation++;
}

/*-----------------------------------------------------------------------*/

  uint32_t
color_palette_generation(void)
{
  return palette_generation;
}

/*-----------------------------------------------------------------------*/

  const palette_t *
palette_get(palette_kind_t kind)
{
  if( kind < 0 || kind >= PALETTE_NUM )
  {
    BUG("invalid palette kind %d, using ramp\n", kind);
    return &palettes[PALETTE_RAMP];
  }

  return &palettes[kind];
}

/*-----------------------------------------------------------------------*/

/**
 * palette_index() - Map a coordinate to a LUT index per wrap behavior
 * @p:     palette whose wrap behavior applies
 * @coord: hue coordinate; any real value
 */
static int
palette_index(const palette_t *p, double coord)
{
  if( p->cyclic )
  {
    coord -= floor(coord);
    return (int)(coord * (double)PALETTE_LUT_SIZE) % PALETTE_LUT_SIZE;
  }

  if( coord <= 0.0 )
    return 0;

  if( coord >= 1.0 )
    return PALETTE_LUT_SIZE - 1;

  return (int)(coord * (double)(PALETTE_LUT_SIZE - 1));
}

/*-----------------------------------------------------------------------*/

  rgb_f_t
palette_lookup_scaled(const palette_t *p, double coord, double v)
{
  const rgb_f_t *lin = &p->lut_lin[palette_index(p, coord)];
  rgb_f_t c;

  c.r = linear_channel_to_srgb((float)(lin->r * v));
  c.g = linear_channel_to_srgb((float)(lin->g * v));
  c.b = linear_channel_to_srgb((float)(lin->b * v));
  return c;
}

/*-----------------------------------------------------------------------*/
