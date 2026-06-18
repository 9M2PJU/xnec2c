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

/* Smith-chart background grid, expressed as normalised resistance and
 * reactance values.  Each constant-resistance value yields a full circle and
 * each constant-reactance value a mirrored arc pair; the geometry derives
 * from the value so positions stay exact rather than approximated. */

static const rgb_f_t SMITH_C_R     = { 0.40f, 0.52f, 0.72f };
static const rgb_f_t SMITH_C_X     = { 0.72f, 0.52f, 0.40f };
static const rgb_f_t SMITH_C_UNITY = { 0.88f, 0.84f, 0.45f };
static const rgb_f_t SMITH_C_AXIS  = { 0.62f, 0.62f, 0.62f };
static const rgb_f_t SMITH_C_PERIM = { 0.55f, 0.55f, 0.62f };
static const rgb_f_t SMITH_C_WTG   = { 0.50f, 0.62f, 0.55f };

#define SMITH_W_MINOR  1.0f
#define SMITH_W_MAJOR  1.6f
#define SMITH_W_AXIS   2.0f
#define SMITH_W_LOCUS  1.0f

/* Smith-chart paint order (ascending z paints later, hence on top):
 *   constant-X arcs < constant-R circles < unity/major grid and real axis <
 *   text labels (FP_Z_TEXT) < locus (FP_Z_LEFT) < current-frequency marker
 *   (FP_Z_GREEN).  The three grid sublayers occupy the band reserved between
 *   FP_Z_GRID and FP_Z_TEXT. */
#define SMITH_Z_X      (FP_Z_GRID + 1.0f)
#define SMITH_Z_R      (FP_Z_GRID + 2.0f)
#define SMITH_Z_MAJOR  (FP_Z_GRID + 3.0f)

#define SMITH_MARK_STEP  4
#define SMITH_LABEL_GAP  10.0

typedef struct {
  double          value;   /* normalised resistance or |reactance| */
  const rgb_f_t  *color;
  float           width;
  float           depth;   /* painter's-algorithm layer; see paint order above */
  gboolean        label;
} smith_grid_t;

static const smith_grid_t smith_r_grid[] = {
  { 0.0, &SMITH_C_AXIS,  SMITH_W_AXIS,  SMITH_Z_MAJOR, FALSE },
  { 0.2, &SMITH_C_R,     SMITH_W_MINOR, SMITH_Z_R,     TRUE  },
  { 0.5, &SMITH_C_R,     SMITH_W_MINOR, SMITH_Z_R,     TRUE  },
  { 1.0, &SMITH_C_UNITY, SMITH_W_MAJOR, SMITH_Z_MAJOR, FALSE },
  { 2.0, &SMITH_C_R,     SMITH_W_MINOR, SMITH_Z_R,     TRUE  },
  { 5.0, &SMITH_C_R,     SMITH_W_MINOR, SMITH_Z_R,     TRUE  },
};

static const smith_grid_t smith_x_grid[] = {
  { 0.2, &SMITH_C_X,     SMITH_W_MINOR, SMITH_Z_X,     FALSE },
  { 0.5, &SMITH_C_X,     SMITH_W_MINOR, SMITH_Z_X,     TRUE  },
  { 1.0, &SMITH_C_UNITY, SMITH_W_MAJOR, SMITH_Z_MAJOR, TRUE  },
  { 2.0, &SMITH_C_X,     SMITH_W_MINOR, SMITH_Z_X,     TRUE  },
  { 5.0, &SMITH_C_X,     SMITH_W_MINOR, SMITH_Z_X,     TRUE  },
};

/* Per-graph density resolved once at graph entry: stroke widths halve when
 * panels share vertical space; text shrinks 1/n so stacked charts keep
 * proportional labels. */
typedef struct { float stroke; float text; } fp_density_t;

  static fp_density_t
