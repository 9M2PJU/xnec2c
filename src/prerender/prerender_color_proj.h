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

#ifndef PRERENDER_COLOR_PROJ_H
#define PRERENDER_COLOR_PROJ_H  1

#include "prerender_color.h"

/*-----------------------------------------------------------------------
 * Color projection axis: how a per-element phasor maps to a color.
 * AMPLITUDE is the static default and the fall-through baseline; the
 * animated projections evaluate the phasor at the animation phase.
 *----------------------------------------------------------------------*/
typedef enum
{
  COLOR_PROJ_AMPLITUDE = 0, /* |z|, phase-invariant; static default + fall-through */
  COLOR_PROJ_INSTANT,       /* instantaneous magnitude: envelope hue, |cos| brightness */
  COLOR_PROJ_SIGNED,        /* instantaneous polarity: scale(|z|)·cos on blue-black-red */
  COLOR_PROJ_PHASE,         /* phase hue: hue arg(z)+φ, brightness scale(|z|) */
  COLOR_PROJ_NUM
} color_proj_t;

/*-----------------------------------------------------------------------
 * Color scale axis: normalized-magnitude transfer applied before the
 * colorizer.  LINEAR is the identity and the fall-through baseline.
 *----------------------------------------------------------------------*/
typedef enum
{
  COLOR_SCALE_LINEAR = 0,
  COLOR_SCALE_SQRT,
  COLOR_SCALE_SQUARED,
  COLOR_SCALE_DB,
  COLOR_SCALE_NUM
} color_scale_t;

/* Colorizer output kind, resolved once via color_proj_out[] */
typedef enum
{
  PROJ_OUT_RAMP,       /* rainbow ramp via color_from_value */
  PROJ_OUT_DIVERGING,  /* signed blue-black-red ramp */
  PROJ_OUT_HSV         /* hue = phase, brightness = magnitude */
} proj_out_t;

/* Colorizer output kind per projection */
extern const proj_out_t color_proj_out[COLOR_PROJ_NUM];

/* Per-bake evaluation context: animation phase, magnitude bounds, and the
 * scale transfer with its auto-ranged dB floor. */
typedef struct
{
  double c, s;        /* cos/sin of the animation phase */
  double phase;       /* animation phase in radians */
  double cmax;        /* phase-invariant amplitude bound */
  double floor_ratio; /* dB dynamic-range floor / cmax, in (0,1] */
  color_scale_t scale;
} color_ctx_t;

/**
 * color_ctx_init() - Fill a projection context from phase, bounds, and scale
 * @x:     context to fill
 * @phase: animation phase in radians
 * @cmin:  smallest magnitude in the data (dB floor input)
 * @cmax:  largest magnitude in the data
 * @scale: scale transfer selection
 *
 * Derives the dB floor ratio by clamping the data dynamic range to
 * [20, 60] dB, auto-ranging the log scale with the data.
 */
void color_ctx_init(color_ctx_t *x, double phase,
    double cmin, double cmax, color_scale_t scale);

/**
 * color_project() - Map one phasor to a color under projection and scale
 * @z:    element phasor (real-only quantities pass mag + 0i)
 * @proj: projection selection
 * @x:    evaluation context
 */
rgb_f_t color_project(complex double z, color_proj_t proj, const color_ctx_t *x);

/**
 * color_project_norm() - Map a normalized strip position to a legend color
 * @p:    strip position in [0, 1]
 * @proj: projection selection
 * @x:    evaluation context (phase 0 for a static legend)
 *
 * Synthesizes the phasor whose projection sweeps the colorizer domain:
 * magnitude p·cmax for ramp, signed (2p-1)·cmax for diverging, and unit
 * phasor at angle 2π·p for hue.
 */
rgb_f_t color_project_norm(double p, color_proj_t proj, const color_ctx_t *x);

/**
 * color_proj_sanitize() - Bound a persisted projection value to the enum
 * @v: raw persisted integer
 *
 * Emits pr_err and returns COLOR_PROJ_AMPLITUDE when out of range.
 */
color_proj_t color_proj_sanitize(int v);

/**
 * color_scale_sanitize() - Bound a persisted scale value to the enum
 * @v: raw persisted integer
 *
 * Emits pr_err and returns COLOR_SCALE_LINEAR when out of range.
 */
color_scale_t color_scale_sanitize(int v);

/**
 * color_proj_wire_current() - Resolve wire-current colors for one fstep
 * @fstep: frequency step index
 * @phase: animation phase in radians
 * @proj:  projection selection
 * @scale: scale transfer selection
 *
 * Source: crnt_fstep[fstep].cur wire span.  Returns the precomputed
 * amplitude array for AMPLITUDE + LINEAR, else the baked scratch buffer.
 */
const rgb_f_t *color_proj_wire_current(int fstep, double phase,
    color_proj_t proj, color_scale_t scale);

/**
 * color_proj_wire_charge() - Resolve wire-charge colors for one fstep
 * @fstep: frequency step index
 * @phase: animation phase in radians
 * @proj:  projection selection
 * @scale: scale transfer selection
 *
 * Gathers the bir/bii charge phasor before baking.
 */
const rgb_f_t *color_proj_wire_charge(int fstep, double phase,
    color_proj_t proj, color_scale_t scale);

/**
 * color_proj_patch() - Resolve patch-fill colors for one fstep
 * @fstep: frequency step index
 * @phase: animation phase in radians
 * @proj:  projection selection
 * @scale: scale transfer selection
 *
 * Gathers the patch normal-component phasor cur[data.n + 3i] before baking.
 */
const rgb_f_t *color_proj_patch(int fstep, double phase,
    color_proj_t proj, color_scale_t scale);

/**
 * color_proj_generation() - Read the bake generation counter
 *
 * Incremented on every scratch-buffer bake; GL geometry keys staleness
 * on this value beside the color pointer.
 */
uint32_t color_proj_generation(void);

/**
 * color_proj_alloc() - Size the projection scratch buffers to the model
 *
 * Called from alloc_struct_colors(); wire scratch [data.n], patch scratch
 * [data.m], phasor gather scratch [max(data.n, data.m)].
 */
void color_proj_alloc(void);

/**
 * color_proj_free() - Release the projection scratch buffers
 *
 * Called from free_struct_colors().
 */
void color_proj_free(void);

#endif
