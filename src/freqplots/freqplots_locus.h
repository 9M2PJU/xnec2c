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

#ifndef FREQPLOTS_LOCUS_H
#define FREQPLOTS_LOCUS_H  1

#include "../common.h"

/* A locus binds one panel's click-resolution geometry: a primary continuum,
 * the polyline along which a primary click lerps an arbitrary frequency, and a
 * secondary snap set, the discrete existing slots a secondary click selects.
 * XY panels lerp across the full rendered x-axis (a two-point edge continuum)
 * yet snap to the computed samples; the Smith curve serves both roles, so its
 * producer fills the continuum and snap fields with the same curve.  The
 * rendered geometry is the single source for click-to-frequency resolution. */
struct fp_locus_s {
  fp_panel_t    panel;     /* owning panel, for popout resolution */
  GdkRectangle  rect;      /* hit-test bounds */
  GdkPoint     *pts;       /* primary-continuum vertices, n entries (owned) */
  double       *freq;      /* frequency per continuum vertex, n entries (owned) */
  int           n;
  GdkPoint     *snap_pts;  /* secondary snap vertices, snap_n entries (owned) */
  double       *snap_freq; /* frequency per snap vertex, snap_n entries (owned) */
  int           snap_n;
  gboolean      valid;
};

/* Authoritative-input descriptor: one producer fills it with borrowed pointers
 * to its rendered geometry; the registry deep-copies both roles at deposit, so
 * one cohesive object crosses the boundary rather than positional scalars.
 * cont_* is the primary-click continuum, snap_* the secondary-click snap set;
 * both roles are mandatory. */
typedef struct {
  fp_panel_t      panel;
  GdkRectangle    rect;
  const GdkPoint *cont_pts;
  const double   *cont_freq;
  int             cont_n;
  const GdkPoint *snap_pts;
  const double   *snap_freq;
  int             snap_n;
} fp_locus_input_t;

/* Reset the per-view registry for a new frame; buffers persist for reuse. */
void fp_locus_frame_begin(freqplots_view_t *v);

/* Deposit one rendered locus into the view's registry.  in carries both the
 * primary-click continuum and the secondary-click snap set as borrowed
 * pointers, deep-copied here.  cont_n == 0 loci are skipped. */
void fp_locus_add(freqplots_view_t *v, const fp_locus_input_t *in);

/* Resolve a pixel to a frequency on the locus whose rect contains it.  With
 * snap true the nearest vertex; false the nearest-segment projection lerped
 * between bracketing sample frequencies.  FALSE when no locus contains the
 * pixel. */
gboolean fp_locus_freq_at_pixel(freqplots_view_t *v,
    double px, double py, gboolean snap, double *fmhz);

/* Panel owning the locus whose rect contains the pixel, or FP_PANEL_ALL. */
fp_panel_t fp_locus_panel_at(freqplots_view_t *v, double px, double py);

/* Release the registry and every per-locus buffer; zeroes the view state. */
void fp_locus_free(freqplots_view_t *v);

#endif /* FREQPLOTS_LOCUS_H */
