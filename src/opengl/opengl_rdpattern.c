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

#include "opengl_rdpattern.h"
#include "opengl_rdpattern_geometry.h"
#include "opengl_settings.h"
#include "opengl_structure.h"
#include "opengl_structure_geometry.h"
#include "../shared.h"
#include "../rdpattern_ui.h"

#ifdef HAVE_OPENGL

#include "../opengl-engine/opengl_view.h"
#include "../opengl-engine/opengl_view_notice.h"
#include "../render/render_dispatch.h"

/* Surface color dim values in rc_config.brightness_rdpat_surface
 * and rc_config.brightness_rdpat_wire (applied via u_color_dim
 * shader uniform in the engine).
 *
 * In "both" mode the surface is dimmed further so wireframe lines
 * stand out.  The ratio preserves the original relationship between
 * RDPAT_SURFACE_BOTH_DIM (0.30) and RDPAT_SURFACE_DIM (0.47). */
#define RDPAT_BOTH_SURFACE_DIM_RATIO 0.64f


/* Translated far-field points buffer for excitation center offset */
static point_3d_t *rdpat_translated_points = NULL;
static int rdpat_translated_capacity = 0;

/* Widget pointer for external access */
static GtkWidget *rdpattern_gl_widget = NULL;


/* Overlay configuration for structure rendering in rdpattern */
static const gl_overlay_config_t rdpattern_overlay_config = {
  .vertex_shader_path = "/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "/gl/lit-color-fragment.glsl",
  .attribs = opengl_chevron_attribs,
  .attrib_count = 7
};


/*-----------------------------------------------------------------------*/

/** gl_rdpat_draw_nearfield() - Near-field leaf: convert prerendered vectors to GL batches
 * @ctx:      gl_view_state_s (passed as void* through render_ops_t)
 * @origins:  sample point positions [npts]
 * @npts:     number of near-field sample points
 * @fields:   dispatch-resolved vector sets (0-3 active field types)
 * @n_fields: number of active field sets
 * @dr:       normalization scale distance
 * @r_max:    maximum distance from origin for view scaling
 *
 * One GL batch per field set. Backend iterates — zero field-type branching.
 */
  static gboolean
gl_rdpat_draw_nearfield(void *ctx,
    const near_field_point_t *origins, int npts,
    const nf_field_set_t *fields, int n_fields,
    double dr, double r_max)
{
  gl_view_state_t *state = (gl_view_state_t *)ctx;
  gl_view_content_t *out = &state->content;
  int total_lines;

  total_lines = opengl_rdpattern_generate_nf_field_lines(
      origins, npts, fields, n_fields, dr);
  if( total_lines <= 0 )
    return FALSE;

  /* Near-field positions overlap structure in same coordinate space */
  {
    int nf_count;
    lit_color_point_t *nf_buf;

    nf_buf = opengl_rdpattern_get_nf_lines(&nf_count);
    out->batches[0].vertices = nf_buf;
    out->batches[0].vertex_count = nf_count * 2;
    out->batches[0].draw_mode = GL_LINES;
    out->batches[0].color_dim = rc_config.brightness_nearfield;
    out->batches[0].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_nearfield);
    out->batch_count = 1;
  }

  out->vertex_stride = (int)sizeof(lit_color_point_t);
  out->r_max = (float)r_max;
  out->clip_extent = (float)(r_max + dr);
  out->model_scale = 1.0f;
  out->generation = opengl_rdpattern_get_nf_generation();

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/** gl_rdpat_draw_farfield() - Far-field leaf: tessellate gain pattern and populate batches
 * @ctx:    gl_view_state_s (passed as void* through render_ops_t)
 * @_fstep: current frequency step, indexes ff_pre[] for vertex data
 * @ff:     dispatch-resolved far-field draw parameters
 *
 * Returns TRUE when batches are populated, FALSE on data dependency failure.
 */
  static gboolean
