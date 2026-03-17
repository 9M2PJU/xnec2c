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

#include "opengl_view.h"
#include "opengl_view_scene.h"
#include "opengl_view_overlay.h"
#include "opengl_view_input.h"
#include "opengl_view_render.h"
#include "opengl_view_msaa.h"
#include "opengl_view_tooltip.h"
#include "opengl_axes.h"
#include "opengl_cairo_overlay.h"
#include "opengl_ground_plane.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/* gl_view_state_free()
 *
 * Free view state resources
 */
  static void
gl_view_state_free(gl_view_state_t *state)
{
  if( !state )
    return;

  if( state->tooltip_timeout_id )
  {
    g_source_remove(state->tooltip_timeout_id);
    state->tooltip_timeout_id = 0;
  }

  g_free(state->tooltip_text);

  if( state->tooltip_surface )
    cairo_surface_destroy(state->tooltip_surface);

  if( state->tooltip_overlay )
    cairo_gl_overlay_free(state->tooltip_overlay);

  /* Destroy all renderables in reverse registration order */
  if( state->renderables )
  {
    int ri;

    for( ri = (int)state->renderables->len - 1; ri >= 0; ri-- )
    {
      gl_renderable_t *r = &g_array_index(
          state->renderables, gl_renderable_t, ri);

      if( r->destroy )
        r->destroy(r->ctx);
    }

    g_array_free(state->renderables, TRUE);
    state->renderables = NULL;
  }

  if( state->overlay )
    gradient_overlay_free(state->overlay);

  gl_view_msaa_free(state);

  g_free(state);

} /* gl_view_state_free() */

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler
 */
  static void
on_realize(GtkGLArea *area, gpointer user_data)
{
  gl_view_state_t *state;
  GtkAllocation alloc;
  gl_renderable_t r;

  state = (gl_view_state_t *)user_data;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error\n");
    return;
  }

  /* Build renderables array */
  state->renderables = g_array_sized_new(FALSE, TRUE,
      sizeof(gl_renderable_t), 4);

  /* Create scene renderable — fatal on failure */
  r = gl_view_scene_renderable_new(state);
  if( !r.render )
    return;
  g_array_append_val(state->renderables, r);

  /* Create overlay renderable if configured */
  r = gl_view_overlay_renderable_new(state);
  if( r.render )
    g_array_append_val(state->renderables, r);

  /* Create axes renderer */
  {
    opengl_axes_t *axes;

    axes = opengl_axes_new();
    if( !axes )
    {
      pr_err("Failed to create axes renderer\n");
      return;
    }

    r = (gl_renderable_t){
      .render               = opengl_axes_render,
      .prepare              = opengl_axes_prepare,
      .destroy              = opengl_axes_free,
      .is_active            = opengl_axes_is_active,
      .far_extent           = opengl_axes_far_extent,
      .ctx                  = axes,
      .alpha                = 1.0f,
      .origin               = {0.0f, 0.0f, 0.0f},
      .transparent_sort_order = 0
    };
    g_array_append_val(state->renderables, r);
  }

  /* Create ground plane renderer — only when the scene provides an
   * is_active predicate, since visibility is domain-specific */
  if( state->scene->ground_plane_is_active )
  {
    opengl_ground_plane_t *ground_plane;

    ground_plane = opengl_ground_plane_new();
    if( !ground_plane )
    {
      pr_err("Failed to create ground plane renderer\n");
      return;
    }

    r = (gl_renderable_t){
      .render               = opengl_ground_plane_render,
      .prepare              = opengl_ground_plane_prepare,
      .destroy              = opengl_ground_plane_free,
      .is_active            = state->scene->ground_plane_is_active,
      .far_extent           = opengl_ground_plane_far_extent,
      .ctx                  = ground_plane,
      .alpha                = 0.5f,
      .origin               = {0.0f, 0.0f, 0.0f},
      .transparent_sort_order = 2,
      .transparent_on_drag  = FALSE
    };
    g_array_append_val(state->renderables, r);
  }

  state->initialized = TRUE;

  /* Gradient overlay (2D HUD, not a renderable) — optional */
  if( state->config->has_gradient )
  {
    state->overlay = gradient_overlay_new(state->config->gradient_draw);
    if( !state->overlay )
      pr_err("Failed to create gradient overlay\n");
    else
    {
      gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
      gradient_overlay_set_viewport(state->overlay, alloc.width, alloc.height);
    }
  }

  /* Initialize MSAA resources after GL context is ready */
  gl_view_recreate_msaa(state, rc_config.opengl_msaa_samples);

} /* on_realize() */

