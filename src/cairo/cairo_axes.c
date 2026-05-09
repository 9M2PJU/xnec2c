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
 * cairo_axes: XYZ axis drawing for Cairo rendering.
 */
#include "cairo_draw.h"
#include "../shared.h"

/*-----------------------------------------------------------------------*/

/**
 * Project_XYZ_Axes() - Project xyz axes to screen coordinates
 * @cr:      Cairo context (for Pango layout)
 * @v:       view for projection
 * @segment: output array of 3 Segment_t (x, y, z axes)
 */
  static void
Project_XYZ_Axes(cairo_t *cr, view_t *v, float extent, Segment_t *segment)
{
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

  double scale = view_projection_scale(v, extent, v->zoom);
  double xc    = view_x_center(v);
  double yc    = view_y_center(v);
  double px    = (double)v->pan_offset[0];
  double py    = (double)v->pan_offset[1];
  PangoLayout *layout;
  int idx;

  cairo_set_source_rgb(cr, WHITE);

  /* Use the drawingarea matching the view type for font metrics */
  GtkWidget *da = (v->type == VIEW_RDPATTERN)
    ? rdpattern_drawingarea : structure_drawingarea;
  layout = gtk_widget_create_pango_layout(da, NULL);

  for( idx = 0; idx < 3; idx++ )
  {
    Segment_t *segm = &segment[idx];
    double x, y;

    /* Axis endpoints at content extent distance along each direction */
    Project_on_Screen(v,
        axis_table[idx].ox * (double)extent,
        axis_table[idx].oy * (double)extent,
        axis_table[idx].oz * (double)extent,
        &x, &y);

    segm->x1 = (gint)(xc + px);
    segm->y1 = v->height - (gint)(yc + py);
    segm->x2 = (gint)(xc + px + x * scale);
    segm->y2 = v->height - (gint)(yc + py + y * scale);

    pango_layout_set_text(layout, axis_table[idx].label, -1);
    cairo_move_to(cr, (double)segm->x2, (double)segm->y2);
    pango_cairo_show_layout(cr, layout);
  }

  g_object_unref(layout);
}

/*-----------------------------------------------------------------------*/

/**
 * Draw_XYZ_Axes() - Draw xyz axes to screen
 * @cr: Cairo context
 * @v:  view for projection parameters
 */
  void
Draw_XYZ_Axes(cairo_t *cr, view_t *v, float extent)
{
  static Segment_t seg[3];

  Project_XYZ_Axes(cr, v, extent, seg);
  Cairo_Draw_Segments(cr, seg, 3);
}

/*-----------------------------------------------------------------------*/

/**
 * cairo_draw_axes() - render_ops_t draw_axes vtable implementation
 * @ctx:    cairo_render_ctx_t cast to void*
 * @extent: half-extent of primary content for axis length
 */
  void
cairo_draw_axes(void *ctx, float extent)
{
  cairo_render_ctx_t *cc = (cairo_render_ctx_t *)ctx;

  Draw_XYZ_Axes(cc->cr, cc->view, extent);
}

/*-----------------------------------------------------------------------*/