fp_density_for( int ngraph )
{
  return (fp_density_t){ ngraph > 1 ? 0.5f : 1.0f, 1.0f / (float)ngraph };
}

/* smith_polar()
 *
 * Screen point on a chart circle: centre (cx, cy), radius in pixels, angle in
 * degrees counter-clockwise from the positive real axis.  Screen y inverts so
 * a positive angle rises into the upper half.  Single placement primitive for
 * every label that rides a circular locus.
 */
  static void
smith_polar( double cx, double cy, double radius, double deg, int *px, int *py )
{
  double a = deg * (M_PI / 180.0);

  *px = (int)( cx + radius * cos(a) );
  *py = (int)( cy - radius * sin(a) );

} /* smith_polar() */

/* smith_draw_resistance()
 *
 * Deposits one constant-resistance circle and its bold value label.  The
 * circle centres at re = r/(r+1) with radius 1/(r+1) in the normalised
 * reflection plane.  The label rides a glyph-width and a half outside its own
 * circle at the angle of a capacitive band point so it clears the circle
 * stroke as well as the real axis and the other grid lines: r below 3 lands
 * between the -j0.5 and -j1 arcs, r of 5 lands between the -j1 and -j2 arcs.
 */
  static void
smith_draw_resistance( fp_render_t *fp, int x0, int y0, double half,
    double dw, const smith_grid_t *g, fp_density_t density )
{
  double cx  = x0 + ( g->value / (g->value + 1.0) ) * half;
  double rad = half / (g->value + 1.0);

  fp_add_arc( fp, cx, y0, rad, 0.0, M_2PI,
      (fp_stroke_t){ .color = *g->color, .width = g->width * density.stroke,
                     .z_mid = g->depth } );

  if( g->label )
  {
    char   buf[16];
    double band = ( g->value < 3.0 ) ? -0.75 : -1.5;
    double u0   = g->value / (g->value + 1.0);
    double gre, gim;
    int    lx, ly;

    Calculate_Smith( g->value, band, 1.0, &gre, &gim );
    smith_polar( cx, y0, rad + 1.5 * dw,
        atan2( gim, gre - u0 ) * (180.0 / M_PI), &lx, &ly );

    g_snprintf( buf, sizeof(buf), "%g", g->value );
    fp_add_text(fp, lx, ly, density.text, buf,
                JUSTIFY_CENTER | JUSTIFY_MIDDLE | JUSTIFY_BOLD, *g->color);
  }

} /* smith_draw_resistance() */

/* smith_draw_reactance()
 *
 * Deposits a mirrored constant-reactance arc pair and its outer-circle value
 * labels.  Both arcs converge at Gamma = 1; each centres one radius off the
 * real axis with radius 1/x.  The arc terminates where it meets the unit
 * circle, the end angle computed exactly from x to avoid edge mismatch.
 */
  static void
smith_draw_reactance( fp_render_t *fp, int x0, int y0, double half,
    double dw, const smith_grid_t *g, fp_density_t density )
{
  double x   = g->value;
  double rad = half / x;
  double cx  = x0 + half;
  double end = M_PI + atan2( (x*x - 1.0) / (x * (x*x + 1.0)),
                             2.0 / (x*x + 1.0) );
  fp_stroke_t s = { .color = *g->color, .width = g->width * density.stroke,
                    .z_mid = g->depth };

  fp_add_arc( fp, cx, y0 - rad, rad, M_PI / 2.0, end, s );

  /* The mirror twin spans the same angular extent below the real axis.
   * Arc angles follow cairo_arc's increasing-sweep convention, so the
   * capacitive arc orders its endpoints low-to-high (-end < -M_PI/2). */
  fp_add_arc( fp, cx, y0 + rad, rad, -end, -M_PI / 2.0, s );

  if( g->label )
  {
    char   buf[16];
    double dir_re = (x*x - 1.0) / (x*x + 1.0);  /* unit-circle landing direction */
    double dir_im = 2.0 * x / (x*x + 1.0);
    double deg = atan2( dir_im, dir_re ) * (180.0 / M_PI);
    int    ux, uy, lx, ly;

    /* Inductive label rides the rim at the landing angle; the capacitive
     * twin mirrors it across the real axis. */
    smith_polar( x0, y0, half + SMITH_LABEL_GAP,  deg, &ux, &uy );
    smith_polar( x0, y0, half + SMITH_LABEL_GAP, -deg, &lx, &ly );

    g_snprintf( buf, sizeof(buf), "j%g", x );
    fp_add_text(fp, ux, uy, density.text, buf,
                JUSTIFY_CENTER | JUSTIFY_MIDDLE, *g->color);
    g_snprintf( buf, sizeof(buf), "-j%g", x );
    fp_add_text(fp, lx - (int)(0.5 * dw), ly, density.text, buf,
                JUSTIFY_CENTER | JUSTIFY_MIDDLE, *g->color);
  }

} /* smith_draw_reactance() */

