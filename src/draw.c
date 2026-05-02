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

#include "draw.h"
#include "shared.h"

/*-----------------------------------------------------------------------*/

/*  Project_XYZ_Axes()
 *
 *  Sets Segment_t data to project xyz axes on Screen.
 *
 *  The three axes differ only in which component carries r_max and
 *  in the glyph drawn at the tip; drive both from a tiny table so the
 *  origin, projection, and label steps run through one loop.
 */
  static void
Project_XYZ_Axes(
    cairo_t *cr,
    view_t *v,
    Segment_t *segment )
{
  /* axis_table - {tip offset, label} per Cartesian axis */
  static const struct
  {
    double ox, oy, oz;
    const gchar *label;
  } axis_table[3] =
  {
    { 1.0, 0.0, 0.0, "x"  },
    { 0.0, 1.0, 0.0, "y"  },
    { 0.0, 0.0, 1.0, " z" }
  };

  double scale = view_xy_scale(v);
  double xc    = view_x_center(v);
  double yc    = view_y_center(v);
  double px    = (double)v->pan_offset[0];
  double py    = (double)v->pan_offset[1];
  PangoLayout *layout;
  int idx;

  cairo_set_source_rgb( cr, WHITE );
  layout = gtk_widget_create_pango_layout( structure_drawingarea, NULL );

  for( idx = 0; idx < 3; idx++ )
  {
    Segment_t *segm = &segment[idx];
    double x, y;

    Project_on_Screen( v,
        axis_table[idx].ox * v->r_max,
        axis_table[idx].oy * v->r_max,
        axis_table[idx].oz * v->r_max,
        &x, &y );

    segm->x1 = (gint)( xc + px );
    segm->y1 = v->height - (gint)( yc + py );
    segm->x2 = (gint)( xc + px + x * scale );
    segm->y2 = v->height - (gint)( yc + py + y * scale );

    pango_layout_set_text( layout, axis_table[idx].label, -1 );
    cairo_move_to( cr, (double)segm->x2, (double)segm->y2 );
    pango_cairo_show_layout( cr, layout );
  }

  g_object_unref( layout );

} /* Project_XYZ_Axes() */

/*-----------------------------------------------------------------------*/

/*  Draw_XYZ_Axes()
 *
 *  Draws the xyz axes to Screen
 */
  void
Draw_XYZ_Axes( cairo_t *cr, view_t *v )
{
  static Segment_t seg[3];

  /* Calculate Screen co-ordinates of xyz axes */
  Project_XYZ_Axes( cr, v, seg );

  /* Draw xyz axes */
  Cairo_Draw_Segments( cr, seg, 3 );

} /* Draw_XYZ_Axes() */

/*-----------------------------------------------------------------------*/

/* Cairo_Draw_Polygon()
 *
 * Draws a polygon, given a number of points
 */
  void
Cairo_Draw_Polygon( cairo_t* cr, GdkPoint *points, int npoints )
{
  int idx;

  cairo_move_to( cr, (double)points[0].x, (double)points[0].y );
  for( idx = 1; idx < npoints; idx++ )
    cairo_line_to( cr, (double)points[idx].x, (double)points[idx].y );
  cairo_close_path( cr );

} /* Cairo_Draw_Polygon() */

/*-----------------------------------------------------------------------*/

/* Cairo_Draw_Segments()
 *
 * Draws a number of line segments
 */
  void
Cairo_Draw_Segments( cairo_t *cr, Segment_t *segm, int nseg )
{
  int idx;

  for( idx = 0; idx < nseg; idx++ )
  {
    cairo_move_to( cr, (double)segm[idx].x1, (double)segm[idx].y1 );
    cairo_line_to( cr, (double)segm[idx].x2, (double)segm[idx].y2 );
  }
  cairo_stroke( cr );

} /* Cairo_Draw_Segments() */

/*-----------------------------------------------------------------------*/

/* Cairo_Draw_Line()
 *
 * Draws a line between to x,y co-ordinates
 */
  void
Cairo_Draw_Line( cairo_t *cr, int x1, int y1, int x2, int y2 )
{
  cairo_move_to( cr, (double)x1, (double)y1 );
  cairo_line_to( cr, (double)x2, (double)y2 );
  cairo_stroke( cr );

} /* Cairo_Draw_Line() */

/*-----------------------------------------------------------------------*/

/* Cairo_Draw_Lines()
 *
 * Draws lines between points
 */
  void
Cairo_Draw_Lines( cairo_t *cr, GdkPoint *points, int npoints )
{
  int idx;

  cairo_move_to( cr, (double)points[0].x, (double)points[0].y );
  for( idx = 1; idx < npoints; idx++ )
    cairo_line_to( cr, (double)points[idx].x, (double)points[idx].y );
  cairo_stroke( cr );

} /* Cairo_Draw_Line() */

/*-----------------------------------------------------------------------*/

