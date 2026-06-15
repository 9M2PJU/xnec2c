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

/*
 * smith_graph: Smith-chart frequency plot type.
 *
 * The reactance/resistance background arcs are sampled into grid-layer
 * segments, the impedance locus and its markers deposit on the magenta
 * layer, and the current-frequency marker on the top layer.
 */

#include "../freqplots_internal.h"
#include "../../shared.h"

#include <math.h>
#include <string.h>

/* Adjacent locus step indices spanning a frequency, with the
 * interpolation fraction between them. */
typedef struct {
  int    lo;
  int    hi;
  double frac;
} fp_locus_bracket_t;

/* Impedance locus geometry captured on each render so a click on the chart
 * can resolve to a frequency.  The rectangle hit-tests the click; the per-step
 * pixel points and their frequencies map a pixel onto the nearest locus
 * position. */
static GdkRectangle smith_locus_rect;
static GdkPoint    *smith_locus_pts = NULL;
static double      *smith_locus_freq = NULL;
static int          smith_locus_n = 0;
static gboolean     smith_locus_valid = FALSE;

  static void
Calculate_Smith( double zr, double zi, double z0, double *re, double *im )
{
  complex double z = zr / z0 + I * zi / z0;
  complex double r = ( z - 1.0 ) / ( z + 1.0 );
  *re = creal( r );
  *im = cimag( r );
}

/*-----------------------------------------------------------------------*/

/* fp_locus_bracket()
 *
 * Resolves the two adjacent locus step indices spanning fmhz in the
 * per-step frequency axis, with the interpolation fraction between them.
 * Clamps to an endpoint outside the range; falls back to the nearest
 * index for a non-monotonic axis (e.g. overlapping FR cards).
 */
  static fp_locus_bracket_t
fp_locus_bracket( const double *freq, int nc, double fmhz )
{
  fp_locus_bracket_t bracket = { 0, 0, 0.0 };
  int    idx;
  int    nearest;
  double best_diff;

  if( nc <= 1 )
    return bracket;

  /* Clamp below the first locus point */
  if( !dl_fgt(fmhz, freq[0]) )
    return bracket;

  /* Clamp above the last locus point */
  if( !dl_flt(fmhz, freq[nc - 1]) )
  {
    bracket.lo = nc - 1;
    bracket.hi = nc - 1;
    return bracket;
  }

  /* Scan for the in-order span bracketing fmhz */
  for( idx = 0; idx < nc - 1; idx++ )
  {
    double lo_f = freq[idx];
    double hi_f = freq[idx + 1];

    if( !dl_fgt(lo_f, fmhz) && !dl_fgt(fmhz, hi_f) )
    {
      double denom = hi_f - lo_f;

      bracket.lo = idx;
      bracket.hi = idx + 1;
      if( dl_fgt(denom, 0.0) )
          bracket.frac = (fmhz - lo_f) / denom;
      return bracket;
    }
  }

  /* Non-monotonic fallback: nearest index by absolute frequency difference */
  nearest = 0;
  best_diff = fabs( fmhz - freq[0] );
  for( idx = 1; idx < nc; idx++ )
  {
    double diff = fabs( fmhz - freq[idx] );
    if( dl_flt(diff, best_diff) )
    {
      best_diff = diff;
      nearest = idx;
    }
  }
  bracket.lo = nearest;
  bracket.hi = nearest;
  return bracket;

} /* fp_locus_bracket() */

/*-----------------------------------------------------------------------*/

/* Plot_Graph_Smith()
 *
 * Plots graphs of two functions against a common variable
 */
  void
