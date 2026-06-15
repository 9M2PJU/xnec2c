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
 * cairo_scenebuffer: Depth-sorted segment accumulator for Cairo rendering.
 *
 * Each frame: reset -> leaf renderers deposit segments -> flush draws all
 * segments depth-sorted via painter's algorithm with minimal cairo_stroke calls.
 */

/* Default capacity for lazy-init in scenebuffer_reset */
#define SCENEBUFFER_INITIAL_CAP 4096

/* Filled marker shapes painted opaque on top of same-depth segments. */
typedef enum
{
  SCENEBUFFER_MARK_SQUARE  = 0, /* axis-aligned filled square */
  SCENEBUFFER_MARK_CIRCLE  = 1, /* filled disc */
  SCENEBUFFER_MARK_DIAMOND = 2  /* filled diamond (45-degree square) */
} scenebuffer_mark_shape_t;

/* One opaque filled marker.  Painted during scenebuffer_flush after every
 * segment sharing its quantized depth so it overlays equal-depth lines. */
typedef struct
{
  gint  x, y;    /* marker centre, screen space */
  float z_mid;   /* painter's-algorithm depth, same scale as Segment_t */
  float r, g, b; /* RGB color [0.0, 1.0] */
  int   size;    /* square/diamond: full extent; circle: radius, pixels */
  scenebuffer_mark_shape_t shape;
  int   seq;     /* deposit order; set by scenebuffer_add_marker for stable
                  * equal-depth paint order */
} Marker_t;

/* Dynamic array accumulating all segments for the current frame */
typedef struct cairo_scenebuffer
{
  Segment_t *segs; /* heap-allocated segment array */
  int        count;
  int        capacity;
  Marker_t  *marks; /* heap-allocated filled-marker array */
  int        mark_count;
  int        mark_capacity;
} cairo_scenebuffer_t;

/*-----------------------------------------------------------------------
 * Lifecycle
 *----------------------------------------------------------------------*/

void scenebuffer_destroy(cairo_scenebuffer_t *sb);

/*-----------------------------------------------------------------------
 * Per-frame operations
 *----------------------------------------------------------------------*/

/* Reset count to 0; retains allocation for reuse */
void scenebuffer_reset(cairo_scenebuffer_t *sb);

/* Append one segment; grows allocation as needed */
void scenebuffer_add(cairo_scenebuffer_t *sb, const Segment_t *seg);

/* Decompose a 4-point polygon into 4 outline edge segments.
 * template_seg supplies z_mid, r, g, b, width; xs/ys supply per-edge coords. */
void scenebuffer_add_polygon_outline(cairo_scenebuffer_t *sb,
    const Segment_t *template_seg,
    int xs[4], int ys[4]);

/* Append one opaque filled marker; grows allocation as needed.  The seq
 * field is assigned here so equal-depth markers paint in deposit order. */
void scenebuffer_add_marker(cairo_scenebuffer_t *sb, const Marker_t *m);

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

/* Sort by z_mid ascending (back-to-front), batch by (color, width),
 * flush with minimal cairo_stroke calls.
 * Reads rc_config.cairo_depth_bins and rc_config.cairo_color_quant directly.
 * When stats is non-NULL, the populated metrics describe this flush. */
void scenebuffer_flush(cairo_scenebuffer_t *sb, cairo_t *cr,
    cairo_flush_stats_t *stats);

#endif /* CAIRO_SCENEBUFFER_H */
