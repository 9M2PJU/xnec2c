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
 * prerender_color_proj: phasor→color projection and scale core.
 *
 * One projection mapping (color_project) and one scale transfer
 * (color_scale_apply) serve wire currents, wire charges, patch fill,
 * near-field samples, and the color-code legend.  Per-source resolvers
 * bake into module scratch buffers and fall through to the precomputed
 * amplitude arrays when the selection is the identity mapping.
 */
#include "prerender_color_proj.h"
#include "../shared.h"

const proj_out_t color_proj_out[COLOR_PROJ_NUM] = {
  [COLOR_PROJ_AMPLITUDE] = PROJ_OUT_RAMP,
  [COLOR_PROJ_INSTANT]   = PROJ_OUT_RAMP,
  [COLOR_PROJ_SIGNED]    = PROJ_OUT_DIVERGING,
  [COLOR_PROJ_PHASE]     = PROJ_OUT_HSV,
};

/* Bake source discriminator for the input-snapshot cache keys */
typedef enum
{
  PROJ_SRC_NONE = 0,
  PROJ_SRC_WIRE_CRNT,
  PROJ_SRC_WIRE_CHRG,
  PROJ_SRC_PATCH
} proj_src_t;

/* Bake-input snapshot: a resolver rebakes only when the whole key differs
 * from the previous call, so repeated exposes reuse the last bake. */
typedef struct
{
  proj_src_t    src;
  int           fstep;
  double        phase;
  double        freq_mhz;
  float         cmin, cmax;   /* data bounds; change when the step refills */
  color_proj_t  proj;
  color_scale_t scale;
} proj_bake_key_t;

static rgb_f_t *wire_proj_rgb  = NULL;  /* [data.n] baked wire colors */
static rgb_f_t *patch_proj_rgb = NULL;  /* [data.m] baked patch colors */
static complex double *proj_z_scratch = NULL; /* gather buffer for SoA sources */
static uint32_t proj_generation = 0;

static proj_bake_key_t wire_key;
static proj_bake_key_t patch_key;
static const rgb_f_t *wire_result  = NULL;
static const rgb_f_t *patch_result = NULL;

/*-----------------------------------------------------------------------*/

/**
 * proj_key_equal() - Compare two bake-input snapshots
 * @a: previous key
 * @b: candidate key
 *
 * Scalar identity comparison: both sides are verbatim copies of the same
 * inputs, so exact equality is the correct match test.
 */
static gboolean
proj_key_equal(const proj_bake_key_t *a, const proj_bake_key_t *b)
{
  return a->src   == b->src
      && a->fstep == b->fstep
      && a->phase == b->phase
      && a->freq_mhz == b->freq_mhz
      && a->cmin  == b->cmin
      && a->cmax  == b->cmax
      && a->proj  == b->proj
      && a->scale == b->scale;
}

/*-----------------------------------------------------------------------*/

  void
color_ctx_init(color_ctx_t *x, double phase,
    double cmin, double cmax, color_scale_t scale)
{
  double range_db;

  x->phase = phase;
  x->c     = cos(phase);
  x->s     = sin(phase);
  x->cmax  = cmax;
  x->scale = scale;

  if( cmax <= 0.0 )
  {
    x->floor_ratio = 1.0;
    return;
  }

  /* Auto-range the dB floor: clamp the data dynamic range to [20, 60] dB */
  range_db = 20.0 * log10(cmax / fmax(cmin, cmax * 1e-6));
  range_db = fmin(fmax(range_db, 20.0), 60.0);
  x->floor_ratio = pow(10.0, -range_db / 20.0);
}

/*-----------------------------------------------------------------------*/

/**
 * color_scale_apply() - Apply the scale transfer to a normalized magnitude
 * @n: normalized magnitude in [0, 1]
 * @x: evaluation context holding scale and dB floor
 */
