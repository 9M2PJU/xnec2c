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
#include "../draw_radiation.h"

#ifdef HAVE_OPENGL

#include "../opengl-engine/opengl_view.h"
#include "../opengl-engine/opengl_view_notice.h"
#include "../render/render_dispatch.h"

/* Default structure overlay extent as multiple of radiation pattern r_max */
#define OVERLAY_DEFAULT_EXTENT 1.25f

/* Surface color dim values in rc_config.brightness_rdpat_surface
 * and rc_config.brightness_rdpat_wire (applied via u_color_dim
 * shader uniform in the engine).
 *
 * In "both" mode the surface is dimmed further so wireframe lines
 * stand out.  The ratio preserves the original relationship between
 * RDPAT_SURFACE_BOTH_DIM (0.30) and RDPAT_SURFACE_DIM (0.47). */
#define RDPAT_BOTH_SURFACE_DIM_RATIO 0.64f


/* Overlay model scale adjustment — written by scroll handler, read by scene generator */
static double rdpat_ovl_scale_adj = 1.0;

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

/** rdpattern_overlay_base_scale() - Calculate base scale factor mapping structure to rdpattern coordinate space
 * @r_max: radiation pattern maximum radius
 * @view_scale: structure geometry extent
 */
  static float
rdpattern_overlay_base_scale(float r_max, float view_scale)
{
  if( view_scale > 0.001f )
    return( OVERLAY_DEFAULT_EXTENT * r_max / view_scale );

  return( 1.0f );
}

/*-----------------------------------------------------------------------*/

/** gl_rdpat_draw_nearfield() - Near-field leaf: convert prerendered vectors to GL batches
 * @ctx:      gl_view_content_t to fill
 * @zoom:     current zoom factor
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
gl_rdpat_draw_nearfield(void *ctx, float zoom,
    const near_field_point_t *origins, int npts,
    const nf_field_set_t *fields, int n_fields,
    double dr, double r_max)
{
  gl_view_content_t *out = (gl_view_content_t *)ctx;
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
  out->zoom = zoom;
  out->model_scale = 1.0f;
  out->generation = opengl_rdpattern_get_nf_generation();

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/** gl_rdpat_draw_farfield() - Far-field leaf: tessellate gain pattern and populate batches
 * @ctx:   gl_view_content_t to fill
 * @_fstep: current frequency step (unused; Generate_Rdpattern_Data reads global state)
 * @zoom:  current zoom factor
 * @ff: dispatch-resolved far-field draw parameters
 *
 * Returns TRUE when batches are populated, FALSE on data dependency failure.
 */
  static gboolean
gl_rdpat_draw_farfield(void *ctx, int _fstep, float zoom, const ff_draw_params_t *ff)
{
  static double last_ff_scale_adj = 1.0;
  static unsigned int last_ff_gen = 0;
  static int last_draw_style = -1;
  static gboolean last_translate_state = FALSE;
  gl_view_content_t *out = (gl_view_content_t *)ctx;
  gboolean translate_to_excitation;
  float offset_x, offset_y, offset_z;
  unsigned int current_gen;
  double r_min, r_range;
  rdpattern_data_t rd_data;
  point_3d_t *points_to_use;
  float off_len;
  float r_max;

  off_len = 0.0f;
  translate_to_excitation = ff->active;
  offset_x = ff->x;
  offset_y = ff->y;
  offset_z = ff->z;

  current_gen = Generate_Rdpattern_Data(&r_min, &r_range);
  if( current_gen == 0 )
    return FALSE;

  if( !Get_Radiation_Pattern_Data(&rd_data) || !rd_data.valid )
    return FALSE;

  r_max = (float)rd_data.r_max;
  points_to_use = rd_data.points;

  if( translate_to_excitation )
  {
    int npts, i;
    float model_scale;
    float scaled_off_x, scaled_off_y, scaled_off_z;

    npts = rd_data.nth * rd_data.nph;

    /* Scale offset from structure coords to radiation pattern coords */
    model_scale = rdpattern_overlay_base_scale(r_max, ff->view_scale)
        * (float)ff->scale_adj;

    scaled_off_x = offset_x * model_scale;
    scaled_off_y = offset_y * model_scale;
    scaled_off_z = offset_z * model_scale;

    off_len = (float)sqrt(
        (double)(scaled_off_x * scaled_off_x +
                 scaled_off_y * scaled_off_y +
                 scaled_off_z * scaled_off_z));

    if( npts > rdpat_translated_capacity )
    {
      size_t mreq = (size_t)npts * sizeof(point_3d_t);
      mem_realloc((void **)&rdpat_translated_points, mreq, __LOCATION__);
      rdpat_translated_capacity = npts;
    }

    for( i = 0; i < npts; i++ )
    {
      rdpat_translated_points[i].x = rd_data.points[i].x + (double)scaled_off_x;
      rdpat_translated_points[i].y = rd_data.points[i].y + (double)scaled_off_y;
      rdpat_translated_points[i].z = rd_data.points[i].z + (double)scaled_off_z;
      rdpat_translated_points[i].r = rd_data.points[i].r;
    }

    points_to_use = rdpat_translated_points;
  }

  /* Regenerate geometry on data change, translation state change,
   * scale change, or draw style change */
  if( current_gen != last_ff_gen ||
      rc_config.rdpattern_draw_style != last_draw_style ||
      translate_to_excitation != last_translate_state ||
      (translate_to_excitation && ff->scale_adj != last_ff_scale_adj) )
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
          points_to_use, rd_data.nth, rd_data.nph, r_min, r_range);

      if( tri_count <= 0 )
        return FALSE;
    }

    if( need_lines )
    {
      int line_count = opengl_rdpattern_generate_lines(
          points_to_use, rd_data.nth, rd_data.nph, r_min, r_range);

      if( line_count <= 0 )
        return FALSE;
    }

    last_ff_gen = current_gen;
    last_draw_style = rc_config.rdpattern_draw_style;
    last_translate_state = translate_to_excitation;
    last_ff_scale_adj = ff->scale_adj;
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

  out->r_max = r_max;

  /* Clip extent accounts for excitation center translation so
   * clip planes encompass shifted geometry without altering
   * camera distance or overlay scaling (which depend on r_max) */
  out->clip_extent = r_max + off_len;
  out->zoom = zoom;
  out->model_scale = 1.0f;
  out->generation = opengl_rdpattern_get_ff_generation();

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/* Backend vtable for radiation pattern GL scene.
 * draw_structure is NULL: render_check never resolves RENDER_MODE_STRUCTURE
 * for RENDER_VIEW_RDPATTERN. */
