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
#include "opengl_structure.h"
#include "opengl_structure_geometry.h"
#include "../shared.h"
#include "../draw_radiation.h"

#ifdef HAVE_OPENGL

#include "../opengl-engine/opengl_view.h"
#include "../opengl-engine/opengl_view_tooltip.h"

/* Default structure overlay extent as multiple of radiation pattern r_max */
#define OVERLAY_DEFAULT_EXTENT 1.25f

/* Last far-field NEC generation seen — staleness detection */
static unsigned int rdpat_last_ff_gen = 0;

/* Track translation state to detect changes */
static gboolean rdpat_last_translate_state = FALSE;

/* Cache overlay model scale adjustment for translation */
static double rdpat_ovl_scale_adj = 1.0;
static double rdpat_last_ovl_scale_adj = 1.0;

/* Translated far-field points buffer for excitation center offset */
static point_3d_t *rdpat_translated_points = NULL;
static int rdpat_translated_capacity = 0;

/* Widget pointer for external access */
static GtkWidget *rdpattern_gl_widget = NULL;

/* Arcball for radiation pattern view */
static arcball_state_t *rdpattern_arcball = NULL;


/* Overlay configuration for structure rendering in rdpattern */
static const gl_overlay_config_t rdpattern_overlay_config = {
  .vertex_shader_path = "/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "/gl/lit-color-fragment.glsl",
  .attribs = opengl_structure_attribs,
  .attrib_count = 3
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

/** rdpattern_init_empty_scene() - Populate a minimal scene with no geometry
 * @out: scene content to initialize
 * @zoom: zoom factor to set in the scene
 *
 * Caller sets status_message after return.  Only non-zero fields are set;
 * caller provides the struct zero-initialized via {0}.
 */
  static void
rdpattern_init_empty_scene(gl_view_content_t *out, float zoom)
{
  out->draw_mode = GL_TRIANGLES;
  out->r_max = 1.0f;
  out->clip_extent = 1.0f;
  out->zoom = zoom;
  out->model_scale = 1.0f;
}

/*-----------------------------------------------------------------------*/

  static gboolean
rdpattern_scene_generate(gl_view_content_t *out)
{
  float zoom;
  float r_max;
  gboolean translate_to_excitation;
  float offset_x, offset_y, offset_z;
  const structure_overlay_data_t *geom = NULL;

  /* Zoom from rdpattern spinbutton, normalized to multiplier */
  zoom = 1.0f;
  if( rdpattern_zoom )
    zoom = (float)gtk_spin_button_get_value(rdpattern_zoom) / 100.0f;

  if( zoom < 0.01f )
    zoom = 0.01f;

  /* Check if we need to translate pattern to excitation center */
  translate_to_excitation = FALSE;
  offset_x = 0.0f;
  offset_y = 0.0f;
  offset_z = 0.0f;

  if( isFlagSet(OVERLAY_STRUCT) )
  {
    geom = opengl_structure_get_shared_geometry();
    if( geom && geom->has_excitation_center )
    {
      translate_to_excitation = TRUE;

      offset_x = (float)geom->excitation_center_x;
      offset_y = (float)geom->excitation_center_y;
      offset_z = (float)geom->excitation_center_z;
    }
  }

  /* Near-field rendering path */
  if( isFlagSet(DRAW_EHFIELD) && isFlagSet(ENABLE_NEAREH) && near_field.valid )
  {
    int line_count;
    int nf_count;
    lit_color_point_t *nf_buf;

    line_count = opengl_rdpattern_generate_nf_lines();
    if( line_count <= 0 )
      return( FALSE );

    r_max = (float)near_field.r_max;

    /* Near-field positions directly overlap structure in same coordinate space;
     * no translation needed (excitation already aligned) */

    nf_buf = opengl_rdpattern_get_nf_lines(&nf_count);
    out->vertices = nf_buf;
    out->vertex_count = nf_count * 2;
    out->vertex_stride = (int)sizeof(lit_color_point_t);
    out->draw_mode = GL_LINES;
    out->r_max = r_max;
    out->clip_extent = r_max;
    out->zoom = zoom;
    out->model_scale = 1.0f;
    out->show_gradient = FALSE;
    out->generation = opengl_rdpattern_get_nf_generation();

    return( TRUE );
  }

  /* Near-field selected but NE/NH cards absent or data not yet valid */
  if( isFlagSet(DRAW_EHFIELD) )
  {
    rdpattern_init_empty_scene(out, zoom);
    out->status_message = "Near field requires NE or NH cards in the NEC file";
    return( TRUE );
  }

  /* Far-field (gain) rendering path */
  if( isFlagSet(DRAW_GAIN) )
  {
    unsigned int current_gen;
    double r_min, r_range;
    rdpattern_data_t rd_data;
    point_3d_t *points_to_use;
    float off_len;

    off_len = 0.0f;

    /* No RP card — cannot compute gain pattern */
    if( isFlagClear(ENABLE_RDPAT) )
    {
      rdpattern_init_empty_scene(out, zoom);
      out->status_message = "Gain pattern requires an RP card in the NEC file";
      return( TRUE );
    }

    current_gen = Generate_Rdpattern_Data(&r_min, &r_range);
    if( current_gen == 0 )
      return( FALSE );

    if( !Get_Radiation_Pattern_Data(&rd_data) || !rd_data.valid )
      return( FALSE );

    r_max = (float)rd_data.r_max;

    points_to_use = rd_data.points;

    if( translate_to_excitation )
    {
      int npts, i;
      float model_scale;
      float scaled_off_x, scaled_off_y, scaled_off_z;

      npts = rd_data.nth * rd_data.nph;

      /* Scale offset from structure coords to radiation pattern coords */
      model_scale = rdpattern_overlay_base_scale(r_max, geom->view_scale)
          * (float)rdpat_ovl_scale_adj;

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

    /* Regenerate triangles on data change or translation state change or scale change */
    if( current_gen != rdpat_last_ff_gen ||
        translate_to_excitation != rdpat_last_translate_state ||
        (translate_to_excitation && rdpat_ovl_scale_adj != rdpat_last_ovl_scale_adj) )
    {
      int tri_count = opengl_rdpattern_generate_triangles(
          points_to_use, rd_data.nth, rd_data.nph, r_min, r_range);

      if( tri_count <= 0 )
        return( FALSE );

      rdpat_last_ff_gen = current_gen;
      rdpat_last_translate_state = translate_to_excitation;
      rdpat_last_ovl_scale_adj = rdpat_ovl_scale_adj;
    }

    {
      int tri_count;
      lit_color_triangle_t *tri_buf = opengl_rdpattern_get_triangles(&tri_count);

      if( tri_count == 0 )
        return( FALSE );

      out->vertices = tri_buf;
      out->vertex_count = tri_count * 3;
    }
    out->vertex_stride = (int)sizeof(lit_color_point_t);
    out->draw_mode = GL_TRIANGLES;

    out->r_max = r_max;

    /* Clip extent accounts for excitation center translation so
     * clip planes encompass shifted geometry without altering
     * camera distance or overlay scaling (which depend on r_max) */
    out->clip_extent = r_max + off_len;
    out->zoom = zoom;
    out->model_scale = 1.0f;
    out->show_gradient = isFlagSet(DRAW_GAIN) && rc_config.rdpattern_gradient_key;
    out->generation = opengl_rdpattern_get_ff_generation();

    return( TRUE );
  }

  /* Neither near-field nor far-field is active; return a minimal scene so
   * the render loop proceeds to clear the framebuffer to black */
  rdpattern_init_empty_scene(out, zoom);
  out->status_message = "Select Gain Pattern or Near Field";

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/** rdpattern_scene_post_render() - Scene provider post-render callback
 */
  static void
rdpattern_scene_post_render(void)
{
  Update_Rdpattern_UI();
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

  if( !state || isFlagClear(OVERLAY_STRUCT) )
    return( FALSE );

  /* Scale adjustment only applies to far-field (gain) view */
  if( isFlagClear(DRAW_GAIN) )
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

  /* Cache for use in translation calculation */
  rdpat_ovl_scale_adj = state->ovl_model_scale_adj;

  gtk_widget_queue_draw(widget);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/** rdpattern_overlay_generate() - Overlay provider generate callback
 * @primary: primary scene content (rdpattern geometry)
 * @out: overlay scene content to populate
 *
 * Returns structure geometry for overlay when OVERLAY_STRUCT is set.
 * Computes physical scale ratio from structure to radiation pattern.
 */
  static gboolean
rdpattern_overlay_generate(const gl_view_content_t *primary, gl_view_content_t *out)
{
  const structure_overlay_data_t *geom;
  static gboolean tooltip_shown = FALSE;

  if( isFlagClear(OVERLAY_STRUCT) || data.n <= 0 || !primary )
    return( FALSE );

  /* Show tooltip on first activation */
  if( !tooltip_shown && rdpattern_gl_widget )
  {
    gl_view_show_tooltip(rdpattern_gl_widget, "Shift Scroll to Scale Structure", 2500);
    tooltip_shown = TRUE;
  }

  /* Ensure shared geometry is fresh */
  opengl_structure_update_shared_geometry();
  geom = opengl_structure_get_shared_geometry();

  if( !geom || geom->vertex_count <= 0 )
    return( FALSE );

  out->vertices = geom->vertices;
  out->vertex_count = geom->vertex_count;
  out->vertex_stride = geom->vertex_stride;
  out->draw_mode = geom->draw_mode;
  out->show_gradient = FALSE;

  /* Scale structure to match radiation pattern space for far-field,
   * use 1:1 for near-field (both already in meters) */
  if( isFlagSet(DRAW_GAIN) )
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
  .gradient_draw = Draw_Color_Legend_Overlay
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

/** rdpattern_arcball_changed_cb() - Arcball change callback for constrained rotation mode
 * @ab: arcball that changed
 * @_user_data: unused
 *
 * Syncs arcball WR/WI angles back to rdpattern_proj_params and
 * spin button display text when COMMON_PROJECTION is off.
 */
  static void
rdpattern_arcball_changed_cb(arcball_state_t *ab, gpointer _user_data)
{
  float wr, wi;

  (void)_user_data;

  if( arcball_get_drag_mode(ab) != ARCBALL_DRAG_CONSTRAINED )
    return;

  /* When common projection is active, the structure arcball is
   * the single point of truth — structure callback handles sync */
  if( isFlagSet(COMMON_PROJECTION) )
    return;

  arcball_get_angles(ab, &wr, &wi);

  rdpattern_proj_params.Wr = (double)wr;
  rdpattern_proj_params.Wi = (double)wi;

  opengl_update_spin_display( rotate_rdpattern, rdpattern_proj_params.Wr );
  opengl_update_spin_display( incline_rdpattern, rdpattern_proj_params.Wi );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_create_widget_impl() - Create the OpenGL radiation pattern widget using the generic view engine
 */
  static GtkWidget*
opengl_rdpattern_create_widget_impl(void)
{
  if( !rdpattern_arcball )
  {
    rdpattern_arcball = arcball_new((float)MOTION_EVENTS_COUNT);

    /* Initialize mode from rc_config */
    arcball_set_drag_mode(rdpattern_arcball,
        rc_config.arcball_constrained_rotation ?
        ARCBALL_DRAG_CONSTRAINED : ARCBALL_DRAG_FREE);

    /* Sync constrained rotation back to WR/WI and spin buttons */
    arcball_add_callback(rdpattern_arcball,
        rdpattern_arcball_changed_cb, NULL);
  }

  rdpattern_gl_widget = gl_view_create_widget(
      &rdpattern_view_config,
      &rdpattern_scene_provider,
      rdpattern_arcball,
      &rdpattern_zoom);

  gtk_widget_show(rdpattern_gl_widget);

  return( rdpattern_gl_widget );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_get_arcball() - Return reference to rdpattern arcball
 */
  arcball_state_t*
opengl_rdpattern_get_arcball(void)
{
  return( rdpattern_arcball );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_cleanup_impl() - Free rdpattern GL resources
 */
  static void
opengl_rdpattern_cleanup_impl(void)
{
  rdpattern_scene_cleanup();
  rdpattern_gl_widget = NULL;

  if( rdpattern_arcball )
  {
    arcball_free(rdpattern_arcball);
    rdpattern_arcball = NULL;
  }
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
    xnec2_widget_queue_draw( rdpattern_gl_widget );
#endif
}
