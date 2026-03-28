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
#include "opengl_view_peel.h"
#include "opengl_view_notice.h"
#include "opengl_axes.h"
#include "opengl_cairo_overlay.h"
#include "opengl_ground_plane.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/** gl_view_state_free() - Free view state resources
 * @state: view state
 */
  static void
gl_view_state_free(gl_view_state_t *state)
{
  if( !state )
    return;

  if( state->notice_timeout_id )
  {
    g_source_remove(state->notice_timeout_id);
    state->notice_timeout_id = 0;
  }

  g_free(state->notice_text);

  if( state->notice_surface )
    cairo_surface_destroy(state->notice_surface);

  if( state->notice_overlay )
    cairo_gl_overlay_free(state->notice_overlay);

  /* Release shared noise texture before renderables */
  if( state->noise_tex )
  {
    glDeleteTextures(1, &state->noise_tex);
    state->noise_tex = 0;
  }

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

  /* Release depth-peel FBO resources (per-resize lifecycle) */
  gl_view_peel_free(state);

  /* Release composite resources (per-realize lifecycle) */
  GL_DELETE(glDeleteBuffers, state->composite_vbo);
  GL_DELETE(glDeleteVertexArrays, state->composite_vao);
  GL_DELETE_OBJ(glDeleteProgram, state->composite_program);
  GL_DELETE_OBJ(glDeleteShader, state->composite_vs);
  GL_DELETE_OBJ(glDeleteShader, state->composite_fs);

  gl_view_msaa_free(state);

  g_free(state);

} /* gl_view_state_free() */

/*-----------------------------------------------------------------------*/

/** on_realize() - GtkGLArea realize signal handler
 * @area: GL area widget
 * @user_data: view state
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
      .get_alpha            = opengl_axes_get_alpha,
      .origin               = {0.0f, 0.0f, 0.0f},
      .transparent_sort_order = 0
    };

    /* Scene provider may supply a domain-specific axes predicate */
    if( state->scene->axes_is_active )
      r.is_active = state->scene->axes_is_active;

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
      .get_alpha            = opengl_ground_plane_get_alpha,
      .origin               = {0.0f, 0.0f, 0.0f},
      .transparent_sort_order = 2,
      .transparent_on_drag  = FALSE
    };
    g_array_append_val(state->renderables, r);
  }

  if( state->renderables->len > MAX_RENDERABLES )
  {
    pr_err("Renderable count %u exceeds MAX_RENDERABLES (%d)\n",
        state->renderables->len, MAX_RENDERABLES);
    return;
  }

  state->initialized = TRUE;

  /* Generate LIC noise texture (256x256 grayscale, shared by all renderables).
   * Created here in view state rather than in a single renderable so that
   * both scene and overlay can reference it without ownership ambiguity. */
  {
    enum { NOISE_SIZE = 256 };
    unsigned char noise_data[NOISE_SIZE * NOISE_SIZE];
    unsigned int rng = 42;
    int ni;

    for( ni = 0; ni < NOISE_SIZE * NOISE_SIZE; ni++ )
    {
      rng ^= rng << 13;
      rng ^= rng >> 17;
      rng ^= rng << 5;
      noise_data[ni] = (unsigned char)(rng & 0xFF);
    }

    glGenTextures(1, &state->noise_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, state->noise_tex);

    /* GL_RED for core profile compatibility (GL_LUMINANCE is
     * deprecated in 3.x+ core contexts created by GtkGLArea) */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, NOISE_SIZE, NOISE_SIZE, 0,
        GL_RED, GL_UNSIGNED_BYTE, noise_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0);
  }

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

  /* Compile depth-peel composite shader and create fullscreen triangle */
  {
    gl_shader_t cs = {0};
    gboolean ok;

    ok = gl_shader_load(&cs,
        "/gl/peel-composite-vertex.glsl",
        "/gl/peel-composite-fragment.glsl");

    if( ok )
    {
      static const float tri_verts[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
      };
      GLint pos_loc;

      state->composite_program = cs.program;
      state->composite_vs = cs.vertex;
      state->composite_fs = cs.fragment;
      state->composite_u_layer =
        glGetUniformLocation(cs.program, "u_layer");

      pos_loc = glGetAttribLocation(cs.program, "position");

      glGenBuffers(1, &state->composite_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, state->composite_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(tri_verts),
          tri_verts, GL_STATIC_DRAW);

      glGenVertexArrays(1, &state->composite_vao);
      glBindVertexArray(state->composite_vao);
      glEnableVertexAttribArray(pos_loc);
      glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else
    {
      pr_err("Failed to load peel composite shaders\n");
    }
  }

} /* on_realize() */

/*-----------------------------------------------------------------------*/

/** on_unrealize() - GtkGLArea unrealize signal handler
 * @area: GL area widget
 * @user_data: view state
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

/** on_resize() - GtkGLArea resize signal handler
 * @area: GL area widget
 * @width: new width in pixels
 * @height: new height in pixels
 * @user_data: view state
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

  /* Dimensions unchanged — skip FBO resize.
   * Update aspect/viewport in case GTK fires redundantly. */
  if( width == state->msaa_width && height == state->msaa_height )
  {
    glViewport(0, 0, width, height);
    gtk_widget_queue_draw(GTK_WIDGET(area));
    return;
  }

  /* Store dimensions for MSAA recreation */
  state->msaa_width = width;
  state->msaa_height = height;

  /* Recreate MSAA FBO at new dimensions */
  if( rc_config.opengl_msaa_samples > 0 )
    gl_view_recreate_msaa(state, rc_config.opengl_msaa_samples);

  /* Recreate depth-peel FBOs at new dimensions (only when
   * composite shader loaded successfully during realize) */
  if( state->initialized && state->composite_program )
    gl_view_peel_recreate(state, width, height, state->msaa_samples);

  if( state->overlay )
    gradient_overlay_set_viewport(state->overlay, width, height);

  glViewport(0, 0, width, height);

  /* Force redraw so the window does not remain black after resize */
  gtk_widget_queue_draw(GTK_WIDGET(area));

} /* on_resize() */

/*-----------------------------------------------------------------------*/

/** gl_view_create_widget() - Create GL area widget with engine wired
 * @config: view configuration
 * @scene: scene provider
 * @arcball: arcball state
 * @zoom_spinbutton: pointer to zoom spinbutton pointer
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

  /* Initialize notice state */
  state->notice_active = FALSE;
  state->notice_text = NULL;
  state->notice_alpha = 0.0;
  state->notice_start_time = 0;
  state->notice_timeout_id = 0;

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

/** gl_view_get_state() - Get view state from widget
 * @widget: GL area widget
 */
  gl_view_state_t*
gl_view_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* gl_view_get_state() */

/*-----------------------------------------------------------------------*/

/** gl_view_set_arcball() - Set the arcball reference for a view
 * @widget: GL area widget
 * @arcball: arcball state
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

/** gl_view_sync_arcball() - Sync arcball rotation from angles
 * @widget: GL area widget
 * @wr: rotation angle (real component)
 * @wi: rotation angle (imaginary component)
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

/** gl_view_reset_pan() - Reset pan offset to center the view
 * @widget: GL area widget
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
