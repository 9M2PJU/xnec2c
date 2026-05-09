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

#ifndef PRERENDER_COLOR_H
#define PRERENDER_COLOR_H       1

#include "../common.h"

/*-----------------------------------------------------------------------
 * Segment color classification
 *
 * Bare enum per wire segment: excitation source, loaded, or plain wire.
 * Precomputed O(1) lookup replacing per-frame iteration over vsorc/zload.
 *----------------------------------------------------------------------*/
typedef enum
{
  SEG_COLOR_NORMAL = 0,
  SEG_COLOR_LOADED,
  SEG_COLOR_LOADED_RESISTIVITY,   /* ldtype == 5: resistivity, drawn width 2.0 */
  SEG_COLOR_EXCITATION,
} segment_color_type_t;

/**
 * Value_to_Color() - Map a scalar value to an RGB color
 * @red:   output red channel [0..1]
 * @grn:   output green channel [0..1]
 * @blu:   output blue channel [0..1]
 * @val:   input value
 * @max:   normalization maximum
 *
 * Maps val/max through a blue-cyan-green-yellow-red rainbow ramp.
 * Values outside [0..max] are clamped.
 */
void Value_to_Color(
    double *red, double *grn, double *blu,
    double val, double max);

/**
 * segment_type_to_rgb() - Map segment classification to display color
 * @type: segment color classification
 * @r:    output red [0..1]
 * @g:    output green [0..1]
 * @b:    output blue [0..1]
 */
void segment_type_to_rgb(segment_color_type_t type,
    float *r, float *g, float *b);


/**
 * get_segment_color_type() - Classify a wire segment by excitation/load/network
 * @seg_num: 1-indexed segment number (matches vsorc.isant, zload.ldsegn)
 *
 * Returns the classification for coloring in geometry display mode.
 */
segment_color_type_t get_segment_color_type(int seg_num);

/**
 * segment_type_to_width() - Map segment classification to Cairo line width
 * @type: segment color classification
 *
 * Returns the geometry-mode line width in Cairo units:
 *   NORMAL / LOADED_RESISTIVITY → 2.0
 *   LOADED                      → 9.0
 *   EXCITATION                  → 5.0
 */
float segment_type_to_width(segment_color_type_t type);

/*-----------------------------------------------------------------------
 * Per-vertex RGB color (float precision, renderer-agnostic)
 *----------------------------------------------------------------------*/
typedef struct
{
  float r, g, b;
} rgb_f_t;

/**
 * color_from_value() - Map a scalar to an rgb_f_t via the rainbow ramp
 * @val: value to map (0..max)
 * @max: normalization maximum; returns black when max <= 0
 *
 * Single point of truth for the Value_to_Color → rgb_f_t conversion used
 * by struct_colors_fill_fstep and ff_presentation_recompute.
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

/*-----------------------------------------------------------------------
 * Per-fstep wire/patch color buffers (Tier 2)
 *
 * Filled by struct_colors_fill_fstep() per frequency step.
 * Dispatch selects which set to pass to backends.
 *----------------------------------------------------------------------*/
typedef struct
{
  rgb_f_t *wire_crnt_rgb;     /* [data.n] wire current magnitude colors */
  rgb_f_t *wire_chrg_rgb;     /* [data.n] wire charge density colors */
  rgb_f_t *patch_crnt_rgb;    /* [data.m] patch current magnitude colors */
  float    wire_crnt_cmin;
  float    wire_crnt_cmax;
  float    wire_chrg_cmin;
  float    wire_chrg_cmax;
  float    patch_crnt_cmin;
  float    patch_crnt_cmax;
} struct_colors_t;

/*-----------------------------------------------------------------------
 * Tier 1 geometry color arrays (file-load time, frequency-independent)
 *----------------------------------------------------------------------*/
extern rgb_f_t *seg_rgb;    /* [data.n] blue/yellow/red per segment type */
extern float   *seg_width;  /* [data.n] Cairo line width per segment type */
extern rgb_f_t *patch_rgb;  /* [data.m] blue constant */

/*-----------------------------------------------------------------------
 * Tier 2 per-fstep color arrays
 *----------------------------------------------------------------------*/
extern struct_colors_t *struct_colors;

/**
 * alloc_struct_colors() - Allocate per-fstep struct_colors array
 * @nfrq: total frequency steps
 */
void alloc_struct_colors(int nfrq);

/**
 * free_struct_colors() - Release struct_colors and geometry color arrays
 */
void free_struct_colors(void);

/**
 * init_geometry_colors() - Compute Tier 1 seg_rgb/patch_rgb at file load
 *
 * Iterates data.n segments via get_segment_color_type() + segment_type_to_rgb().
 * Fills patch_rgb[] with SEG_COLOR_NORMAL blue.
 * Called from Init_Struct_Drawing() after structure_segs is allocated.
 */
void init_geometry_colors(void);

/**
 * struct_colors_fill_fstep() - Compute Tier 2 colors for one fstep
 * @fstep: frequency step index
 *
 * Reads crnt_fstep[fstep] current/charge data.  Scans cmin/cmax.
 * Calls Value_to_Color() for wire current/charge and patch magnitudes.
 */
void struct_colors_fill_fstep(int fstep);

#endif
