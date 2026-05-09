/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  The official website and doumentation for xnec2c is available here:
 *    https://www.xnec2c.org/
 */

/*
 * cairo_project: Cairo drawing primitives.
 */
#include "cairo_draw.h"
#include "../shared.h"
#include "../prerender/prerender_aggregate.h"

/*-----------------------------------------------------------------------
 * Drawing primitives
 *----------------------------------------------------------------------*/

  void
Cairo_Draw_Polygon(cairo_t *cr, GdkPoint *points, int npoints)
{
  int idx;

  cairo_move_to(cr, (double)points[0].x, (double)points[0].y);
  for( idx = 1; idx < npoints; idx++ )
    cairo_line_to(cr, (double)points[idx].x, (double)points[idx].y);
  cairo_close_path(cr);
}

/*-----------------------------------------------------------------------*/

  void
Cairo_Draw_Segments(cairo_t *cr, Segment_t *segm, int nseg)
{
  int idx;

  for( idx = 0; idx < nseg; idx++ )
  {
    cairo_move_to(cr, (double)segm[idx].x1, (double)segm[idx].y1);
    cairo_line_to(cr, (double)segm[idx].x2, (double)segm[idx].y2);
  }
  cairo_stroke(cr);
}

/*-----------------------------------------------------------------------*/

  void
Cairo_Draw_Line(cairo_t *cr, int x1, int y1, int x2, int y2)
{
  cairo_move_to(cr, (double)x1, (double)y1);
  cairo_line_to(cr, (double)x2, (double)y2);
  cairo_stroke(cr);
}

/*-----------------------------------------------------------------------*/

  void
Cairo_Draw_Lines(cairo_t *cr, GdkPoint *points, int npoints)
{
  int idx;

  cairo_move_to(cr, (double)points[0].x, (double)points[0].y);
  for( idx = 1; idx < npoints; idx++ )
    cairo_line_to(cr, (double)points[idx].x, (double)points[idx].y);
  cairo_stroke(cr);
}

/*-----------------------------------------------------------------------*/