/* smith_draw_perimeter()
 *
 * Deposits the two outer-rim scales in the reserved annulus: the reflection-
 * coefficient angle (ticks every 10°, degree labels every 30°) and the
 * wavelengths-toward-generator scale (0 at the short, clockwise, 0.5λ per
 * turn).  Screen y is inverted so positive reactance reads in the upper half.
 */
  static void
smith_draw_perimeter( fp_render_t *fp, int x0, int y0, double half, double rh,
    fp_density_t density )
{
  rgb_f_t  perim_c = SMITH_C_PERIM;
  rgb_f_t  wtg_c   = SMITH_C_WTG;
  int      deg;
  int      k;

  for( deg = 0; deg < 360; deg += 10 )
  {
    double   rad   = deg * M_PI / 180.0;
    double   c     = cos( rad );
    double   s     = sin( rad );
    gboolean major = ( deg % 30 == 0 );
    double   t1    = half + ( major ? 7.0 : 4.0 );
    int      x1    = x0 + (int)( c * half );
    int      y1    = y0 - (int)( s * half );
    int      x2    = x0 + (int)( c * t1 );
    int      y2    = y0 - (int)( s * t1 );

    fp_add_line( fp, x1, y1, x2, y2,
        (fp_stroke_t){ .color = perim_c, .width = FP_LINE_WIDTH, .z_mid = SMITH_Z_MAJOR } );

    /* The 0 Ω and ∞ Ω endpoints own the horizontal axis terminals */
    if( major && deg != 0 && deg != 180 )
    {
      char buf[16];
      int  lx, ly;
      int  signed_deg = ( deg <= 180 ) ? deg : deg - 360;

      smith_polar( x0, y0, half + SMITH_LABEL_GAP + 0.75 * rh,
          (double)deg, &lx, &ly );

      g_snprintf( buf, sizeof(buf), "%d°", signed_deg );
      fp_add_text(fp, lx, ly, density.text, buf,
                  JUSTIFY_CENTER | JUSTIFY_MIDDLE, perim_c);
    }
  }

  for( k = 0; k < 10; k++ )
  {
    double w   = k * 0.05;
    double phi = 180.0 - w * 720.0;
    int    lx, ly;
    char   buf[16];

    /* Skip the axis-coincident marks; the 0 Ω and ∞ Ω endpoints own those */
    if( ((int)phi) % 180 == 0 )
      continue;

    smith_polar( x0, y0, half + SMITH_LABEL_GAP + 0.75 * rh, phi, &lx, &ly );

    g_snprintf( buf, sizeof(buf), "%.2f", w );
    fp_add_text(fp, lx, ly, density.text, buf,
                JUSTIFY_CENTER | JUSTIFY_MIDDLE, wtg_c);
  }

} /* smith_draw_perimeter() */

