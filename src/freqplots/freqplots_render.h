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

#ifndef FREQPLOTS_RENDER_H
#define FREQPLOTS_RENDER_H  1

#include "../common.h"
#include "../cairo/cairo_scenebuffer.h"

/* Painter's-algorithm depth layers for the frequency plots.  Ascending
 * z_mid is drawn back-to-front by scenebuffer_flush(), so a larger value
 * paints on top.  Grid sits furthest back, the right (cyan) trace next,
 * the left (magenta) trace above it, and the current-frequency line on
 * top of all segment geometry.  Deferred text paints after the flush and
 * therefore overlays every layer. */
#define FP_Z_GRID   0.0f
#define FP_Z_RIGHT  1.0f
#define FP_Z_LEFT   2.0f
#define FP_Z_GREEN  3.0f

/* Every freq-plot stroke uses the GTK draw context default line width. */
#define FP_LINE_WIDTH  2.0f

/* Maximum deferred-text length (titles, scale values, min/max labels). */
#define FP_TEXT_LEN  64

/* Text justification, calculated against the supplied x (horizontal) and
 * y (vertical) anchor coordinates. */
enum {
  JUSTIFY_LEFT   = 0,
  JUSTIFY_CENTER = 1,
  JUSTIFY_RIGHT  = 2,
  JUSTIFY_HMASK  = 0x03,

  JUSTIFY_BELOW  = 0,
  JUSTIFY_MIDDLE = 1 << 2,
  JUSTIFY_ABOVE  = 2 << 2,
  JUSTIFY_VMASK  = 0x0c
};

/* Per-frame render handle carrying the active Cairo context.  Segment and
 * deferred-text storage are owned by freqplots_render.c. */
typedef struct
{
  cairo_t *cr;
} fp_render_t;

/* Lifecycle: reset clears the scenebuffer and deferred-text list for a new
 * frame; flush depth-sorts and strokes all segments then paints deferred
 * text on top; destroy releases the backing allocations. */
void fp_render_reset(fp_render_t *fp, cairo_t *cr);
void fp_render_flush(fp_render_t *fp);
void fp_render_destroy(void);

/* Segment deposit helpers.  Colour and depth are resolved by the caller;
 * each helper appends line geometry to the per-frame scenebuffer. */
void fp_add_line(fp_render_t *fp, int x1, int y1, int x2, int y2,
    float z, rgb_f_t c);
void fp_add_polyline(fp_render_t *fp, const GdkPoint *pts, int n,
    float z, rgb_f_t c);
void fp_add_rect(fp_render_t *fp, int x, int y, int w, int h,
    float z, rgb_f_t c);
void fp_add_quad(fp_render_t *fp, const GdkPoint pts[4],
    float z, rgb_f_t c);
void fp_add_arc(fp_render_t *fp, double cx, double cy, double r,
    double a0, double a1, float z, rgb_f_t c);

/* Opaque filled point markers.  Each deposits one marker that paints on
 * top of segments sharing its depth layer.  size is the full square or
 * diamond extent; radius is the circle radius, in pixels. */
void fp_add_filled_square(fp_render_t *fp, int cx, int cy, int size,
    float z, rgb_f_t c);
void fp_add_filled_diamond(fp_render_t *fp, int cx, int cy, int size,
    float z, rgb_f_t c);
void fp_add_filled_circle(fp_render_t *fp, int cx, int cy, int radius,
    float z, rgb_f_t c);

/* Defer one text run; painted during fp_render_flush after all segments. */
void fp_defer_text(fp_render_t *fp, int x, int y, const char *text,
    int justify, rgb_f_t c);

/* Pixel size of a string laid out in the freqplots drawing area font. */
void pango_text_size(GtkWidget *widget, int *width, int *height, char *s);

#endif /* FREQPLOTS_RENDER_H */
