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
 * cairo_scenebuffer_flush: Depth-sort and batched-stroke pipeline.
 *
 * Sorts accumulated segments by quantized z_mid (painter's algorithm),
 * clusters consecutive segments sharing identical (r,g,b,width) into
 * batched cairo_move_to/cairo_line_to paths, and strokes each cluster once.
 */
#include "cairo_scenebuffer.h"
#include "../common.h"
#include "../shared.h"

#include <stdlib.h>
#include <math.h>

/* Use shared float-comparison helpers from common.h (fl_feq, fl_flt, fl_fgt) */

/*-----------------------------------------------------------------------*/

/* qsort comparator: ascending quantized z_mid (painter's algorithm),
 * then by (r,g,b,width) to cluster identical-attribute segments within
 * each depth band for longer batch runs in scenebuffer_flush.
 *
 * Bitwise == instead of epsilon comparison: qsort requires a strict weak
 * ordering (transitivity).  Epsilon-equality is not transitive and causes
 * undefined behavior in qsort.  After scenebuffer_quantize_depth() and
 * scenebuffer_quantize_colors(), values land on a discrete lattice
 * produced by floorf(); identical quantization inputs yield identical
 * bit patterns.  When color quantization is disabled (color_quant=0),
 * raw floats from Value_to_Color() compare exactly as expected. */
static int
cmp_z_color(const void *a, const void *b)
{
  const Segment_t *sa = (const Segment_t *)a;
  const Segment_t *sb_seg = (const Segment_t *)b;

  if( sa->z_mid != sb_seg->z_mid )
    return (sa->z_mid < sb_seg->z_mid) ? -1 : 1;

  if( sa->r != sb_seg->r )
    return (sa->r < sb_seg->r) ? -1 : 1;
  if( sa->g != sb_seg->g )
    return (sa->g < sb_seg->g) ? -1 : 1;
  if( sa->b != sb_seg->b )
    return (sa->b < sb_seg->b) ? -1 : 1;
  if( sa->width != sb_seg->width )
    return (sa->width < sb_seg->width) ? -1 : 1;

  return 0;
}

/*-----------------------------------------------------------------------*/

/* qsort comparator: ascending quantized z_mid, then deposit order so
 * overlapping equal-depth markers paint in the sequence they were added.
 * Bitwise == on quantized depth: same rationale as cmp_z_color. */
static int
cmp_marker_z(const void *a, const void *b)
{
  const Marker_t *ma = (const Marker_t *)a;
  const Marker_t *mb = (const Marker_t *)b;

  if( ma->z_mid != mb->z_mid )
    return (ma->z_mid < mb->z_mid) ? -1 : 1;

  if( ma->seq != mb->seq )
    return (ma->seq < mb->seq) ? -1 : 1;

  return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_quantize_colors() - Round RGB values to discrete levels
 * @sb:          scenebuffer with deposited segments
 * @color_quant: quantization levels (0 = disabled)
 */
static void
scenebuffer_quantize_colors(cairo_scenebuffer_t *sb, int color_quant)
{
  if( color_quant <= 0 )
    return;

  float levels = (float)color_quant;
  int qi;
  for( qi = 0; qi < sb->count; qi++ )
  {
    Segment_t *s = &sb->segs[qi];
    s->r = floorf(s->r * levels + 0.5f) / levels;
    s->g = floorf(s->g * levels + 0.5f) / levels;
    s->b = floorf(s->b * levels + 0.5f) / levels;
  }
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_quantize_depth() - Bin z_mid values for painter's algorithm
 * @sb:         scenebuffer with deposited segments and markers
 * @depth_bins: number of z bins (>=1)
 *
 * Folds both the segment and marker depths into one extent so the two
 * arrays share a single quantization lattice and stay depth-comparable
 * during the flush merge.  When all depths are equal (zero range) the
 * early return preserves natural order.
 */
static void
scenebuffer_quantize_depth(cairo_scenebuffer_t *sb, int depth_bins)
{
  float z_min, z_max;
  int   n;

  /* Seed the extent from whichever array holds geometry; the flush guard
   * guarantees at least one segment or marker is present. */
  if( sb->count > 0 )
    z_min = z_max = sb->segs[0].z_mid;
  else
    z_min = z_max = sb->marks[0].z_mid;

  for( n = 0; n < sb->count; n++ )
  {
    if( fl_flt(sb->segs[n].z_mid, z_min) ) z_min = sb->segs[n].z_mid;
    if( fl_fgt(sb->segs[n].z_mid, z_max) ) z_max = sb->segs[n].z_mid;
  }
  for( n = 0; n < sb->mark_count; n++ )
  {
    if( fl_flt(sb->marks[n].z_mid, z_min) ) z_min = sb->marks[n].z_mid;
    if( fl_fgt(sb->marks[n].z_mid, z_max) ) z_max = sb->marks[n].z_mid;
  }

  float z_range = z_max - z_min;
  if( !fl_fgt(z_range, 0.0f) )
    return;

  float inv_range = 1.0f / z_range;
  float bins_f    = (float)depth_bins;

  for( n = 0; n < sb->count; n++ )
  {
    float norm = (sb->segs[n].z_mid - z_min) * inv_range;
    sb->segs[n].z_mid = z_min + floorf(norm * bins_f) / bins_f * z_range;
  }
  for( n = 0; n < sb->mark_count; n++ )
  {
    float norm = (sb->marks[n].z_mid - z_min) * inv_range;
    sb->marks[n].z_mid = z_min + floorf(norm * bins_f) / bins_f * z_range;
  }
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_emit_marker() - Fill one opaque marker into the context
 * @cr: Cairo context to draw into
 * @m:  marker describing centre, color, size, and shape
 */
static void
scenebuffer_emit_marker(cairo_t *cr, const Marker_t *m)
{
  cairo_set_source_rgb(cr, (double)m->r, (double)m->g, (double)m->b);
  cairo_new_path(cr);

  switch( m->shape )
  {
    case SCENEBUFFER_MARK_CIRCLE:
      cairo_arc(cr, (double)m->x, (double)m->y, (double)m->size, 0.0, M_2PI);
      break;

    case SCENEBUFFER_MARK_SQUARE:
      cairo_rectangle(cr,
          (double)(m->x - m->size / 2), (double)(m->y - m->size / 2),
          (double)m->size, (double)m->size);
      break;

    case SCENEBUFFER_MARK_DIAMOND:
    {
      int half = m->size / 2;
      cairo_move_to(cr, (double)(m->x - half), (double)m->y);
      cairo_line_to(cr, (double)m->x,          (double)(m->y + half + 1));
      cairo_line_to(cr, (double)(m->x + half), (double)m->y);
      cairo_line_to(cr, (double)m->x,          (double)(m->y - half - 1));
      cairo_close_path(cr);
      break;
    }

    default:
      BUG("scenebuffer_emit_marker: shape=%d\n", m->shape);
      return;
  }

  cairo_fill(cr);
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_flush() - Sort, group, and stroke all accumulated segments
 * @sb:          scenebuffer to flush
 * @cr:          Cairo context to draw into
 * @stats:       out-param populated with flush metrics; NULL to skip
 *
 * Reads rc_config.cairo_depth_bins and rc_config.cairo_color_quant directly.
 *
 * Groups consecutive segments sharing identical (r,g,b,width) into one
 * batched path stroked with a single cairo_stroke() call.
 */
  void
scenebuffer_flush(cairo_scenebuffer_t *sb, cairo_t *cr,
    cairo_flush_stats_t *stats)
{
  int depth_bins  = rc_config.cairo_depth_bins;
  int color_quant = rc_config.cairo_color_quant;

  if( depth_bins < 1 )
  {
    BUG("scenebuffer_flush: depth_bins=%d\n", depth_bins);
    return;
  }

  if( stats != NULL )
  {
    stats->segments     = sb->count;
    stats->batch_groups = 0;
    stats->sort_us      = 0;
    stats->stroke_us    = 0;
    stats->capacity     = sb->capacity;
  }

  if( sb->count == 0 && sb->mark_count == 0 )
    return;

  scenebuffer_quantize_colors(sb, color_quant);
  scenebuffer_quantize_depth(sb, depth_bins);

  gint64 t_sort_start = 0;
  if( stats != NULL )
    t_sort_start = g_get_monotonic_time();

  qsort(sb->segs, (size_t)sb->count, sizeof(Segment_t), cmp_z_color);
  if( sb->mark_count > 0 )
    qsort(sb->marks, (size_t)sb->mark_count, sizeof(Marker_t), cmp_marker_z);

  gint64 t_stroke_start = 0;
  if( stats != NULL )
  {
    stats->sort_us = g_get_monotonic_time() - t_sort_start;
    t_stroke_start = g_get_monotonic_time();
  }
  int    groups = 0;
  int i = 0;
  int mi = 0;
  while( i < sb->count )
  {
    Segment_t *head = &sb->segs[i];

    /* Paint markers strictly below this run's depth; equal-depth markers
     * defer so they overlay the same-depth segment runs. */
    while( mi < sb->mark_count && sb->marks[mi].z_mid < head->z_mid )
      scenebuffer_emit_marker(cr, &sb->marks[mi++]);

    /* Find the run end where (r,g,b,width) changes */
    int j = i + 1;
    /* Bitwise == for run detection: same rationale as cmp_z_color */
    while( j < sb->count &&
        sb->segs[j].r     == head->r &&
        sb->segs[j].g     == head->g &&
        sb->segs[j].b     == head->b &&
        sb->segs[j].width == head->width )
      j++;

    /* Emit one path for the run [i, j) */
    cairo_set_source_rgb(cr, (double)head->r, (double)head->g, (double)head->b);
    cairo_set_line_width(cr, (double)head->width);
    cairo_new_path(cr);

    int k;
    for( k = i; k < j; k++ )
    {
      Segment_t *s = &sb->segs[k];
      cairo_move_to(cr, (double)s->x1, (double)s->y1);
      cairo_line_to(cr, (double)s->x2, (double)s->y2);
    }

    cairo_stroke(cr);
    groups++;
    i = j;
  }

  /* Remaining markers sit at or above the deepest segment depth; paint
   * them last so they overlay all equal-or-shallower geometry. */
  while( mi < sb->mark_count )
    scenebuffer_emit_marker(cr, &sb->marks[mi++]);

  if( stats != NULL )
  {
    stats->batch_groups = groups;
    stats->stroke_us    = g_get_monotonic_time() - t_stroke_start;
  }
}