/* smith_draw_grid()
 *
 * Lays down the full Smith-chart background: the real axis, every constant-
 * resistance circle and constant-reactance arc from the grid tables, and the
 * matched-point marker at the chart centre.
 */
  static void
smith_draw_grid( fp_render_t *fp, GtkWidget *area, int x0, int y0, int scale, fp_density_t density )
{
  double  half   = scale / 2.0;
  rgb_f_t axis_c = SMITH_C_AXIS;
  int     dw_px, dh_px;
  int     ex, ey;
  double  dw;
  size_t  i;

  pango_text_size( area, &dw_px, &dh_px, "0" );
  dw = (double)dw_px;

  fp_add_line( fp, x0 - (int)half, y0, x0 + (int)half, y0,
      (fp_stroke_t){ .color = axis_c, .width = FP_LINE_WIDTH, .z_mid = SMITH_Z_MAJOR } );

  for( i = 0; i < G_N_ELEMENTS(smith_r_grid); i++ )
    smith_draw_resistance( fp, x0, y0, half, dw, &smith_r_grid[i], density );

  for( i = 0; i < G_N_ELEMENTS(smith_x_grid); i++ )
    smith_draw_reactance( fp, x0, y0, half, dw, &smith_x_grid[i], density );

  /* Bright matched-point dot at the chart centre */
  fp_add_filled_circle( fp, x0, y0, 3, SMITH_Z_MAJOR, COLOR_WHITE );

  /* Real-axis endpoints: left is a short (0 Ω), right an open (∞ Ω) */
  smith_polar( x0, y0, half + SMITH_LABEL_GAP, 180.0, &ex, &ey );
  fp_add_text(fp, ex, ey, density.text, _("0 Ω"),
              JUSTIFY_RIGHT | JUSTIFY_MIDDLE, axis_c);
  smith_polar( x0, y0, half + SMITH_LABEL_GAP, 0.0, &ex, &ey );
  fp_add_text(fp, ex, ey, density.text, _("∞ Ω"),
              JUSTIFY_LEFT | JUSTIFY_MIDDLE, axis_c);

  smith_draw_perimeter( fp, x0, y0, half, (double)dh_px, density );

} /* smith_draw_grid() */

/* smith_draw_legend()
 *
 * Stacks a colour key upward from the lower-left plot corner so the
 * constant-resistance circles, constant-reactance arcs, impedance locus, and
 * rim scales can be told apart without prior Smith-chart familiarity.  Entries
 * read top to bottom; each row pairs a colour swatch with its description so
 * the swatches and the text align in two columns.
 */
  static void
smith_draw_legend( fp_render_t *fp, int x, int y_bottom, int line_h, float scale )
{
  struct { rgb_f_t color; const char *text; } key[] = {
    { SMITH_C_R,     _("constant R") },
    { SMITH_C_X,     _("constant X") },
    { SMITH_C_UNITY, _("unity R, X") },
    { COLOR_WHITE,   _("match (z = 1)") },
    { COLOR_MAGENTA, _("impedance vs freq") },
    { COLOR_GREEN,   _("selected frequency") },
    { SMITH_C_PERIM, _("reflection angle (°)") },
    { SMITH_C_WTG,   _("wavelengths → gen") },
  };
  size_t n  = G_N_ELEMENTS(key);
  int    sw = line_h / 2;            /* swatch half-extent */
  int    tx = x + line_h + 4;        /* text column, clear of the swatch */
  size_t i;

  for( i = 0; i < n; i++ )
  {
    int row_y = y_bottom - (int)(n - 1 - i) * line_h;

    /* Swatch centred on the text row's vertical midpoint */
    fp_add_filled_square( fp, x + sw, row_y - sw, sw, FP_Z_LEFT, key[i].color );
    fp_add_text(fp, tx, row_y, scale, key[i].text,
                JUSTIFY_LEFT | JUSTIFY_ABOVE, key[i].color);
  }

} /* smith_draw_legend() */

/*-----------------------------------------------------------------------*/

