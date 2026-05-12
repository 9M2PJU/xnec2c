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
 * cairo_structure: Cairo renderer for wire segments and surface patches.
 *
 * draw_wire_segments and draw_surface_patches receive pre-selected colors
 * via struct_draw_params_t.  Zero flag evaluation for content selection;
 * dispatch decides currents vs charges vs geometry mode.
 */
#include "cairo_draw.h"
#include "../shared.h"
#include "../rdpattern_ui.h"

/*-----------------------------------------------------------------------*/

/**
 * draw_wire_segments() - Draw all wire segments with dispatch-selected colors
 * @cr:     Cairo context
 * @v:      view for segment coordinates
 * @segm:   projected segment array [nseg]
 * @nseg:   number of segments
 * @params: dispatch-resolved draw parameters (colors, cmax, show_flow)
 *
 * Single pass with precomputed per-segment colors from dispatch.
 * seg_rgb encodes type color (blue/yellow/red) in geometry mode.
 */
  static void
draw_wire_segments(cairo_t *cr, view_t *v, Segment_t *segm, gint nseg,
    const struct_draw_params_t *params)
{
  if( !nseg || isFlagSet(INPUT_PENDING) )
    return;

  if( params->wire_widths == NULL )
    BUG("draw_wire_segments: wire_widths is NULL\n");

  int idx;

  cairo_set_line_width(cr, 2.0);

  /* Draw networks */
  for( idx = 0; idx < netcx.nonet; idx++ )
  {
    int i1, i2;

    i1 = netcx.iseg1[idx] - 1;
    i2 = netcx.iseg2[idx] - 1;

    switch( netcx.ntyp[idx] )
    {
      case 1: /* Two-port network */
        {
          GdkPoint points[4];

          points[0].x = segm[i1].x1 + (segm[i2].x1 - segm[i1].x1) / 3;
          points[0].y = segm[i1].y1 + (segm[i2].y1 - segm[i1].y1) / 3;
          points[1].x = segm[i2].x1 + (segm[i1].x1 - segm[i2].x1) / 3;
          points[1].y = segm[i2].y1 + (segm[i1].y1 - segm[i2].y1) / 3;
          points[2].x = segm[i2].x2 + (segm[i1].x2 - segm[i2].x2) / 3;
          points[2].y = segm[i2].y2 + (segm[i1].y2 - segm[i2].y2) / 3;
          points[3].x = segm[i1].x2 + (segm[i2].x2 - segm[i1].x2) / 3;
          points[3].y = segm[i1].y2 + (segm[i2].y2 - segm[i1].y2) / 3;

          cairo_set_source_rgb(cr, MAGENTA);
          Cairo_Draw_Polygon(cr, points, 4);
          cairo_fill(cr);

          Cairo_Draw_Line(cr,
              segm[i1].x1, segm[i1].y1,
              segm[i2].x1, segm[i2].y1);
          Cairo_Draw_Line(cr,
              segm[i1].x2, segm[i1].y2,
              segm[i2].x2, segm[i2].y2);
        }
        break;

      case 2: /* Straight transmission line */
        cairo_set_source_rgb(cr, CYAN);
        Cairo_Draw_Line(cr,
            segm[i1].x1, segm[i1].y1,
            segm[i2].x1, segm[i2].y1);
        Cairo_Draw_Line(cr,
            segm[i1].x2, segm[i1].y2,
            segm[i2].x2, segm[i2].y2);
        break;

      case 3: /* Crossed transmission line */
        cairo_set_source_rgb(cr, CYAN);
        Cairo_Draw_Line(cr,
            segm[i1].x1, segm[i1].y1,
            segm[i2].x2, segm[i2].y2);
        Cairo_Draw_Line(cr,
            segm[i1].x2, segm[i1].y2,
            segm[i2].x1, segm[i2].y1);

    } /* switch( netcx.ntyp ) */

  } /* for( idx = 0; idx < netcx.nonet ) */

  /* Per-segment precomputed colors and widths from dispatch. */
  for( idx = 0; idx < nseg; idx++ )
  {
    cairo_set_line_width(cr, (double)params->wire_widths[idx]);
    cairo_set_source_rgb(cr,
        (double)params->wire_colors[idx].r,
        (double)params->wire_colors[idx].g,
        (double)params->wire_colors[idx].b);
    Cairo_Draw_Line(cr,
        segm[idx].x1, segm[idx].y1,
        segm[idx].x2, segm[idx].y2);
  }

} /* draw_wire_segments() */