static double
color_scale_apply(double n, const color_ctx_t *x)
{
  switch( x->scale )
  {
    case COLOR_SCALE_SQRT:
      return sqrt(n);

    case COLOR_SCALE_SQUARED:
      return n * n;

    case COLOR_SCALE_DB:
    {
      double f = x->floor_ratio;
      return (log10(fmax(n, f)) - log10(f)) / (-log10(f));
    }

    case COLOR_SCALE_LINEAR:
      return n;

    case COLOR_SCALE_NUM:
      break;
  }

  BUG("unhandled color scale %d\n", x->scale);
  return n;
}

/*-----------------------------------------------------------------------*/

/**
 * proj_diverging() - Map a signed normalized value to blue-black-red
 * @signed_n: scaled value in [-1, 1]
 *
 * Zero renders dark so a segment fades at its null on the dark canvas;
 * magnitude rides the brightness channel.
 */
static rgb_f_t
proj_diverging(double signed_n)
{
  rgb_f_t c = { 0.0f, 0.0f, 0.0f };

  if( signed_n < 0.0 )
    c.b = (float)(-signed_n);
  else
    c.r = (float)signed_n;

  return c;
}

/*-----------------------------------------------------------------------*/

/**
 * proj_hsv() - Map hue and brightness to RGB at full saturation
 * @h: hue in [0, 1)
 * @v: brightness in [0, 1]
 */
static rgb_f_t
proj_hsv(double h, double v)
{
  double r = 0.0, g = 0.0, b = 0.0;
  double hh = h * 6.0;
  int sect = (int)hh;
  double f = hh - (double)sect;
  rgb_f_t c;

  switch( sect % 6 )
  {
    case 0: r = 1.0;     g = f;       b = 0.0;     break;
    case 1: r = 1.0 - f; g = 1.0;     b = 0.0;     break;
    case 2: r = 0.0;     g = 1.0;     b = f;       break;
    case 3: r = 0.0;     g = 1.0 - f; b = 1.0;     break;
    case 4: r = f;       g = 0.0;     b = 1.0;     break;
    default: /* sector 5 */
            r = 1.0;     g = 0.0;     b = 1.0 - f; break;
  }

  c.r = (float)(r * v);
  c.g = (float)(g * v);
  c.b = (float)(b * v);
  return c;
}

/*-----------------------------------------------------------------------*/

  rgb_f_t
color_project(complex double z, color_proj_t proj, const color_ctx_t *x)
{
  double m, t, h, v;
  rgb_f_t c;

  switch( proj )
  {
    /* Composite channels: hue holds the scaled static envelope so a
     * segment keeps its amplitude identity, while the raw cosine factor
     * rides brightness so a null crossing sweeps smoothly instead of
     * racing through the unbounded sqrt/dB slope at zero. */
    case COLOR_PROJ_INSTANT:
      m = cabs(z);
      t = (m > 0.0) ? fabs(creal(z) * x->c - cimag(z) * x->s) / m : 0.0;
      c = color_from_value(color_scale_apply(m / x->cmax, x), 1.0);
      c.r *= (float)t;
      c.g *= (float)t;
      c.b *= (float)t;
      return c;

    case COLOR_PROJ_SIGNED:
      m = cabs(z);
      t = (m > 0.0) ? (creal(z) * x->c - cimag(z) * x->s) / m : 0.0;
      return proj_diverging(color_scale_apply(m / x->cmax, x) * t);

    case COLOR_PROJ_PHASE:
      h = fmod(carg(z) + x->phase + M_2PI, M_2PI) / M_2PI;
      v = color_scale_apply(cabs(z) / x->cmax, x);
      return proj_hsv(h, v);

    case COLOR_PROJ_AMPLITUDE:
      m = cabs(z) / x->cmax;
      return color_from_value(color_scale_apply(m, x), 1.0);

    case COLOR_PROJ_NUM:
      break;
  }

  BUG("unhandled color projection %d\n", proj);
  c = (rgb_f_t){ 0.0f, 0.0f, 0.0f };
  return c;
}

