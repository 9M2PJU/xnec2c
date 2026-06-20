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

#ifndef CAIRO_SCENEBUFFER_H
#define CAIRO_SCENEBUFFER_H 1

#include <cairo.h>
#include <glib.h>
#include "../common.h"

/*
 * cairo_scenebuffer: Depth-sorted accumulator for every Cairo drawing
 * primitive a view emits.
 *
 * Each frame: reset -> leaf renderers deposit primitives -> flush draws
 * them depth-sorted via painter's algorithm.  Four primitive lanes share
 * one depth scale: straight lines and stroked outlines as Segment_t, arcs
 * (stroked or filled discs) as Arc_t, filled convex polygons as Polygon_t,
 * and text runs as Text_t.  Depth dominates overlay; kind breaks equal-depth
 * ties in the order segment, arc, polygon, text.
 */

/* Default capacity for lazy-init in scenebuffer_reset */
#define SCENEBUFFER_INITIAL_CAP 4096

/* Vertex headroom for any filled polygon: square, diamond, quad, rect (4). */
#define SCENE_POLY_MAX 8

/* Maximum stored length of one text run (titles, scale values, labels). */
#define SCENE_TEXT_LEN 64

/* Text justification, calculated against the supplied x (horizontal) and
 * y (vertical) anchor coordinates.  JUSTIFY_BOLD selects the bold weight. */
enum {
  JUSTIFY_LEFT   = 0,
  JUSTIFY_CENTER = 1,
  JUSTIFY_RIGHT  = 2,
  JUSTIFY_HMASK  = 0x03,

  JUSTIFY_BELOW  = 0,
  JUSTIFY_MIDDLE = 1 << 2,
  JUSTIFY_ABOVE  = 2 << 2,
  JUSTIFY_VMASK  = 0x0c,

  JUSTIFY_BOLD   = 1 << 4
};

/* Paint operation for an arc: stroked outline or filled disc. */
typedef enum { SCENE_STROKE, SCENE_FILL } scene_paint_t;

/* Circular arc: stroked outline, or filled disc when mode is SCENE_FILL.
 * Common fields follow the Segment_t order (geometry, z_mid, r,g,b, width)
 * with mode trailing. */
typedef struct
{
  double        cx, cy, radius, a0, a1; /* centre, radius, start/end angle */
  float         z_mid;                  /* painter's-algorithm depth */
  float         r, g, b;                /* RGB color [0.0, 1.0] */
  float         width;                  /* stroke width; unused when filled */
  scene_paint_t mode;
} Arc_t;

/* Filled convex polygon: square, diamond, and any future fill.  Vertices are
 * stored AoS in screen space; common fields follow the Segment_t order. */
typedef struct
{
  GdkPoint pts[SCENE_POLY_MAX]; /* vertices, screen space */
  int      n;                   /* vertex count, <= SCENE_POLY_MAX */
  float    z_mid;               /* painter's-algorithm depth */
  float    r, g, b;             /* RGB color [0.0, 1.0] */
} Polygon_t;

/* One deferred text run.  scale multiplies the base layout glyph size; the
 * justify bits select anchor alignment and bold weight. */
typedef struct
{
  int   x, y;                 /* anchor, screen space */
  float z_mid;                /* painter's-algorithm depth */
  float r, g, b;              /* RGB color [0.0, 1.0] */
  float scale;                /* glyph-size multiplier, per run */
  int   justify;              /* JUSTIFY_* bits incl. JUSTIFY_BOLD */
  char  text[SCENE_TEXT_LEN];
} Text_t;

/* Per-frame accumulator.  Each kind owns a packed lazily-grown array sized to
 * its own element so the segment hot path keeps optimal cache density.  The
 * text layout is a non-owning base font set by the text-using consumer. */
typedef struct cairo_scenebuffer
{
  Segment_t   *segs; /* straight lines and stroked outlines */
  int          count;
  Arc_t       *arcs;
  int          arc_count;
  Polygon_t   *polys;
  int          poly_count;
  Text_t      *texts;
  int          text_count;
  PangoLayout *text_layout; /* non-owning base font for every text run */
} cairo_scenebuffer_t;

/*-----------------------------------------------------------------------
 * Lifecycle
 *----------------------------------------------------------------------*/

void scenebuffer_destroy(cairo_scenebuffer_t *sb);

/*-----------------------------------------------------------------------
 * Per-frame operations
 *----------------------------------------------------------------------*/

/* Reset all lane counts to 0; retains allocations for reuse */
void scenebuffer_reset(cairo_scenebuffer_t *sb);

/* Append one segment; grows allocation as needed */
void scenebuffer_add(cairo_scenebuffer_t *sb, const Segment_t *seg);

/* Decompose a 4-point polygon into 4 outline edge segments.
 * template_seg supplies z_mid, r, g, b, width; xs/ys supply per-edge coords. */
void scenebuffer_add_polygon_outline(cairo_scenebuffer_t *sb,
    const Segment_t *template_seg,
    int xs[4], int ys[4]);

/* Append one arc (stroked or filled disc); grows allocation as needed */
void scenebuffer_add_arc(cairo_scenebuffer_t *sb, const Arc_t *a);

/* Append one filled convex polygon; grows allocation as needed */
void scenebuffer_add_polygon(cairo_scenebuffer_t *sb, const Polygon_t *p);

/* Append one text run; grows allocation as needed */
void scenebuffer_add_text(cairo_scenebuffer_t *sb, const Text_t *t);

/* Set the base layout used to render every text primitive in this buffer.
 * Caller retains ownership and lifetime; NULL means this buffer emits no
 * text and any text deposit is a BUG. */
void scenebuffer_set_text_layout(cairo_scenebuffer_t *sb, PangoLayout *layout);

/* Set to 1 to collect per-frame flush metrics; 0 passes NULL to
 * scenebuffer_flush, skipping all timing and metric overhead. */
#define CAIRO_FLUSH_STATS 0

/* Per-frame flush metrics produced by scenebuffer_flush.  Consumers
 * use these for performance reporting (segments rendered, batch
 * compression ratio, sort/stroke wall-times). */
typedef struct
{
  int    segments;     /* total segments emitted */
  int    batch_groups; /* cairo_stroke invocations */
  gint64 sort_us;      /* depth-sort wall-time, microseconds */
  gint64 stroke_us;    /* path build + stroke wall-time, microseconds */
  int    capacity;     /* allocator capacity at flush time */
} cairo_flush_stats_t;

/* Sort each lane by quantized z_mid ascending (back-to-front), then sweep a
 * z-frontier emitting at each depth in kind order (segment, arc, polygon,
 * text) with minimal cairo_stroke/cairo_fill calls.
 * Reads rc_config.cairo_depth_bins and rc_config.cairo_color_quant directly.
 * When stats is non-NULL, the populated metrics describe this flush. */
void scenebuffer_flush(cairo_scenebuffer_t *sb, cairo_t *cr,
    cairo_flush_stats_t *stats);

#endif /* CAIRO_SCENEBUFFER_H */