gl_rdpat_draw_farfield(void *ctx, int _fstep, const ff_draw_params_t *ff)
{
  static float last_ff_off_len = NAN;
  static uint32_t last_ff_gen = 0;
  static int last_draw_style = -1;
  gl_view_state_t *state = (gl_view_state_t *)ctx;
  gl_view_content_t *out = &state->content;
  uint32_t current_gen;
  double r_min, r_range;
  int nth, nph, npts;
  point_3d_t *verts;
  point_3d_t *points_to_use;
  gboolean translate_to_excitation;

  if( ff_pre == NULL || _fstep < 0 )
    return FALSE;

  ff_pre_t *fp = &ff_pre[_fstep];
  nth = fpat.nth;
  nph = fpat.nph;
  npts = nth * nph;

  if( npts <= 0 || fp->vertices == NULL )
    return FALSE;

  verts = fp->vertices;
  current_gen = fp->generation;
  r_min = (double)fp->r_min;
  r_range = (double)(fp->pattern_radius - fp->r_min);

  translate_to_excitation = (ff->off_len > 0.001f);
  points_to_use = verts;

  if( translate_to_excitation )
  {
    int i;

    if( npts > rdpat_translated_capacity )
    {
      size_t mreq = (size_t)npts * sizeof(point_3d_t);
      mem_realloc((void **)&rdpat_translated_points, mreq, __LOCATION__);
      rdpat_translated_capacity = npts;
    }

    /* ff->x/y/z already pre-scaled to pattern space by dispatch */
    for( i = 0; i < npts; i++ )
    {
      rdpat_translated_points[i].x = verts[i].x + (double)ff->x;
      rdpat_translated_points[i].y = verts[i].y + (double)ff->y;
      rdpat_translated_points[i].z = verts[i].z + (double)ff->z;
      rdpat_translated_points[i].r = verts[i].r;
    }

    points_to_use = rdpat_translated_points;
  }

  /* Regenerate geometry on data change, translation change, or draw style change */
  if( current_gen != last_ff_gen ||
      rc_config.rdpattern_draw_style != last_draw_style ||
      ff->off_len != last_ff_off_len )
  {
    gboolean need_tris =
      (rc_config.rdpattern_draw_style == RDPAT_STYLE_SURFACE ||
       rc_config.rdpattern_draw_style == RDPAT_STYLE_BOTH);
    gboolean need_lines =
      (rc_config.rdpattern_draw_style == RDPAT_STYLE_WIREFRAME ||
       rc_config.rdpattern_draw_style == RDPAT_STYLE_BOTH);

    if( need_tris )
    {
      int tri_count = opengl_rdpattern_generate_triangles(
          points_to_use, nth, nph, r_min, r_range);

      if( tri_count <= 0 )
        return FALSE;
    }

    if( need_lines )
    {
      int line_count = opengl_rdpattern_generate_lines(
          points_to_use, nth, nph, r_min, r_range);

      if( line_count <= 0 )
        return FALSE;
    }

    last_ff_gen = current_gen;
    last_draw_style = rc_config.rdpattern_draw_style;
    last_ff_off_len = ff->off_len;
  }

  /* Populate batches per draw style */
  switch( rc_config.rdpattern_draw_style )
  {
    case RDPAT_STYLE_SURFACE:
    {
      int tri_count;
      lit_color_triangle_t *tri_buf =
        opengl_rdpattern_get_triangles(&tri_count);

      if( tri_count == 0 )
        return FALSE;

      out->batches[0].vertices = tri_buf;
      out->batches[0].vertex_count = tri_count * 3;
      out->batches[0].draw_mode = GL_TRIANGLES;
      out->batches[0].polygon_offset = FALSE;
      out->batches[0].color_dim = rc_config.brightness_rdpat_surface;
      out->batches[0].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_rdpat_surface);
      out->batch_count = 1;
      break;
    }

    case RDPAT_STYLE_WIREFRAME:
    {
      int line_count;
      lit_color_point_t *line_buf =
        opengl_rdpattern_get_lines(&line_count);

      if( line_count == 0 )
        return FALSE;

      out->batches[0].vertices = line_buf;
      out->batches[0].vertex_count = line_count * 2;
      out->batches[0].draw_mode = GL_LINES;
      out->batches[0].polygon_offset = FALSE;
      out->batches[0].color_dim = rc_config.brightness_rdpat_wire;
      out->batches[0].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_rdpat_wire);
      out->batch_count = 1;
      break;
    }

    case RDPAT_STYLE_BOTH:
    {
      int tri_count, line_count;
      lit_color_triangle_t *tri_buf =
        opengl_rdpattern_get_triangles(&tri_count);
      lit_color_point_t *line_buf =
        opengl_rdpattern_get_lines(&line_count);

      if( tri_count == 0 || line_count == 0 )
        return FALSE;

      /* Surface pushed behind wireframe via glPolygonOffset */
      out->batches[0].vertices = tri_buf;
      out->batches[0].vertex_count = tri_count * 3;
      out->batches[0].draw_mode = GL_TRIANGLES;
      out->batches[0].polygon_offset = TRUE;
      out->batches[0].color_dim =
          rc_config.brightness_rdpat_surface * RDPAT_BOTH_SURFACE_DIM_RATIO;
      out->batches[0].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_rdpat_surface);

      out->batches[1].vertices = line_buf;
      out->batches[1].vertex_count = line_count * 2;
      out->batches[1].draw_mode = GL_LINES;
      out->batches[1].polygon_offset = FALSE;
      out->batches[1].color_dim = rc_config.brightness_rdpat_wire;
      out->batches[1].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_rdpat_wire);
      out->batch_count = 2;
      break;
    }

    default:
      pr_err("rdpattern: invalid draw style %d, using surface\n",
          rc_config.rdpattern_draw_style);
      rc_config.rdpattern_draw_style = RDPAT_STYLE_SURFACE;
      return FALSE;
  }

  out->vertex_stride = (int)sizeof(lit_color_point_t);

  /* pattern_radius from dispatch (ff_presentation_recompute result) */
  out->r_max = ff->pattern_radius;

  /* Clip extent accounts for excitation center translation */
  out->clip_extent = ff->pattern_radius + ff->off_len;
  out->model_scale = 1.0f;
  out->generation = opengl_rdpattern_get_ff_generation();

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/* Backend vtable for radiation pattern GL scene.
 * draw_structure is NULL: render_check never resolves RENDER_MODE_STRUCTURE
 * for VIEW_RDPATTERN. */