static const render_ops_t gl_rdpat_ops =
{
  .draw_farfield  = gl_rdpat_draw_farfield,
  .draw_nearfield = gl_rdpat_draw_nearfield,
  .draw_structure = NULL,
  .init_empty     = gl_view_init_empty,
  .set_status     = gl_view_set_status,
  .set_gradient   = gl_view_set_gradient,
};

/*-----------------------------------------------------------------------*/

/** rdpattern_scene_generate() - Scene provider generate callback
 * @out: view content to populate
 *
 * Thin wrapper: extracts zoom and delegates to unified render() dispatch.
 */
  static gboolean
rdpattern_scene_generate(gl_view_content_t *out)
{
  float zoom;

  opengl_structure_show_ctrl_notice(rdpattern_gl_widget);

  /* Zoom from view->zoom; view_set_zoom() is the authoritative writer. */
  zoom = (rdpattern_view != NULL) ? rdpattern_view->zoom : 1.0f;
  if( zoom < 0.01f )
    zoom = 0.01f;

  return render((void *)out, &gl_rdpat_ops, RENDER_VIEW_RDPATTERN, zoom,
      rdpat_ovl_scale_adj);
}

/*-----------------------------------------------------------------------*/

/** rdpattern_scene_post_render() - Scene provider post-render callback
 *
 * Updates rdpattern-window UI readouts after each rendered frame.
 */
  static void
rdpattern_scene_post_render(void)
{
  g_rec_mutex_lock(&freq_data_lock);
  Update_Rdpattern_UI();
  g_rec_mutex_unlock(&freq_data_lock);
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

  if( !state || !render_get_last_rdpat_check()->overlay_active )
    return( FALSE );

  /* Scale adjustment only applies to far-field view */
  if( render_get_last_rdpat_check()->mode != RENDER_MODE_FARFIELD )
    return( FALSE );

  /* Compute scale factor matching zoom behavior */
  scale = compute_zoom_scale(
      (int)(state->viewport_height * state->aspect),
      (int)state->viewport_height,
      state->ovl_model_scale_adj * 100.0);

  if( event->direction == GDK_SCROLL_UP )
    state->ovl_model_scale_adj *= (1.0 + 0.1 * scale);
  else if( event->direction == GDK_SCROLL_DOWN )
    state->ovl_model_scale_adj /= (1.0 + 0.1 * scale);
  else
    return( FALSE );

  rdpat_ovl_scale_adj = state->ovl_model_scale_adj;

  xnec2_widget_queue_draw(widget, TRUE);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/** rdpattern_overlay_generate() - Overlay provider generate callback
 * @primary: primary scene content (rdpattern geometry)
 * @out: overlay scene content to populate
 *
 * Returns structure geometry for overlay when dispatch resolved overlay_active.
 * Computes physical scale ratio from structure to radiation pattern.
 */
  static gboolean
rdpattern_overlay_generate(const gl_view_content_t *primary, gl_view_content_t *out)
{
  const structure_overlay_data_t *geom;
  static gboolean notice_shown = FALSE;

  if( !render_get_last_rdpat_check()->overlay_active ||
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

  /* Scale structure to match radiation pattern space for far-field,
   * use 1:1 for near-field (both already in meters) */
  if( render_get_last_rdpat_check()->mode == RENDER_MODE_FARFIELD )
  {
    out->model_scale = rdpattern_overlay_base_scale(primary->r_max, geom->view_scale);
    out->scale_adj_locked = FALSE;
  }
  else
  {
    out->model_scale = 1.0f;
    out->scale_adj_locked = TRUE;
  }

  /* Structure raw extent for overlay clip plane calculation */
  out->r_max = geom->view_scale;
  out->clip_extent = geom->view_scale;
  out->zoom = 1.0f;
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
  return( render_get_last_rdpat_check()->mode != RENDER_MODE_NONE );
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