/*-----------------------------------------------------------------------*/

/**
 * draw_surface_patches() - Draw patch segments with dispatch-selected colors
 * @cr:     Cairo context
 * @v:      view for segment coordinates
 * @segm:   projected patch segment array [npatch*2]
 * @npatch: number of patches (four rectangle edges each)
 * @params: dispatch-resolved draw parameters (colors)
 *
 * When params->cmax > 0 (current mode): draws each patch rectangle
 * in precomputed per-patch colors from struct_colors.
 * When params->cmax == 0 (geometry mode): draws all patches in the
 * base color from params->patch_colors[0].
 */
  static void
draw_surface_patches(cairo_t *cr, view_t *v, Segment_t *segm, gint npatch,
    const struct_draw_params_t *params)
{
  if( !npatch )
    return;

  int idx;

  cairo_set_line_width(cr, 1.0);

  /* Current mode: per-patch precomputed colors */
  if( params->cmax > 0.0 )
  {
    for( idx = 0; idx < npatch; idx++ )
    {
      int j = 4 * idx;
      int k;

      cairo_set_source_rgb(cr,
          (double)params->patch_colors[idx].r,
          (double)params->patch_colors[idx].g,
          (double)params->patch_colors[idx].b);

      /* 4 rectangle edges per patch */
      for( k = 0; k < 4; k++ )
        Cairo_Draw_Line(cr,
            segm[j + k].x1, segm[j + k].y1,
            segm[j + k].x2, segm[j + k].y2);
    }
    return;
  }

  /* Geometry mode: uniform base color */
  cairo_set_source_rgb(cr,
      (double)params->patch_colors[0].r,
      (double)params->patch_colors[0].g,
      (double)params->patch_colors[0].b);
  int nsg = 4 * npatch;
  for( idx = 0; idx < nsg; idx++ )
  {
    Cairo_Draw_Line(cr,
        segm[idx].x1, segm[idx].y1,
        segm[idx].x2, segm[idx].y2);
  }

} /* draw_surface_patches() */

/*-----------------------------------------------------------------------*/

/**
 * cairo_draw_structure() - Cairo renderer entry for structure window
 * @ctx:    cairo_render_ctx_t* with cr and structure_view
 * @params: dispatch-resolved draw parameters
 *
 * Projects geometry, draws axes, segments, and patches.
 * Post-render UI updates are handled by render().
 * Returns TRUE.
 */
  gboolean
cairo_draw_structure(void *ctx, float extent,
    const struct_draw_params_t *params)
{
  cairo_render_ctx_t *cc = (cairo_render_ctx_t *)ctx;
  cairo_t *cr = cc->cr;
  view_t *v = cc->view;
  double scale = view_projection_scale(v, extent, v->zoom);

  /* Project geometry to screen coordinates */
  Process_Wire_Segments(v, scale);
  Process_Surface_Patches(v, scale);

  /* Draw patches below wires */
  draw_surface_patches(cr, v, structure_segs + data.n, data.m, params);
  draw_wire_segments(cr, v, structure_segs, data.n, params);

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/**
 * cairo_init_empty() - Initialize an empty Cairo scene
 * @ctx: cairo_render_ctx_t*
 */
  void
cairo_init_empty(void *ctx)
{
  cairo_render_ctx_t *cc = (cairo_render_ctx_t *)ctx;

  cairo_set_source_rgb(cc->cr, BLACK);
  cairo_rectangle(cc->cr, 0.0, 0.0,
      (double)cc->view->width, (double)cc->view->height);
  cairo_fill(cc->cr);
}

/*-----------------------------------------------------------------------*/

/**
 * cairo_set_status() - Display a centered status message
 * @ctx: cairo_render_ctx_t*
 * @msg: UTF-8 status message
 */
  void
cairo_set_status(void *ctx, const char *msg)
{
  cairo_render_ctx_t *cc = (cairo_render_ctx_t *)ctx;
  Draw_Centered_Message(cc->cr, cc->view->width, cc->view->height, msg);
}

/*-----------------------------------------------------------------------*/

/**
 * cairo_set_gradient() - Enable or disable gradient key display
 * @ctx:  cairo_render_ctx_t*
 * @show: TRUE to draw the gradient color legend
 */
  void
cairo_set_gradient(void *ctx, gboolean show)
{
  cairo_render_ctx_t *cc = (cairo_render_ctx_t *)ctx;
  if( show )
    Draw_Color_Legend_Overlay(cc->cr);
}

/*-----------------------------------------------------------------------*/