/*-----------------------------------------------------------------------*/

  rgb_f_t
color_project_norm(double p, color_proj_t proj, const color_ctx_t *x)
{
  switch( proj )
  {
    case COLOR_PROJ_SIGNED:
      return color_project((2.0 * p - 1.0) * x->cmax, proj, x);

    case COLOR_PROJ_PHASE:
      return color_project(cexp(I * M_2PI * p) * x->cmax, proj, x);

    /* Magnitude ramp projections synthesize the phasor p·cmax */
    case COLOR_PROJ_AMPLITUDE:
    case COLOR_PROJ_INSTANT:
      return color_project(p * x->cmax, proj, x);

    case COLOR_PROJ_NUM:
      break;
  }

  BUG("unhandled color projection %d\n", proj);
  return color_project(p * x->cmax, COLOR_PROJ_AMPLITUDE, x);
}

/*-----------------------------------------------------------------------*/

  color_proj_t
color_proj_sanitize(int v)
{
  if( v < 0 || v >= COLOR_PROJ_NUM )
  {
    pr_err("invalid color projection %d, using amplitude\n", v);
    return COLOR_PROJ_AMPLITUDE;
  }

  return (color_proj_t)v;
}

/*-----------------------------------------------------------------------*/

  color_scale_t
color_scale_sanitize(int v)
{
  if( v < 0 || v >= COLOR_SCALE_NUM )
  {
    pr_err("invalid color scale %d, using linear\n", v);
    return COLOR_SCALE_LINEAR;
  }

  return (color_scale_t)v;
}

/*-----------------------------------------------------------------------*/

/**
 * color_proj_scalar_bake() - Bake one phasor array into a color buffer
 * @out:      destination scratch buffer [n]
 * @fallback: precomputed amplitude+linear color array
 * @z:        source phasor array [n]
 * @n:        element count
 * @cmin:     smallest source magnitude (dB floor input)
 * @cmax:     largest source magnitude
 * @phase:    animation phase in radians
 * @proj:     projection selection
 * @scale:    scale transfer selection
 *
 * Unconditional projector: behavior invariant across callers.  Returns
 * the fallback without baking for the identity mapping or when no data
 * bounds exist; otherwise bakes and bumps the generation counter.
 */
static const rgb_f_t *
color_proj_scalar_bake(rgb_f_t *out, const rgb_f_t *fallback,
    const complex double *z, int n, double cmin, double cmax,
    double phase, color_proj_t proj, color_scale_t scale)
{
  color_ctx_t x;
  int i;

  if( proj == COLOR_PROJ_AMPLITUDE && scale == COLOR_SCALE_LINEAR )
    return fallback; /* precomputed amplitude+linear array; no bake, no bump */

  if( out == NULL || z == NULL || n <= 0 || cmax <= 0.0 )
    return fallback;

  color_ctx_init(&x, phase, cmin, cmax, scale);

  for( i = 0; i < n; i++ )
    out[i] = color_project(z[i], proj, &x);

  proj_generation++;
  return out;
}

/*-----------------------------------------------------------------------*/

  const rgb_f_t *
color_proj_wire_current(int fstep, double phase,
    color_proj_t proj, color_scale_t scale)
{
  proj_bake_key_t k = { .src = PROJ_SRC_WIRE_CRNT, .fstep = fstep,
    .phase = phase, .freq_mhz = calc_data.freq_mhz,
    .cmin = struct_colors[fstep].wire_crnt_cmin,
    .cmax = struct_colors[fstep].wire_crnt_cmax,
    .proj = proj, .scale = scale };

  if( proj_key_equal(&wire_key, &k) )
    return wire_result;

  wire_result = color_proj_scalar_bake(wire_proj_rgb,
      struct_colors[fstep].wire_crnt_rgb,
      crnt_fstep[fstep].cur, data.n,
      (double)k.cmin, (double)k.cmax, phase, proj, scale);
  wire_key = k;
  return wire_result;
}