static const render_ops_t gl_rdpat_ops =
{
  .draw_farfield  = gl_rdpat_draw_farfield,
  .draw_nearfield = gl_rdpat_draw_nearfield,
  .draw_structure       = NULL,
  .draw_axes      = NULL,
  .init_empty     = gl_view_init_empty,
  .set_status     = gl_view_set_status,
  .set_gradient   = gl_view_set_gradient,
};

/*-----------------------------------------------------------------------*/

/** rdpattern_scene_generate() - Scene provider generate callback
 * @state: view state; scene writes into state->content
 *
 * Delegates to unified render() dispatch with state as ctx.
 */
  static gboolean
rdpattern_scene_generate(gl_view_state_t *state)
{
  opengl_structure_show_ctrl_notice(rdpattern_gl_widget);

  return render((void *)state, &gl_rdpat_ops, rdpattern_view);
}

/*-----------------------------------------------------------------------*/

/** rdpattern_scene_post_render() - Scene provider post-render callback */
  static void
rdpattern_scene_post_render(void)
{
}

/*-----------------------------------------------------------------------*/

/** rdpattern_scene_cleanup() - Scene provider cleanup callback
 */
  static void
rdpattern_scene_cleanup(void)
{
  opengl_rdpattern_geometry_cleanup();

  free_ptr((void **)&rdpat_translated_points);
  rdpat_translated_capacity = 0;
}

