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
 * freqplots_render: Depth-buffered segment and deferred-text accumulator
 * for the frequency-plots Cairo drawing area.
 *
 * Leaf plot renderers deposit line geometry via the fp_add_* helpers and
 * text via fp_defer_text during a frame.  fp_render_flush() strokes the
 * accumulated segments back-to-front through the shared cairo_scenebuffer
 * painter's pipeline, then paints the deferred text so labels overlay all
 * geometry.
 */

#include "freqplots_render.h"
#include "../shared.h"

#include <math.h>
#include <string.h>

/* One deferred text run, painted after all segments are flushed. */
typedef struct
{
  int     x, y;
  int     justify;
  rgb_f_t c;
  char    text[FP_TEXT_LEN];
} fp_text_t;

/* Per-frame accumulators, reused across frames. */
static cairo_scenebuffer_t fp_sb;
static fp_text_t          *fp_texts     = NULL;
static int                 fp_text_count = 0;
static int                 fp_text_cap   = 0;

/*-----------------------------------------------------------------------*/

/**
 * fp_render_reset() - Begin a new frame
 * @fp: render handle to bind to @cr
 * @cr: active Cairo context for this frame
 */
  void
fp_render_reset(fp_render_t *fp, cairo_t *cr)
{
  fp->cr = cr;
  scenebuffer_reset(&fp_sb);
  fp_text_count = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * fp_render_destroy() - Release the segment and text allocations
 */
  void
fp_render_destroy(void)
{
  scenebuffer_destroy(&fp_sb);
  mem_free((void **)&fp_texts);
  fp_text_count = 0;
  fp_text_cap   = 0;
}

/*-----------------------------------------------------------------------*/

/* Append one line segment to the per-frame scenebuffer. */
  static void
fp_add_seg(float z, rgb_f_t c, int x1, int y1, int x2, int y2)
{
  Segment_t s;

  s.x1 = x1;
  s.y1 = y1;
  s.x2 = x2;
  s.y2 = y2;
  s.z_mid = z;
  s.width = FP_LINE_WIDTH;
  seg_set_color(&s, c);

  scenebuffer_add(&fp_sb, &s);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_line(fp_render_t *fp, int x1, int y1, int x2, int y2,
    float z, rgb_f_t c)
{
  (void)fp;
  fp_add_seg(z, c, x1, y1, x2, y2);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_polyline(fp_render_t *fp, const GdkPoint *pts, int n,
    float z, rgb_f_t c)
{
  int i;

  (void)fp;
  for( i = 1; i < n; i++ )
    fp_add_seg(z, c, pts[i-1].x, pts[i-1].y, pts[i].x, pts[i].y);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_quad(fp_render_t *fp, const GdkPoint pts[4], float z, rgb_f_t c)
{
  Segment_t tmpl;
  int xs[4], ys[4], k;

  (void)fp;
  tmpl.z_mid = z;
  tmpl.width = FP_LINE_WIDTH;
  seg_set_color(&tmpl, c);

  for( k = 0; k < 4; k++ )
  {
    xs[k] = pts[k].x;
    ys[k] = pts[k].y;
  }

  scenebuffer_add_polygon_outline(&fp_sb, &tmpl, xs, ys);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_rect(fp_render_t *fp, int x, int y, int w, int h, float z, rgb_f_t c)
{
  GdkPoint p[4];

  p[0].x = x;     p[0].y = y;
  p[1].x = x + w; p[1].y = y;
  p[2].x = x + w; p[2].y = y + h;
  p[3].x = x;     p[3].y = y + h;

  fp_add_quad(fp, p, z, c);
}

/*-----------------------------------------------------------------------*/

/**
 * fp_add_arc() - Sample a circular arc into line segments
 * @cx, @cy: arc centre
 * @r:       arc radius
 * @a0, @a1: start and end angle in radians; sampled linearly from a0 to a1
 *           so a decreasing range yields a negative sweep
 *
 * Stroked arcs cannot enter the segment scenebuffer directly, so the arc
 * is approximated by a polyline whose segment count scales with its angular
 * span to preserve smoothness while keeping the painter's-algorithm layer.
 */
  void
fp_add_arc(fp_render_t *fp, double cx, double cy, double r,
    double a0, double a1, float z, rgb_f_t c)
{
  int nseg, i;
  double px, py;

  (void)fp;
  nseg = (int)( fabs(a1 - a0) / M_2PI * 64.0 + 0.5 );
  if( nseg < 2 ) nseg = 2;

  px = cx + r * cos(a0);
  py = cy + r * sin(a0);
  for( i = 1; i <= nseg; i++ )
  {
    double t = (double)i / (double)nseg;
    double a = a0 + t * (a1 - a0);
    double nx = cx + r * cos(a);
    double ny = cy + r * sin(a);
    fp_add_seg(z, c, (int)(px + 0.5), (int)(py + 0.5),
        (int)(nx + 0.5), (int)(ny + 0.5));
    px = nx;
    py = ny;
  }
}

/*-----------------------------------------------------------------------*/

/* Deposit one opaque filled marker of the given shape onto the per-frame
 * scenebuffer; size carries the square/diamond extent or circle radius. */
  static void
fp_add_marker(float z, rgb_f_t c, int cx, int cy, int size,
    scenebuffer_mark_shape_t shape)
{
  Marker_t m;

  m.x     = cx;
  m.y     = cy;
  m.z_mid = z;
  m.r     = c.r;
  m.g     = c.g;
  m.b     = c.b;
  m.size  = size;
  m.shape = shape;

  scenebuffer_add_marker(&fp_sb, &m);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_filled_square(fp_render_t *fp, int cx, int cy, int size,
    float z, rgb_f_t c)
{
  (void)fp;
  fp_add_marker(z, c, cx, cy, size, SCENEBUFFER_MARK_SQUARE);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_filled_diamond(fp_render_t *fp, int cx, int cy, int size,
    float z, rgb_f_t c)
{
  (void)fp;
  fp_add_marker(z, c, cx, cy, size, SCENEBUFFER_MARK_DIAMOND);
}

/*-----------------------------------------------------------------------*/

  void
fp_add_filled_circle(fp_render_t *fp, int cx, int cy, int radius,
    float z, rgb_f_t c)
{
  (void)fp;
  fp_add_marker(z, c, cx, cy, radius, SCENEBUFFER_MARK_CIRCLE);
}

/*-----------------------------------------------------------------------*/

  void
fp_defer_text(fp_render_t *fp, int x, int y, const char *text,
    int justify, rgb_f_t c)
{
  fp_text_t *t;

  (void)fp;
  if( fp_text_count >= fp_text_cap )
  {
    int new_cap = fp_text_cap ? fp_text_cap * 2 : 64;
    fp_text_t *grown = NULL;
    mem_alloc((void **)&grown, (size_t)new_cap * sizeof(fp_text_t));
    if( fp_text_count )
      memcpy(grown, fp_texts, (size_t)fp_text_count * sizeof(fp_text_t));
    mem_free((void **)&fp_texts);
    fp_texts    = grown;
    fp_text_cap = new_cap;
  }

  t = &fp_texts[fp_text_count++];
  t->x       = x;
  t->y       = y;
  t->justify = justify;
  t->c       = c;
  g_strlcpy(t->text, text, FP_TEXT_LEN);
}

/*-----------------------------------------------------------------------*/

/**
 * fp_render_flush() - Stroke all segments, then paint deferred text on top
 * @fp: render handle bound to the active Cairo context
 *
 * Segments flush through the shared depth-sorted painter's pipeline.  Each
 * deferred text run is then justified against its anchor exactly as the
 * legacy draw_text() did and painted over the flushed geometry.
 */
  void
fp_render_flush(fp_render_t *fp)
{
  cairo_t *cr = fp->cr;
  PangoLayout *layout;
  int i, w, h;

  scenebuffer_flush(&fp_sb, cr, NULL);

  if( fp_text_count == 0 )
    return;

  layout = gtk_widget_create_pango_layout(freqplots_drawingarea, NULL);
  for( i = 0; i < fp_text_count; i++ )
  {
    fp_text_t *t = &fp_texts[i];
    int x = t->x, y = t->y;

    cairo_set_source_rgb(cr, (double)t->c.r, (double)t->c.g, (double)t->c.b);
    pango_layout_set_text(layout, t->text, -1);
    pango_layout_get_pixel_size(layout, &w, &h);

    switch( t->justify & JUSTIFY_HMASK )
    {
      case JUSTIFY_RIGHT:  x -= w;     break;
      case JUSTIFY_CENTER: x -= w / 2; break;
      case JUSTIFY_LEFT:
      default:                         break;
    }

    switch( t->justify & JUSTIFY_VMASK )
    {
      case JUSTIFY_ABOVE:  y -= h;     break;
      case JUSTIFY_MIDDLE: y -= h / 2; break;
      case JUSTIFY_BELOW:
      default:                         break;
    }

    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);
  }
  g_object_unref(layout);
}

/*-----------------------------------------------------------------------*/

/**
 * pango_text_size() - Measure a string in the freqplots drawing-area font
 * @widget: unused; the layout is always created against freqplots_drawingarea
 *          so all plot text shares one font metric source
 */
  void
pango_text_size(GtkWidget *widget, int *width, int *height, char *s)
{
  PangoLayout *layout;

  (void)widget;
  layout = gtk_widget_create_pango_layout(freqplots_drawingarea, s);
  pango_layout_get_pixel_size(layout, width, height);
  g_object_unref(layout);
}