/*-----------------------------------------------------------------------*/

  const rgb_f_t *
color_proj_wire_charge(int fstep, double phase,
    color_proj_t proj, color_scale_t scale)
{
  int i;
  proj_bake_key_t k = { .src = PROJ_SRC_WIRE_CHRG, .fstep = fstep,
    .phase = phase, .freq_mhz = calc_data.freq_mhz,
    .cmin = struct_colors[fstep].wire_chrg_cmin,
    .cmax = struct_colors[fstep].wire_chrg_cmax,
    .proj = proj, .scale = scale };

  if( proj_key_equal(&wire_key, &k) )
    return wire_result;

  /* Gather the charge phasor from the split bir/bii arrays */
  if( proj_z_scratch != NULL )
  {
    for( i = 0; i < data.n; i++ )
      proj_z_scratch[i] = crnt_fstep[fstep].bir[i]
                        + I * crnt_fstep[fstep].bii[i];
  }

  wire_result = color_proj_scalar_bake(wire_proj_rgb,
      struct_colors[fstep].wire_chrg_rgb,
      proj_z_scratch, data.n,
      (double)k.cmin, (double)k.cmax, phase, proj, scale);
  wire_key = k;
  return wire_result;
}

/*-----------------------------------------------------------------------*/

  const rgb_f_t *
color_proj_patch(int fstep, double phase,
    color_proj_t proj, color_scale_t scale)
{
  int i;
  proj_bake_key_t k = { .src = PROJ_SRC_PATCH, .fstep = fstep,
    .phase = phase, .freq_mhz = calc_data.freq_mhz,
    .cmin = struct_colors[fstep].patch_crnt_cmin,
    .cmax = struct_colors[fstep].patch_crnt_cmax,
    .proj = proj, .scale = scale };

  if( proj_key_equal(&patch_key, &k) )
    return patch_result;

  /* Gather the patch normal-component phasor from the strided cur array */
  if( proj_z_scratch != NULL )
  {
    for( i = 0; i < data.m; i++ )
      proj_z_scratch[i] = crnt_fstep[fstep].cur[data.n + 3 * i];
  }

  patch_result = color_proj_scalar_bake(patch_proj_rgb,
      struct_colors[fstep].patch_crnt_rgb,
      proj_z_scratch, data.m,
      (double)k.cmin, (double)k.cmax, phase, proj, scale);
  patch_key = k;
  return patch_result;
}

/*-----------------------------------------------------------------------*/

  uint32_t
color_proj_generation(void)
{
  return proj_generation;
}

/*-----------------------------------------------------------------------*/

  void
color_proj_alloc(void)
{
  int zn = (data.n > data.m) ? data.n : data.m;

  if( data.n > 0 )
    mem_array_realloc(&wire_proj_rgb, data.n);

  if( data.m > 0 )
    mem_array_realloc(&patch_proj_rgb, data.m);

  if( zn > 0 )
    mem_array_realloc(&proj_z_scratch, zn);

  /* Buffers resized for a new model: drop the cached bake results */
  wire_key  = (proj_bake_key_t){ .src = PROJ_SRC_NONE };
  patch_key = (proj_bake_key_t){ .src = PROJ_SRC_NONE };
  wire_result  = NULL;
  patch_result = NULL;
}

/*-----------------------------------------------------------------------*/

  void
color_proj_free(void)
{
  mem_array_free(&wire_proj_rgb);
  mem_array_free(&patch_proj_rgb);
  mem_array_free(&proj_z_scratch);

  wire_key  = (proj_bake_key_t){ .src = PROJ_SRC_NONE };
  patch_key = (proj_bake_key_t){ .src = PROJ_SRC_NONE };
  wire_result  = NULL;
  patch_result = NULL;
}

/*-----------------------------------------------------------------------*/