/*-----------------------------------------------------------------------*/

/* on_unrealize()
 *
 * GtkGLArea unrealize signal handler
 */
  static void
on_unrealize(GtkGLArea *area, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  gtk_gl_area_make_current(area);

  gl_view_state_free(state);

} /* on_unrealize() */

/*-----------------------------------------------------------------------*/

/* on_resize()
 *
 * GtkGLArea resize signal handler
 */
  static void
on_resize(GtkGLArea *area, int width, int height, gpointer user_data)
{
  gl_view_state_t *state;
  float aspect;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return;

  aspect = (float)width / (float)height;

  state->aspect = aspect;
  state->viewport_height = (float)height;

  /* Store dimensions for MSAA recreation */
  state->msaa_width = width;
  state->msaa_height = height;

  /* Recreate MSAA FBO at new dimensions */
  if( state->msaa_samples > 0 )
    gl_view_recreate_msaa(state, state->msaa_samples);

  if( state->overlay )
    gradient_overlay_set_viewport(state->overlay, width, height);

  glViewport(0, 0, width, height);

} /* on_resize() */

/*-----------------------------------------------------------------------*/

/* gl_view_create_widget()
 *
 * Create GL area widget with engine wired
 */
  GtkWidget*
gl_view_create_widget(
    gl_view_config_t *config,
    gl_scene_provider_t *scene,
    arcball_state_t *arcball,
    GtkSpinButton **zoom_spinbutton)
{
  GtkWidget *gl_area;
  gl_view_state_t *state;

  if( !config || !scene || !arcball )
    return( NULL );

  state = g_new0(gl_view_state_t, 1);

  state->config = config;
  state->scene = scene;
  state->arcball = arcball;
  state->zoom_spinbutton = zoom_spinbutton;
  state->last_generation = (unsigned int)-1;
  state->fov_rad = glm_rad(60.0f);
  state->aspect = 1.0f;
  state->viewport_height = 1.0f;

  /* Initialize per-view pan state */
  glm_vec2_zero(state->pan_offset);
  state->cached_camera_distance = 1.0f;

  /* Initialize overlay scale adjustment (1.0 = default from overlay_generate) */
  state->ovl_model_scale_adj = 1.0f;

  /* Drag transparency: halve alpha for marked renderables while dragging */
  state->drag_alpha_factor = DRAG_ALPHA_FACTOR;

  /* Initialize tooltip state */
  state->tooltip_active = FALSE;
  state->tooltip_text = NULL;
  state->tooltip_alpha = 0.0;
  state->tooltip_start_time = 0;
  state->tooltip_timeout_id = 0;

  gl_area = gtk_gl_area_new();

  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), TRUE);
  gtk_gl_area_set_auto_render(GTK_GL_AREA(gl_area), TRUE);

  gtk_widget_set_size_request(gl_area, 400, 400);
  gtk_widget_set_hexpand(gl_area, TRUE);
  gtk_widget_set_vexpand(gl_area, TRUE);

  g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize), state);
  g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_unrealize), state);
  gl_view_render_connect(gl_area, state);
  g_signal_connect(gl_area, "resize", G_CALLBACK(on_resize), state);

  gl_view_input_connect(gl_area, state);

  g_object_set_data(G_OBJECT(gl_area), "gl_state", state);

  return( gl_area );

} /* gl_view_create_widget() */

/*-----------------------------------------------------------------------*/

/* gl_view_get_state()
 *
 * Get view state from widget
 */
  gl_view_state_t*
gl_view_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* gl_view_get_state() */

/*-----------------------------------------------------------------------*/

/* gl_view_set_arcball()
 *
 * Set the arcball reference for a view
 */
  void
gl_view_set_arcball(GtkWidget *widget, arcball_state_t *arcball)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state || !arcball )
    return;

  state->arcball = arcball;

} /* gl_view_set_arcball() */

/*-----------------------------------------------------------------------*/

/* gl_view_sync_arcball()
 *
 * Sync arcball rotation from angles
 */
  void
gl_view_sync_arcball(GtkWidget *widget, double wr, double wi)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state || !state->arcball )
    return;

  arcball_set_view(state->arcball, (float)wr, (float)wi);
  arcball_notify_changed(state->arcball);

} /* gl_view_sync_arcball() */

/*-----------------------------------------------------------------------*/

/* gl_view_reset_pan()
 *
 * Reset pan offset to center the view
 */
  void
gl_view_reset_pan(GtkWidget *widget)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state )
    return;

  glm_vec2_zero(state->pan_offset);

} /* gl_view_reset_pan() */

#endif /* HAVE_OPENGL */
