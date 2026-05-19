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
#include "../prerender/prerender_patch_arrow.h"
#include "../prerender/prerender_state.h"
#include "../prerender/prerender_color.h"
#include "../rdpattern_ui.h"

/* Cairo-local world-space arrow segment (3D endpoints, one edge per arrow line) */
typedef struct
{
  double x1, y1, z1;
  double x2, y2, z2;
} arrow_seg_3d_t;

/** patch_arrow_to_world() - Rotate arrow template and map to world coordinates
 * @idx:   patch index (0-based into data.m)
 * @fd:    precomputed flow data {Re(ct1),Im(ct1),Re(ct2),Im(ct2)} / cmax
 * @phase: current animation phase (radians)
 * @segs:  output array of ARROW_LINE_COUNT world-space segments
 *
 * Reads center and tangent axes from geom_pre.patch_tangent_frame[idx].
 * Computes instantaneous flow direction at phase, rotates arrow template,
 * maps each UV vertex to world via:
 *   world = center + 2*(rot_u * st1 + rot_v * st2)
 */
static void
patch_arrow_to_world(int idx, const float fd[4], float phase,
    arrow_seg_3d_t segs[ARROW_LINE_COUNT])
{
  const patch_tangent_frame_t *tf = &geom_pre.patch_tangent_frame[idx];
  double cp = cos((double)phase);
  double sp = sin((double)phase);
  double re1 = (double)fd[0] * cp - (double)fd[1] * sp;
  double re2 = (double)fd[2] * cp - (double)fd[3] * sp;
  double angle = atan2(re2, re1);
  double ca = cos(angle);
  double sa = sin(angle);

  int k;
  for( k = 0; k < ARROW_LINE_COUNT; k++ )
  {
    double u0 = (double)arrow_template[k].u1 - 0.5;
    double v0 = (double)arrow_template[k].v1 - 0.5;
    double ru0 = u0 * ca - v0 * sa;
    double rv0 = u0 * sa + v0 * ca;

    double u1 = (double)arrow_template[k].u2 - 0.5;
    double v1 = (double)arrow_template[k].v2 - 0.5;
    double ru1 = u1 * ca - v1 * sa;
    double rv1 = u1 * sa + v1 * ca;

    segs[k].x1 = tf->cx + 2.0 * (ru0 * tf->st1x + rv0 * tf->st2x);
    segs[k].y1 = tf->cy + 2.0 * (ru0 * tf->st1y + rv0 * tf->st2y);
    segs[k].z1 = tf->cz + 2.0 * (ru0 * tf->st1z + rv0 * tf->st2z);
    segs[k].x2 = tf->cx + 2.0 * (ru1 * tf->st1x + rv1 * tf->st2x);
    segs[k].y2 = tf->cy + 2.0 * (ru1 * tf->st1y + rv1 * tf->st2y);
    segs[k].z2 = tf->cz + 2.0 * (ru1 * tf->st1z + rv1 * tf->st2z);
  }
}

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
draw_surface_patches(cairo_t *cr, view_t *v, double scale, Segment_t *segm,
    gint npatch, const struct_draw_params_t *params)
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
      float fd[4];
      float mag_ratio;

      cairo_set_source_rgb(cr,
          (double)params->patch_colors[idx].r,
          (double)params->patch_colors[idx].g,
          (double)params->patch_colors[idx].b);

      /* 4 rectangle edges per patch */
      for( k = 0; k < 4; k++ )
        Cairo_Draw_Line(cr,
            segm[j + k].x1, segm[j + k].y1,
            segm[j + k].x2, segm[j + k].y2);

      /* Draw flow arrow when current data is above threshold */
      if( params->show_flow )
      {
        int fstep = calc_data.freq_step;
        get_precomputed_flow_data(fstep, idx, fd);
        mag_ratio = (float)sqrt(
            fd[0] * fd[0] + fd[1] * fd[1] +
            fd[2] * fd[2] + fd[3] * fd[3]);

        if( mag_ratio > FLOW_MAG_THRESHOLD )
        {
          arrow_seg_3d_t arrow[ARROW_LINE_COUNT];
          patch_arrow_to_world(idx, fd, flow_phase, arrow);

          for( k = 0; k < ARROW_LINE_COUNT; k++ )
          {
            Segment_t s;
            Set_Gdk_Segment(&s, v, scale,
                arrow[k].x1, arrow[k].y1, arrow[k].z1,
                arrow[k].x2, arrow[k].y2, arrow[k].z2);
            Cairo_Draw_Line(cr, s.x1, s.y1, s.x2, s.y2);
          }
        }
      }
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
  draw_surface_patches(cr, v, scale, structure_segs + data.n, data.m, params);
  draw_wire_segments(cr, v, structure_segs, data.n, params);

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/**
 * cairo_set_status() - Store status message for deferred rendering
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
 * cairo_set_gradient() - Store pre-resolved gradient legend surface
 * @ctx:     cairo_render_ctx_t*
 * @surface: ARGB32 gradient surface from gradient_cache
 */
  void
cairo_set_gradient(void *ctx, cairo_surface_t *surface)
{
  cairo_render_ctx_t *cc = (cairo_render_ctx_t *)ctx;
  cc->gradient = surface;
}

/*-----------------------------------------------------------------------*/