/* Plot_Graph_Smith()
 *
 * Plots graphs of two functions against a common variable
 */
  void
Plot_Graph_Smith(
	freqplots_view_t *v, fp_render_t *fp,
	double *fa, double *fb, double *fc,
	int nc, int *card_nfsteps, int posn )
{
  int plot_height, plot_y_position;
  int idx;
  GdkPoint *points = NULL;
  int scale, x0, y0, x, y;
  int ohm_w, ohm_h;
  double re, im;
  char z0buf[24];

  GdkRectangle plot_rect;

  /* Pango layout size */
  static int layout_width, layout_height, width1, height;

  pango_text_size(v->drawingarea, &layout_width, &layout_height, "000000");

  /* Horizontal margin sized to the real-axis endpoint label plus a small gap
   * so the chart fills the width while ∞ Ω and 0 Ω stay off the window edge */
  pango_text_size(v->drawingarea, &ohm_w, &ohm_h, _("∞ Ω"));

  /* Available height for each graph.
   * (np=number of graphs to be plotted) */
  plot_height = v->height / v->ngraph;
  plot_y_position   = ( v->height * posn) / v->ngraph;

  /* Plot box rectangle */
  plot_rect.x = ohm_w + 2;
  plot_rect.y = plot_y_position + 2;
  plot_rect.width = v->width - 2 * ( ohm_w + 2 );
  plot_rect.height = plot_height - 8 - 2 * layout_height;

  /* Reserve vertical space for the chart title row */
  pango_text_size(v->drawingarea, &width1, &height, _("Smith Chart") );
  plot_rect.y += height;

  x0 = plot_rect.x + plot_rect.width  / 2;
  y0 = plot_rect.y + plot_rect.height / 2;
  scale = plot_rect.width;
  if( scale > plot_rect.height )
      scale = plot_rect.height;

  /* Reserve an annulus outside the unit circle for the perimeter scales */
  scale -= 6 * layout_height;

  /* Per-graph density resolved once: stroke widths halve and label sizes
   * shrink 1/n so stacked charts stay legible and proportional. */
  fp_density_t density = fp_density_for( v->ngraph );

  /* Draw smith background grid */
  smith_draw_grid( fp, v->drawingarea, x0, y0, scale, density );

  /* Corner overlays annotate the plot frame rather than the chart geometry,
   * so they hold base size regardless of how many graphs share the window;
   * only the in-chart grid labels follow the 1/n density scale. */
  const float annot_text = 1.0f;

  /* Normalising-impedance annotation */
  g_snprintf( z0buf, sizeof(z0buf), _("Z0 = %g Ω"), calc_data.zo );
  fp_add_text(fp, plot_rect.x, plot_rect.y, annot_text, z0buf,
              JUSTIFY_LEFT | JUSTIFY_BELOW, COLOR_WHITE);

  /* Reactance-sign reminders in the clear plot corners */
  fp_add_text(fp, plot_rect.x + plot_rect.width, plot_rect.y, annot_text,
              _("INDUCTIVE (+jX)"), JUSTIFY_RIGHT | JUSTIFY_BELOW, SMITH_C_X);
  fp_add_text(fp, plot_rect.x + plot_rect.width,
              plot_rect.y + plot_rect.height, annot_text,
              _("CAPACITIVE (-jX)"), JUSTIFY_RIGHT | JUSTIFY_ABOVE, SMITH_C_X);

  /* Colour key in the lower-left corner; swatch and row spacing track the
   * base label height */
  smith_draw_legend( fp, plot_rect.x,
      plot_rect.y + plot_rect.height, layout_height, annot_text );

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
    fp_add_filled_square( fp, points[idx].x, points[idx].y, SMITH_MARK_STEP,
        FP_Z_LEFT, COLOR_MAGENTA );
  }

  /* Draw one locus per FR card so the trace does not jump between the
   * disjoint frequency ranges of adjacent cards.  card_nfsteps partitions the
   * flat point array into the same contiguous per-card blocks the XY plots
   * use, summing to nc. */
  int card, coff = 0;
  for( card = 0; card < calc_data.FR_cards; card++ )
  {
    int cn = card_nfsteps[card];
    if( cn > 0 )
      fp_add_polyline( fp, points + coff, cn,
          (fp_stroke_t){ .color = COLOR_MAGENTA, .width = SMITH_W_LOCUS * density.stroke,
                         .z_mid = FP_Z_LEFT } );
    coff += cn;
  }

  /* Capture the locus geometry so a chart click resolves to a frequency */
  v->smith_locus_valid = FALSE;
  if( nc > 0 )
  {
    mem_realloc((void **)&v->smith_locus_pts,  (size_t)nc * sizeof(GdkPoint));
    mem_realloc((void **)&v->smith_locus_freq, (size_t)nc * sizeof(double));
    memcpy( v->smith_locus_pts,  points, (size_t)nc * sizeof(GdkPoint) );
    memcpy( v->smith_locus_freq, fc,     (size_t)nc * sizeof(double) );
    v->smith_locus_rect  = plot_rect;
    v->smith_locus_n     = nc;
    v->smith_locus_valid = TRUE;
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

/* fp_smith_hit()
 *
 * True when the Smith chart was drawn and (px,py) lies within its bounding
 * rectangle.  Sole owner of the chart hit-test, shared by the frequency
 * resolver and the double-click popout query.
 */
  gboolean
fp_smith_hit( freqplots_view_t *v, double px, double py )
{
  if( !v->smith_locus_valid || v->smith_locus_n <= 0 )
    return FALSE;

  return px >= v->smith_locus_rect.x
      && px <= v->smith_locus_rect.x + v->smith_locus_rect.width
      && py >= v->smith_locus_rect.y
      && py <= v->smith_locus_rect.y + v->smith_locus_rect.height;
}

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
fp_smith_freq_at_pixel( freqplots_view_t *v, double px, double py, gboolean snap_to_step, double *fmhz )
{
  int idx;

  if( !fp_smith_hit(v, px, py) )
    return FALSE;

  /* Secondary click: nearest sweep-step vertex by pixel distance */
  if( snap_to_step )
  {
    int    best = 0;
    double dx = px - v->smith_locus_pts[0].x;
    double dy = py - v->smith_locus_pts[0].y;
    double best_d2 = dx * dx + dy * dy;

    for( idx = 1; idx < v->smith_locus_n; idx++ )
    {
      dx = px - v->smith_locus_pts[idx].x;
      dy = py - v->smith_locus_pts[idx].y;
      double d2 = dx * dx + dy * dy;
      if( dl_flt(d2, best_d2) )
      {
        best_d2 = d2;
        best = idx;
      }
    }

    *fmhz = v->smith_locus_freq[best];
    return TRUE;
  }

  /* Single point: no segment to project onto */
  if( v->smith_locus_n == 1 )
  {
    *fmhz = v->smith_locus_freq[0];
    return TRUE;
  }

  /* Primary click: project onto the nearest locus segment and interpolate
   * the bracketing step frequencies by the clamped projection fraction */
  int    best_i = 0;
  double best_t = 0.0;
  double best_d2 = 0.0;

  for( idx = 0; idx < v->smith_locus_n - 1; idx++ )
  {
    double ax = v->smith_locus_pts[idx].x;
    double ay = v->smith_locus_pts[idx].y;
    double vx = v->smith_locus_pts[idx + 1].x - ax;
    double vy = v->smith_locus_pts[idx + 1].y - ay;
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

  *fmhz = v->smith_locus_freq[best_i]
      + best_t * ( v->smith_locus_freq[best_i + 1] - v->smith_locus_freq[best_i] );

  return TRUE;

} /* fp_smith_freq_at_pixel() */