Plot_Graph_Smith(
	fp_render_t *fp,
	double *fa, double *fb, double *fc,
	int nc, int posn )
{
  int plot_height, plot_y_position;
  int idx;
  GdkPoint *points = NULL;
  int scale, x0, y0, x, y;
  double re, im;

  GdkRectangle plot_rect;

  /* Pango layout size */
  static int layout_width, layout_height, width1, height;

  pango_text_size(freqplots_drawingarea, &layout_width, &layout_height, "000000");

  /* Available height for each graph.
   * (np=number of graphs to be plotted) */
  plot_height = freqplots_height / calc_data.ngraph;
  plot_y_position   = ( freqplots_height * posn) / calc_data.ngraph;

  /* Plot box rectangle */
  plot_rect.x = layout_width + 4;
  plot_rect.y = plot_y_position + 2;
  plot_rect.width = freqplots_width - 8 - 2 * layout_width;
  plot_rect.height = plot_height - 8 - 2 * layout_height;

  /* Reserve vertical space for the chart title row */
  pango_text_size(freqplots_drawingarea, &width1, &height, _("Smith Chart") );
  plot_rect.y += height;

  x0 = plot_rect.x + plot_rect.width  / 2;
  y0 = plot_rect.y + plot_rect.height / 2;
  scale = plot_rect.width;
  if( scale > plot_rect.height )
      scale = plot_rect.height;

  /* Draw smith background */
  for( idx = 0; idx < 3; idx++ )
  {
       int div = 2 << idx;
       double angle[] = { 1.0, 1.2, 1.35 };
       fp_add_arc( fp, x0 + scale / 2 - scale / div, y0, scale / div,
           0, M_2PI, FP_Z_GRID, COLOR_GREY );
       fp_add_arc( fp, x0 + scale / 2, y0 - scale / div, scale / div,
           M_PI / 2.0, M_PI * angle[idx], FP_Z_GRID, COLOR_GREY );
       fp_add_arc( fp, x0 + scale / 2, y0 + scale / div, scale / div,
           M_PI * 1.5, ( 2.0 - angle[idx]) * M_PI, FP_Z_GRID, COLOR_GREY );
  }

  /* Calculate points to plot */
  mem_alloc((void **)&points, (size_t)nc * sizeof(GdkPoint));

  if( points == NULL )
  {
    pr_err("memory allocation for points failed\n");
    Stop( ERR_OK, _("Draw_Graph():"
          "Memory allocation for points failed") );
    return;
  }

  for( idx = 0; idx < nc; idx++ )
  {
    Calculate_Smith( fa[idx], fb[idx], calc_data.zo, &re, &im );

    // flip plot vertically because negative imaginary is the bottom half
    im = -im;

    points[idx].x = x0 + (gint)( re * scale / 2 );
    points[idx].y = y0 + (gint)( im * scale / 2 );
    fp_add_filled_square( fp, points[idx].x, points[idx].y, 6,
        FP_Z_LEFT, COLOR_MAGENTA );
  }

  /* Draw the graph */
  fp_add_polyline( fp, points, nc, FP_Z_LEFT, COLOR_MAGENTA );

  /* Capture the locus geometry so a chart click resolves to a frequency */
  smith_locus_valid = FALSE;
  if( nc > 0 )
  {
    mem_realloc((void **)&smith_locus_pts,  (size_t)nc * sizeof(GdkPoint));
    mem_realloc((void **)&smith_locus_freq, (size_t)nc * sizeof(double));
    memcpy( smith_locus_pts,  points, (size_t)nc * sizeof(GdkPoint) );
    memcpy( smith_locus_freq, fc,     (size_t)nc * sizeof(double) );
    smith_locus_rect  = plot_rect;
    smith_locus_n     = nc;
    smith_locus_valid = TRUE;
  }

  mem_free((void **)&points);

  /* Draw a vertical line to show current freq if it was
   * changed by a user click on the plots drawingarea */
  if( calc_data.fmhz_save > 0.0 )
  {
    rgb_f_t green_c = isFlagSet(SY_OPTIMIZER_ACTIVE)
        ? COLOR_DARK_GREEN : COLOR_GREEN;

    /* Interpolate the completed sweep locus at the click frequency so the
     * marker tracks the click without waiting on a pending solve */
    fp_locus_bracket_t bracket = fp_locus_bracket( fc, nc, calc_data.fmhz_save );
    double zr = fa[bracket.lo] + (fa[bracket.hi] - fa[bracket.lo]) * bracket.frac;
    double zi = fb[bracket.lo] + (fb[bracket.hi] - fb[bracket.lo]) * bracket.frac;

    Calculate_Smith( zr, zi, calc_data.zo, &re, &im );

    // flip plot vertically because negative imaginary is the bottom half
    im = -im;

    x = x0 + (gint)( re * scale / 2 );
    y = y0 + (gint)( im * scale / 2 );
    fp_add_filled_square( fp, x, y, 8, FP_Z_GREEN, green_c );
  }

} /* Plot_Graph_Smith() */

