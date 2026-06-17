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
 * cairo_scenebuffer: Depth-sorted primitive accumulator for Cairo rendering.
 *
 * Leaf renderers deposit primitives each frame into four packed lanes
 * (segments, arcs, polygons, text).  scenebuffer_flush() sorts each lane by
 * z_mid (painter's algorithm, back-to-front) and emits them depth-first with
 * batched cairo_stroke/cairo_fill calls.
 */
#include "cairo_scenebuffer.h"
#include "../common.h"
#include "../shared.h"

#include <string.h>


/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_init() - Allocate the segment lane for a scenebuffer instance
 * @sb:               scenebuffer to initialize
 * @initial_capacity: number of segments to pre-allocate
 *
 * The arc, polygon, and text lanes are left empty and grown lazily on first
 * deposit so a segment-only frame never allocates them.
 */
  static void
scenebuffer_init(cairo_scenebuffer_t *sb, size_t initial_capacity)
{
  mem_alloc((void **)&sb->segs, initial_capacity * sizeof(Segment_t));
  sb->count    = 0;
  sb->capacity = (int)initial_capacity;

  sb->arcs         = NULL;
  sb->arc_count    = 0;
  sb->arc_capacity = 0;

  sb->polys         = NULL;
  sb->poly_count    = 0;
  sb->poly_capacity = 0;

  sb->texts         = NULL;
  sb->text_count    = 0;
  sb->text_capacity = 0;

  sb->text_layout = NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_destroy() - Release every lane allocation
 * @sb: scenebuffer to destroy
 *
 * The text layout is non-owning and is not released here.
 */
  void
scenebuffer_destroy(cairo_scenebuffer_t *sb)
{
  mem_free((void **)&sb->segs);
  sb->count    = 0;
  sb->capacity = 0;

  mem_free((void **)&sb->arcs);
  sb->arc_count    = 0;
  sb->arc_capacity = 0;

  mem_free((void **)&sb->polys);
  sb->poly_count    = 0;
  sb->poly_capacity = 0;

  mem_free((void **)&sb->texts);
  sb->text_count    = 0;
  sb->text_capacity = 0;

  sb->text_layout = NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_reset() - Clear all lane counts without releasing memory
 * @sb: scenebuffer to reset
 *
 * Lazy-initializes the segment lane with a default capacity on the first
 * call.  Called at the start of each Cairo frame before leaf renderers
 * deposit primitives.  The text layout persists across resets.
 */
  void
scenebuffer_reset(cairo_scenebuffer_t *sb)
{
  if( sb->capacity == 0 )
    scenebuffer_init(sb, SCENEBUFFER_INITIAL_CAP);

  sb->count      = 0;
  sb->arc_count  = 0;
  sb->poly_count = 0;
  sb->text_count = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_set_text_layout() - Bind the base font for text primitives
 * @sb:     target scenebuffer
 * @layout: caller-owned Pango layout, or NULL to emit no text
 */
  void
scenebuffer_set_text_layout(cairo_scenebuffer_t *sb, PangoLayout *layout)
{
  sb->text_layout = layout;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_add() - Append one segment to the scenebuffer
 * @sb:  target scenebuffer
 * @seg: segment to copy into the buffer
 *
 * Doubles capacity via mem_alloc when the lane is full.
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

/**
 * scenebuffer_add_arc() - Append one arc (stroked outline or filled disc)
 * @sb: target scenebuffer
 * @a:  arc to copy into the buffer
 *
 * Doubles capacity via mem_alloc when the lane is full; the lane is grown
 * from empty on first deposit.
 */
  void
scenebuffer_add_arc(cairo_scenebuffer_t *sb, const Arc_t *a)
{
  if( sb->arc_count >= sb->arc_capacity )
  {
    int new_cap = sb->arc_capacity ? sb->arc_capacity * 2 : 256;
    Arc_t *grown = NULL;
    mem_alloc((void **)&grown, (size_t)new_cap * sizeof(Arc_t));
    if( sb->arc_count )
      memcpy(grown, sb->arcs, (size_t)sb->arc_count * sizeof(Arc_t));
    mem_free((void **)&sb->arcs);
    sb->arcs         = grown;
    sb->arc_capacity = new_cap;
  }

  sb->arcs[sb->arc_count++] = *a;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_add_polygon() - Append one filled convex polygon
 * @sb: target scenebuffer
 * @p:  polygon to copy into the buffer
 *
 * Doubles capacity via mem_alloc when the lane is full; the lane is grown
 * from empty on first deposit.
 */
  void
scenebuffer_add_polygon(cairo_scenebuffer_t *sb, const Polygon_t *p)
{
  if( p->n > SCENE_POLY_MAX )
  {
    BUG("scenebuffer_add_polygon: n=%d > SCENE_POLY_MAX\n", p->n);
    return;
  }

  if( sb->poly_count >= sb->poly_capacity )
  {
    int new_cap = sb->poly_capacity ? sb->poly_capacity * 2 : 256;
    Polygon_t *grown = NULL;
    mem_alloc((void **)&grown, (size_t)new_cap * sizeof(Polygon_t));
    if( sb->poly_count )
      memcpy(grown, sb->polys, (size_t)sb->poly_count * sizeof(Polygon_t));
    mem_free((void **)&sb->polys);
    sb->polys         = grown;
    sb->poly_capacity = new_cap;
  }

  sb->polys[sb->poly_count++] = *p;
}

/*-----------------------------------------------------------------------*/

/**
 * scenebuffer_add_text() - Append one text run
 * @sb: target scenebuffer
 * @t:  text run to copy into the buffer
 *
 * Doubles capacity via mem_alloc when the lane is full; the lane is grown
 * from empty on first deposit.  Depositing text without a base layout is a
 * BUG since flush has no font to render it with.
 */
  void
scenebuffer_add_text(cairo_scenebuffer_t *sb, const Text_t *t)
{
  if( sb->text_layout == NULL )
  {
    BUG("scenebuffer_add_text: no base layout set\n");
    return;
  }

  if( sb->text_count >= sb->text_capacity )
  {
    int new_cap = sb->text_capacity ? sb->text_capacity * 2 : 64;
    Text_t *grown = NULL;
    mem_alloc((void **)&grown, (size_t)new_cap * sizeof(Text_t));
    if( sb->text_count )
      memcpy(grown, sb->texts, (size_t)sb->text_count * sizeof(Text_t));
    mem_free((void **)&sb->texts);
    sb->texts         = grown;
    sb->text_capacity = new_cap;
  }

  sb->texts[sb->text_count++] = *t;
}

/*-----------------------------------------------------------------------*/
