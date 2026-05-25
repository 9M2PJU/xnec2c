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
 * cairo_scenebuffer: Depth-sorted segment accumulator for Cairo rendering.
 *
 * Leaf renderers deposit segments each frame via scenebuffer_add().
 * scenebuffer_flush() sorts by z_mid (painter's algorithm, back-to-front),
 * groups consecutive segments sharing identical (r,g,b,width) into batched
 * cairo_move_to/cairo_line_to paths, and strokes each group once.
 */
#include "cairo_scenebuffer.h"
#include "../common.h"
#include "../shared.h"

#include <string.h>


/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_init() - Allocate segment array for a scenebuffer instance
 * @sb:               scenebuffer to initialize
 * @initial_capacity: number of segments to pre-allocate
 */
  void
scenebuffer_init(cairo_scenebuffer_t *sb, size_t initial_capacity)
{
  mem_alloc((void **)&sb->segs, initial_capacity * sizeof(Segment_t));
  sb->count    = 0;
  sb->capacity = (int)initial_capacity;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_destroy() - Release the segment array
 * @sb: scenebuffer to destroy
 */
  void
scenebuffer_destroy(cairo_scenebuffer_t *sb)
{
  mem_free((void **)&sb->segs);
  sb->count    = 0;
  sb->capacity = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_reset() - Clear segment count without releasing memory
 * @sb: scenebuffer to reset
 *
 * Lazy-initializes with a default capacity on the first call.
 * Called at the start of each Cairo frame before leaf renderers deposit segments.
 */
  void
scenebuffer_reset(cairo_scenebuffer_t *sb)
{
  if( sb->capacity == 0 )
    scenebuffer_init(sb, SCENEBUFFER_INITIAL_CAP);

  sb->count = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_add() - Append one segment to the scenebuffer
 * @sb:  target scenebuffer
 * @seg: segment to copy into the buffer
 *
 * Doubles capacity via mem_alloc when the array is full.
 */
  void
scenebuffer_add(cairo_scenebuffer_t *sb, const Segment_t *seg)
{
  if( sb->count >= sb->capacity )
  {
    int new_cap = sb->capacity * 2;
    Segment_t *grown = NULL;
    mem_alloc((void **)&grown, (size_t)new_cap * sizeof(Segment_t));
    memcpy(grown, sb->segs, (size_t)sb->count * sizeof(Segment_t));
    mem_free((void **)&sb->segs);
    sb->segs     = grown;
    sb->capacity = new_cap;
  }

  sb->segs[sb->count++] = *seg;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_add_polygon_outline() - Decompose a 4-point polygon into 4 edge segments
 * @sb:           target scenebuffer
 * @template_seg: supplies z_mid, r, g, b, width for all 4 edges
 * @xs, @ys:      screen-space x and y coordinates of the 4 vertices
 */
  void
scenebuffer_add_polygon_outline(cairo_scenebuffer_t *sb,
    const Segment_t *template_seg,
    int xs[4], int ys[4])
{
  int k;
  for( k = 0; k < 4; k++ )
  {
    int next = (k + 1) % 4;
    Segment_t edge = *template_seg;
    edge.x1 = xs[k];
    edge.y1 = ys[k];
    edge.x2 = xs[next];
    edge.y2 = ys[next];
    scenebuffer_add(sb, &edge);
  }
}

/*-----------------------------------------------------------------------*/