/*-----------------------------------------------------------------------*/

/** rdpattern_on_shift_scroll() - Shift+scroll handler for adjusting overlay structure scale
 * @widget: source scroll widget
 * @event: scroll event
 * @view_state: pointer to gl_view_state_t
 */
  static gboolean
rdpattern_on_shift_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer view_state)
{
  gl_view_state_t *state;
  double scale;

  state = (gl_view_state_t *)view_state;

  if( !state || !isFlagSet(OVERLAY_STRUCT) )
    return( FALSE );

  /* Scale adjustment only applies to far-field view */
  if( isFlagClear(DRAW_GAIN) )
    return( FALSE );

  /* Compute scale factor matching zoom behavior */
  scale = compute_zoom_scale(
      (int)(state->viewport_height * state->aspect),
      (int)state->viewport_height,
      rc_config.rdpattern_overlay_scale_adj * 100.0);

  if( event->direction == GDK_SCROLL_UP )
    rc_config.rdpattern_overlay_scale_adj *= (1.0 + 0.1 * scale);
  else if( event->direction == GDK_SCROLL_DOWN )
    rc_config.rdpattern_overlay_scale_adj /= (1.0 + 0.1 * scale);
  else
    return( FALSE );

  xnec2_widget_queue_draw(widget, TRUE);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/**
 * rdpattern_overlay_generate() - Overlay provider generate callback
 * @primary: primary scene content (rdpattern geometry)
 * @out: overlay scene content to populate
 *
 * Returns structure geometry for overlay when OVERLAY_STRUCT is set.
 * Derives model_scale from ratio of pattern extent to structure extent.
 */
  static gboolean
rdpattern_overlay_generate(const gl_view_content_t *primary, gl_view_content_t *out)
{
  const structure_overlay_data_t *geom;
  static gboolean notice_shown = FALSE;

  if( !isFlagSet(OVERLAY_STRUCT) ||
      (data.n <= 0 && data.m <= 0) || !primary )
    return( FALSE );

  /* Show notice on first activation */
  if( !notice_shown && rdpattern_gl_widget )
  {
    gl_view_show_notice(rdpattern_gl_widget, "Shift+Scroll to Scale Structure",
        2500, GL_NOTICE_BOTTOM_LEFT);
    notice_shown = TRUE;
  }

  geom = opengl_structure_get_shared_geometry();

  if( !geom || geom->batch_count <= 0 )
    return( FALSE );

  memcpy(out->batches, geom->batches,
      (size_t)geom->batch_count * sizeof(geom->batches[0]));
  out->batch_count = geom->batch_count;
  out->vertex_stride = geom->vertex_stride;
  out->show_gradient = FALSE;

  /* Far-field: overlay_base_scale from prerender includes the default extent
   * factor (FF_OVERLAY_DEFAULT_EXTENT); gl_overlay_effective_scale applies
   * the interactive scale_adj on top.
   * Near-field: structure and field vectors share the same space (scale 1.0). */
  float structure_extent = (float)geom_pre.scene_radius;
  int fstep = calc_data.freq_step;
  if( isFlagSet(DRAW_GAIN) && structure_extent > 0.0f
      && ff_pre != NULL && fstep >= 0 )
  {
    out->model_scale = ff_pre[fstep].overlay_base_scale;
    out->scale_adj_locked = FALSE;
  }
  else
  {
    out->model_scale = 1.0f;
    out->scale_adj_locked = TRUE;
  }

  out->r_max = structure_extent;
  out->clip_extent = structure_extent;
  out->generation = geom->generation;

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/** rdpattern_axes_is_active() - Report whether axes renderable should be drawn
 * @ctx: unused context pointer
 *
 * Axes are meaningful only when a pattern is rendered — hidden when
 * neither gain nor near-field is selected.
 */
  static gboolean
rdpattern_axes_is_active(void *ctx)
{
  (void)ctx;
  return( isFlagSet(DRAW_GAIN) || isFlagSet(DRAW_EHFIELD) );
}

/*-----------------------------------------------------------------------*/

/* Static scene configuration and provider */
static gl_view_config_t rdpattern_view_config = {
  .vertex_shader_path = "/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "/gl/lit-color-fragment.glsl",
  .attribs = opengl_structure_attribs,
  .attrib_count = 3,
  .vertex_stride = (int)sizeof(lit_color_point_t),
  .has_gradient = TRUE,
  .gradient_draw = Draw_Color_Legend_Overlay,
  .on_gl_init_failed = opengl_gl_init_failed
};

static gl_scene_provider_t rdpattern_scene_provider = {
  .generate = rdpattern_scene_generate,
  .post_render = rdpattern_scene_post_render,
  .cleanup = rdpattern_scene_cleanup,
  .overlay_config = &rdpattern_overlay_config,
  .overlay_generate = rdpattern_overlay_generate,
  .overlay_cleanup = NULL,
  .on_shift_scroll = rdpattern_on_shift_scroll,
  .on_ctrl_scroll = opengl_structure_on_ctrl_scroll,
  .axes_is_active = rdpattern_axes_is_active
};

/*-----------------------------------------------------------------------*/

/** rdpattern_view_changed_cb() - view_t change callback for rdpattern view
 * @v:           view that changed (rdpattern_view)
 * @_user_data:  unused
 *
 * Queues a redraw on the rdpattern GL widget and Cairo path and
 * refreshes the rdpattern WR/WI spin display.  Bound as changed_cb at
 * view_new() in callbacks.c; when sharing is active the master
 * (structure_view) reaches this callback via its rotation_follower
 * link inside view_notify_change().
 */
  void
rdpattern_view_changed_cb(view_t *v, gpointer _user_data)
{
  (void)_user_data;

  view_update_spin_display( v );
  Queue_Radiation_Redraw();

  if( rdpattern_gl_widget )
    xnec2_widget_queue_draw( rdpattern_gl_widget, TRUE );

} /* rdpattern_view_changed_cb() */

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_create_widget_impl() - Create the OpenGL radiation pattern widget using the generic view engine
 */
  static GtkWidget*
opengl_rdpattern_create_widget_impl(void)
{
  if( rdpattern_view == NULL )
    return( NULL );

  rdpattern_gl_widget = gl_view_create_widget(
      &rdpattern_view_config,
      &rdpattern_scene_provider,
      rdpattern_view );

  gtk_widget_show(rdpattern_gl_widget);

  return( rdpattern_gl_widget );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_cleanup_impl() - Free rdpattern GL resources
 */
  static void
opengl_rdpattern_cleanup_impl(void)
{
  rdpattern_scene_cleanup();
  rdpattern_gl_widget = NULL;
}

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_create_widget() - Public API: create radiation pattern GL widget
 */
  GtkWidget*
opengl_rdpattern_create_widget(void)
{
#ifdef HAVE_OPENGL
  return( opengl_rdpattern_create_widget_impl() );
#else
  return( NULL );
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_cleanup() - Public API: cleanup rdpattern resources
 */
  void
opengl_rdpattern_cleanup(void)
{
#ifdef HAVE_OPENGL
  opengl_rdpattern_cleanup_impl();
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_get_widget() - Public API: return rdpattern GL widget
 */
  GtkWidget*
opengl_rdpattern_get_widget(void)
{
#ifdef HAVE_OPENGL
  return( rdpattern_gl_widget );
#else
  return( NULL );
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_queue_draw() - Public API: queue redraw of OpenGL radiation pattern widget
 */
  void
opengl_rdpattern_queue_draw(void)
{
#ifdef HAVE_OPENGL
  if( rdpattern_gl_widget != NULL )
    xnec2_widget_queue_draw( rdpattern_gl_widget, TRUE );
#endif
}