/*-----------------------------------------------------------------------*/

/* fp_smith_freq_at_pixel()
 *
 * Resolves a click on the Smith chart to a frequency on the impedance locus
 * captured during the last render.  Returns FALSE when the chart was not
 * drawn or the click lies outside its bounding rectangle.  snap_to_step
 * picks the nearest sweep-step vertex; otherwise the click is projected onto
 * the nearest locus segment and the bracketing step frequencies are
 * interpolated by the projection fraction.
 */
  gboolean
fp_smith_freq_at_pixel( double px, double py, gboolean snap_to_step, double *fmhz )
{
  int idx;

  if( !smith_locus_valid || smith_locus_n <= 0 )
    return FALSE;

  if(    px < smith_locus_rect.x
      || px > smith_locus_rect.x + smith_locus_rect.width
      || py < smith_locus_rect.y
      || py > smith_locus_rect.y + smith_locus_rect.height )
    return FALSE;

  /* Secondary click: nearest sweep-step vertex by pixel distance */
  if( snap_to_step )
  {
    int    best = 0;
    double dx = px - smith_locus_pts[0].x;
    double dy = py - smith_locus_pts[0].y;
    double best_d2 = dx * dx + dy * dy;

    for( idx = 1; idx < smith_locus_n; idx++ )
    {
      dx = px - smith_locus_pts[idx].x;
      dy = py - smith_locus_pts[idx].y;
      double d2 = dx * dx + dy * dy;
      if( dl_flt(d2, best_d2) )
      {
        best_d2 = d2;
        best = idx;
      }
    }

    *fmhz = smith_locus_freq[best];
    return TRUE;
  }

  /* Single point: no segment to project onto */
  if( smith_locus_n == 1 )
  {
    *fmhz = smith_locus_freq[0];
    return TRUE;
  }

  /* Primary click: project onto the nearest locus segment and interpolate
   * the bracketing step frequencies by the clamped projection fraction */
  int    best_i = 0;
  double best_t = 0.0;
  double best_d2 = 0.0;

  for( idx = 0; idx < smith_locus_n - 1; idx++ )
  {
    double ax = smith_locus_pts[idx].x;
    double ay = smith_locus_pts[idx].y;
    double vx = smith_locus_pts[idx + 1].x - ax;
    double vy = smith_locus_pts[idx + 1].y - ay;
    double len2 = vx * vx + vy * vy;
    double t = 0.0;

    if( dl_fgt(len2, 0.0) )
        t = ( (px - ax) * vx + (py - ay) * vy ) / len2;

    if( t < 0.0 ) t = 0.0;
    else if( t > 1.0 ) t = 1.0;

    double cx = ax + t * vx;
    double cy = ay + t * vy;
    double dx = px - cx;
    double dy = py - cy;
    double d2 = dx * dx + dy * dy;

    if( idx == 0 || dl_flt(d2, best_d2) )
    {
      best_d2 = d2;
      best_i  = idx;
      best_t  = t;
    }
  }

  *fmhz = smith_locus_freq[best_i]
      + best_t * ( smith_locus_freq[best_i + 1] - smith_locus_freq[best_i] );

  return TRUE;

} /* fp_smith_freq_at_pixel() */
