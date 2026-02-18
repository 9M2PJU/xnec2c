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
#include "opengl_axes.h"
#include "opengl_cairo_overlay.h"
#include "opengl_ground_plane.h"
#include "shared.h"

#ifdef HAVE_OPENGL

/* Scene rendering context — owns shader and GL resources for primary geometry */
typedef struct
{
  gl_view_state_t *view;
  gl_shader_t shader;
  GLuint vao;
  GLuint vbo;
  GLint mvp_location;
  GLint u_alpha_location;
  GLint *attrib_locations;

} gl_scene_ctx_t;

/* Overlay rendering context — owns shader, GL resources, and cached MVP */
typedef struct
{
  gl_view_state_t *view;
  gl_shader_t shader;
  GLuint vao;
  GLuint vbo;
  GLint mvp_location;
  GLint u_alpha_location;
  GLint *attrib_locations;
  unsigned int last_generation;
  gboolean initialized;
  mat4 cached_mvp;
  gl_view_content_t ovl_content;

} gl_overlay_ctx_t;

/* Scene renderable callbacks */
static void gl_scene_prepare(void *ctx, float r_max);
static void gl_scene_render(void *ctx, mat4 mvp, float alpha);
static gboolean gl_scene_is_active(void *ctx);
static float gl_scene_far_extent(void *ctx, float r_max);
static void gl_scene_free(void *ctx);

/* Overlay renderable callbacks */
static void gl_overlay_prepare(void *ctx, float r_max);
static void gl_overlay_render(void *ctx, mat4 mvp, float alpha);
static gboolean gl_overlay_is_active(void *ctx);
static float gl_overlay_far_extent(void *ctx, float r_max);
static void gl_overlay_free(void *ctx);

/*-----------------------------------------------------------------------*/

/* gl_view_recreate_msaa()
 *
 * Recreate MSAA resources without destroying GL widget
 */
  static void
gl_view_recreate_msaa(gl_view_state_t *state, int requested_samples)
{
  GLint max_samples;
  int width, height;

  if( !state || !state->initialized )
    return;

  /* Delete existing MSAA resources */
  if( state->msaa_fbo )
  {
    glDeleteFramebuffers(1, &state->msaa_fbo);
    state->msaa_fbo = 0;
  }

  if( state->msaa_color_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    state->msaa_color_rbo = 0;
  }

  if( state->msaa_depth_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_depth_rbo = 0;
  }

  state->msaa_samples = 0;

  if( requested_samples == 0 )
    return;

  /* Query hardware limit and clamp */
  glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
  state->msaa_samples = (requested_samples > max_samples) ? max_samples : requested_samples;

  if( state->msaa_samples < 2 )
  {
    state->msaa_samples = 0;
    return;
  }

  /* Get current viewport dimensions */
  width = state->msaa_width;
  height = state->msaa_height;

  if( width == 0 || height == 0 )
    return;

  /* Create multisampled color renderbuffer */
  glGenRenderbuffers(1, &state->msaa_color_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, state->msaa_color_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, state->msaa_samples,
      GL_RGBA8, width, height);

  /* Create multisampled depth renderbuffer */
  glGenRenderbuffers(1, &state->msaa_depth_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, state->msaa_depth_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, state->msaa_samples,
      GL_DEPTH_COMPONENT24, width, height);

  /* Create FBO and attach renderbuffers */
  glGenFramebuffers(1, &state->msaa_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, state->msaa_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER, state->msaa_color_rbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
      GL_RENDERBUFFER, state->msaa_depth_rbo);

  if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
  {
    pr_err("MSAA framebuffer incomplete\n");
    glDeleteFramebuffers(1, &state->msaa_fbo);
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_fbo = 0;
    state->msaa_color_rbo = 0;
    state->msaa_depth_rbo = 0;
    state->msaa_samples = 0;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

} /* gl_view_recreate_msaa() */

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

  g_free(state->saved_alphas);

  if( state->overlay )
    gradient_overlay_free(state->overlay);

  /* Free MSAA resources */
  if( state->msaa_fbo )
  {
    glDeleteFramebuffers(1, &state->msaa_fbo);
    state->msaa_fbo = 0;
  }

  if( state->msaa_color_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    state->msaa_color_rbo = 0;
  }

  if( state->msaa_depth_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_depth_rbo = 0;
  }

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
  gboolean ok;
  int i;

  state = (gl_view_state_t *)user_data;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error\n");
    return;
  }

  /* Create scene rendering context */
  {
    gl_scene_ctx_t *sc;

    sc = g_new0(gl_scene_ctx_t, 1);
    sc->view = state;

    ok = gl_shader_load(&sc->shader,
        state->config->vertex_shader_path,
        state->config->fragment_shader_path);

    if( !ok )
    {
      pr_err("Failed to load shaders\n");
      g_free(sc);
      return;
    }

    sc->mvp_location = glGetUniformLocation(sc->shader.program, "mvp");
    sc->u_alpha_location = glGetUniformLocation(sc->shader.program, "u_alpha");

    glGenVertexArrays(1, &sc->vao);
    glGenBuffers(1, &sc->vbo);

    sc->attrib_locations = g_new(GLint, state->config->attrib_count);

    for( i = 0; i < state->config->attrib_count; i++ )
    {
      sc->attrib_locations[i] = glGetAttribLocation(
          sc->shader.program,
          state->config->attribs[i].name);
    }

    /* Build renderables array */
    state->renderables = g_array_sized_new(FALSE, TRUE,
        sizeof(gl_renderable_t), 4);

    r = (gl_renderable_t){
      .render               = gl_scene_render,
      .prepare              = gl_scene_prepare,
      .destroy              = gl_scene_free,
      .is_active            = gl_scene_is_active,
      .far_extent           = gl_scene_far_extent,
      .ctx                  = sc,
      .alpha                = 1.0f,
      .origin               = {0.0f, 0.0f, 0.0f},
      .transparent_sort_order = 1
    };
    g_array_append_val(state->renderables, r);
  }

  /* Create overlay rendering context if configured */
  if( state->scene->overlay_config )
  {
    const gl_overlay_config_t *ocfg = state->scene->overlay_config;
    gl_overlay_ctx_t *ovl;

    ovl = g_new0(gl_overlay_ctx_t, 1);
    ovl->view = state;

    ok = gl_shader_load(&ovl->shader,
        ocfg->vertex_shader_path,
        ocfg->fragment_shader_path);

    if( ok )
    {
      ovl->mvp_location =
        glGetUniformLocation(ovl->shader.program, "mvp");
      ovl->u_alpha_location =
        glGetUniformLocation(ovl->shader.program, "u_alpha");

      glGenVertexArrays(1, &ovl->vao);
      glGenBuffers(1, &ovl->vbo);

      ovl->attrib_locations = g_new(GLint, ocfg->attrib_count);
      for( i = 0; i < ocfg->attrib_count; i++ )
      {
        ovl->attrib_locations[i] = glGetAttribLocation(
            ovl->shader.program,
            ocfg->attribs[i].name);
      }

      ovl->last_generation = (unsigned int)-1;
      ovl->initialized = TRUE;

      r = (gl_renderable_t){
        .render               = gl_overlay_render,
        .prepare              = gl_overlay_prepare,
        .destroy              = gl_overlay_free,
        .is_active            = gl_overlay_is_active,
        .far_extent           = gl_overlay_far_extent,
        .ctx                  = ovl,
        .alpha                = 1.0f,
        .origin               = {0.0f, 0.0f, 0.0f},
        .transparent_sort_order = 0
      };
      g_array_append_val(state->renderables, r);
    }
    else
    {
      pr_err("Failed to load overlay shaders\n");
      g_free(ovl);
    }
  }

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

  /* Create ground plane renderer */
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
      .is_active            = opengl_ground_plane_is_active,
      .far_extent           = opengl_ground_plane_far_extent,
      .ctx                  = ground_plane,
      .alpha                = 0.5f,
      .origin               = {0.0f, 0.0f, 0.0f},
      .transparent_sort_order = 2
    };
    g_array_append_val(state->renderables, r);
  }

  /* Gradient overlay (2D HUD, not a renderable) */
  if( state->config->has_gradient )
  {
    state->overlay = gradient_overlay_new(state->config->gradient_draw);
    if( !state->overlay )
    {
      pr_err("Failed to create gradient overlay\n");
      return;
    }

    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    gradient_overlay_set_viewport(state->overlay, alloc.width, alloc.height);
  }

  /* Allocate drag save/restore array sized to renderable count */
  state->saved_alphas = g_new0(float, state->renderables->len);

  state->initialized = TRUE;

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

/* gl_view_setup_attribs()
 *
 * Configure vertex attribute pointers in VAO. Called once during prepare
 * when VBO data changes. VAO retains this state for subsequent renders.
 */
  static void
gl_view_setup_attribs(
    GLuint vao,
    GLuint vbo,
    const gl_vertex_attrib_t *attribs,
    const GLint *attrib_locations,
    int attrib_count,
    int vertex_stride)
{
  int i;

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  for( i = 0; i < attrib_count; i++ )
  {
    glEnableVertexAttribArray(attrib_locations[i]);
    glVertexAttribPointer(
        attrib_locations[i],
        attribs[i].components,
        GL_FLOAT,
        GL_FALSE,
        vertex_stride,
        (void *)(long)attribs[i].offset);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

} /* gl_view_setup_attribs() */

/*-----------------------------------------------------------------------*/

/* gl_view_draw_pass()
 *
 * Execute a rendering pass. VAO already has attrib config from prepare.
 */
  static void
gl_view_draw_pass(
    GLuint shader_program,
    GLint mvp_location,
    mat4 mvp,
    GLuint vao,
    GLenum draw_mode,
    int vertex_count)
{
  glUseProgram(shader_program);
  glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (float *)mvp);

  glBindVertexArray(vao);
  glDrawArrays(draw_mode, 0, vertex_count);
  glBindVertexArray(0);

} /* gl_view_draw_pass() */

/*-----------------------------------------------------------------------*/

/* gl_scene_prepare()
 *
 * Upload scene vertex data on generation change
 */
  static void
gl_scene_prepare(void *ctx, float r_max)
{
  gl_scene_ctx_t *sc = ctx;
  gl_view_state_t *view = sc->view;
  gl_view_content_t *c = &view->content;

  (void)r_max;

  if( c->generation == view->last_generation )
    return;

  if( c->vertex_count > 0 )
  {
    glBindBuffer(GL_ARRAY_BUFFER, sc->vbo);
    glBufferData(GL_ARRAY_BUFFER,
        c->vertex_count * c->vertex_stride,
        c->vertices,
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Reconfigure VAO attrib pointers for new VBO layout */
    gl_view_setup_attribs(sc->vao, sc->vbo,
        view->config->attribs, sc->attrib_locations,
        view->config->attrib_count, c->vertex_stride);
  }

  if( view->overlay )
    gradient_overlay_mark_dirty(view->overlay);

  view->last_generation = c->generation;

} /* gl_scene_prepare() */

/*-----------------------------------------------------------------------*/

/* gl_scene_render()
 *
 * Render scene geometry using generic draw pass
 */
  static void
gl_scene_render(void *ctx, mat4 mvp, float alpha)
{
  gl_scene_ctx_t *sc = ctx;
  gl_view_state_t *view = sc->view;

  /* Set alpha multiplier before draw pass */
  glUseProgram(sc->shader.program);
  glUniform1f(sc->u_alpha_location, alpha);

  gl_view_draw_pass(
      sc->shader.program,
      sc->mvp_location,
      mvp,
      sc->vao,
      view->content.draw_mode,
      view->content.vertex_count);

} /* gl_scene_render() */

/*-----------------------------------------------------------------------*/

/* gl_scene_is_active()
 *
 * Returns TRUE when scene has vertex data to render
 */
  static gboolean
gl_scene_is_active(void *ctx)
{
  gl_scene_ctx_t *sc = ctx;

  return( sc->view->content.vertex_count > 0 );

} /* gl_scene_is_active() */

/*-----------------------------------------------------------------------*/

/* gl_scene_far_extent()
 *
 * Returns the scene geometry extent for clip plane calculation
 */
  static float
gl_scene_far_extent(void *ctx, float r_max)
{
  (void)ctx;

  return( r_max );

} /* gl_scene_far_extent() */

/*-----------------------------------------------------------------------*/

/* gl_scene_free()
 *
 * Free scene rendering context and GL resources
 */
  static void
gl_scene_free(void *ctx)
{
  gl_scene_ctx_t *sc = ctx;

  if( !sc )
    return;

  if( sc->view->scene && sc->view->scene->cleanup )
    sc->view->scene->cleanup();

  if( sc->vbo )
    glDeleteBuffers(1, &sc->vbo);

  if( sc->vao )
    glDeleteVertexArrays(1, &sc->vao);

  gl_shader_destroy(&sc->shader);

  g_free(sc->attrib_locations);
  g_free(sc);

} /* gl_scene_free() */

/*-----------------------------------------------------------------------*/

/* gl_overlay_prepare()
 *
 * Generate overlay content, upload to VBO, and cache own MVP
 */
  static void
gl_overlay_prepare(void *ctx, float r_max)
{
  gl_overlay_ctx_t *ovl = ctx;
  gl_view_state_t *view = ovl->view;

  ovl->ovl_content.vertex_count = 0;

  if( !ovl->initialized )
    return;

  if( !view->scene->overlay_generate )
    return;

  if( !view->scene->overlay_generate(&view->content, &ovl->ovl_content) )
    return;

  if( ovl->ovl_content.vertex_count <= 0 )
    return;

  /* Upload overlay vertices on generation change */
  if( ovl->ovl_content.generation != ovl->last_generation )
  {
    glBindBuffer(GL_ARRAY_BUFFER, ovl->vbo);
    glBufferData(GL_ARRAY_BUFFER,
        ovl->ovl_content.vertex_count * ovl->ovl_content.vertex_stride,
        ovl->ovl_content.vertices,
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Reconfigure VAO attrib pointers for new VBO layout */
    {
      const gl_overlay_config_t *ocfg = view->scene->overlay_config;

      gl_view_setup_attribs(ovl->vao, ovl->vbo,
          ocfg->attribs, ovl->attrib_locations,
          ocfg->attrib_count, ovl->ovl_content.vertex_stride);
    }

    ovl->last_generation = ovl->ovl_content.generation;
  }

  /* Compute and cache own MVP with user-adjusted model scale */
  {
    float ovl_model_scale, nearest_point, farthest_point, near_plane, far_plane;

    ovl_model_scale = ovl->ovl_content.model_scale * view->ovl_model_scale_adj;

    nearest_point = view->cached_camera_distance - r_max;
    farthest_point = view->cached_camera_distance + r_max;
    far_plane = farthest_point * 1.2f;

    if( nearest_point > 0.0f )
    {
      near_plane = nearest_point * 0.8f;
    }
    else
    {
      near_plane = 0.001f;
    }

    arcball_get_mvp(view->arcball, ovl->cached_mvp, view->pan_offset,
        view->cached_camera_distance, ovl_model_scale,
        view->aspect, view->fov_rad, near_plane, far_plane);
  }

} /* gl_overlay_prepare() */

/*-----------------------------------------------------------------------*/

/* gl_overlay_render()
 *
 * Render overlay using cached MVP (ignores passed MVP)
 */
  static void
gl_overlay_render(void *ctx, mat4 mvp, float alpha)
{
  gl_overlay_ctx_t *ovl = ctx;

  (void)mvp;

  if( ovl->ovl_content.vertex_count <= 0 )
    return;

  /* Set alpha multiplier before draw pass */
  glUseProgram(ovl->shader.program);
  glUniform1f(ovl->u_alpha_location, alpha);

  gl_view_draw_pass(
      ovl->shader.program,
      ovl->mvp_location,
      ovl->cached_mvp,
      ovl->vao,
      ovl->ovl_content.draw_mode,
      ovl->ovl_content.vertex_count);

} /* gl_overlay_render() */

/*-----------------------------------------------------------------------*/

/* gl_overlay_is_active()
 *
 * Returns TRUE when overlay is initialized and provider exists
 */
  static gboolean
gl_overlay_is_active(void *ctx)
{
  gl_overlay_ctx_t *ovl = ctx;

  return( ovl->initialized &&
          ovl->view->scene->overlay_generate != NULL );

} /* gl_overlay_is_active() */

/*-----------------------------------------------------------------------*/

/* gl_overlay_far_extent()
 *
 * Returns the overlay extent for clip plane calculation
 */
  static float
gl_overlay_far_extent(void *ctx, float r_max)
{
  (void)ctx;

  return( r_max );

} /* gl_overlay_far_extent() */

/*-----------------------------------------------------------------------*/

/* gl_overlay_free()
 *
 * Free overlay rendering context and GL resources
 */
  static void
gl_overlay_free(void *ctx)
{
  gl_overlay_ctx_t *ovl = ctx;

  if( !ovl )
    return;

  if( ovl->view->scene && ovl->view->scene->overlay_cleanup )
    ovl->view->scene->overlay_cleanup();

  if( ovl->vbo )
    glDeleteBuffers(1, &ovl->vbo);

  if( ovl->vao )
    glDeleteVertexArrays(1, &ovl->vao);

  gl_shader_destroy(&ovl->shader);

  g_free(ovl->attrib_locations);
  g_free(ovl);

} /* gl_overlay_free() */

/*-----------------------------------------------------------------------*/

/* on_render()
 *
 * GtkGLArea render signal handler
 */
  static gboolean
on_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
  gl_view_state_t *state;
  gl_view_content_t content;
  mat4 mvp;
  float camera_distance;
  GLint default_fbo = 0;
  guint32 active_mask;
  guint i;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->initialized )
    return( FALSE );

  /* Data acquisition preamble */
  if( !state->scene->generate(&content) )
    return( FALSE );

  state->content = content;

  camera_distance = content.r_max * ARCBALL_BASE_DISTANCE_FACTOR / content.zoom;
  state->cached_camera_distance = camera_distance;

  /* Active survey — build mask and compute far extent in one pass */
  {
    float effective_far, nearest_point, farthest_point, near_plane, far_plane, ext;

    active_mask = 0;
    effective_far = 0.0f;

    for( i = 0; i < state->renderables->len; i++ )
    {
      gl_renderable_t *r = &g_array_index(
          state->renderables, gl_renderable_t, i);

      if( r->is_active != NULL && !r->is_active(r->ctx) )
        continue;

      active_mask |= (1u << i);

      if( r->far_extent )
      {
        ext = r->far_extent(r->ctx, content.r_max);

        if( ext > effective_far )
          effective_far = ext;
      }
    }

    nearest_point = camera_distance - content.r_max;
    farthest_point = camera_distance + effective_far;

    far_plane = farthest_point * 1.2f;

    if( nearest_point > 0.0f )
    {
      near_plane = nearest_point * 0.8f;
    }
    else
    {
      near_plane = 0.001f;
    }

    arcball_get_mvp(state->arcball, mvp, state->pan_offset,
        camera_distance, content.model_scale, state->aspect, state->fov_rad,
        near_plane, far_plane);
  }

  /* Framebuffer setup */
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &default_fbo);

  if( state->msaa_fbo )
    glBindFramebuffer(GL_FRAMEBUFFER, state->msaa_fbo);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);

  /* Opaque pass — depth buffer fully populated */
  for( i = 0; i < state->renderables->len; i++ )
  {
    gl_renderable_t *r = &g_array_index(
        state->renderables, gl_renderable_t, i);

    if( r->alpha < 1.0f )
      continue;

    if( !(active_mask & (1u << i)) )
      continue;

    r->prepare(r->ctx, content.r_max);
    r->render(r->ctx, mvp, r->alpha);
  }

  /* Transparent pass — sorted by priority then back-to-front depth */
  {
    int trans_indices[state->renderables->len];
    int trans_orders[state->renderables->len];
    float trans_depths[state->renderables->len];
    int trans_count, j, k;

    trans_count = 0;

    for( i = 0; i < state->renderables->len; i++ )
    {
      gl_renderable_t *r = &g_array_index(
          state->renderables, gl_renderable_t, i);

      if( r->alpha >= 1.0f )
        continue;

      if( !(active_mask & (1u << i)) )
        continue;

      trans_orders[trans_count] = r->transparent_sort_order;
      trans_depths[trans_count] =
          state->arcball->rotation[0][2] * r->origin[0] +
          state->arcball->rotation[1][2] * r->origin[1] +
          state->arcball->rotation[2][2] * r->origin[2];
      trans_indices[trans_count] = (int)i;
      trans_count++;
    }

    /* Insertion sort: ascending sort_order, then ascending depth
     * (farthest first within same priority) */
    for( j = 1; j < trans_count; j++ )
    {
      int key_order = trans_orders[j];
      float key_depth = trans_depths[j];
      int key_idx = trans_indices[j];

      k = j - 1;

      while( k >= 0 &&
             (trans_orders[k] > key_order ||
              (trans_orders[k] == key_order &&
               trans_depths[k] > key_depth)) )
      {
        trans_orders[k + 1] = trans_orders[k];
        trans_depths[k + 1] = trans_depths[k];
        trans_indices[k + 1] = trans_indices[k];
        k--;
      }

      trans_orders[k + 1] = key_order;
      trans_depths[k + 1] = key_depth;
      trans_indices[k + 1] = key_idx;
    }

    /* Depth writes ON so transparent shells self-occlude.
     * Interior objects render first (lower sort_order) and
     * write depth; enclosing shells depth-test against them. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for( j = 0; j < trans_count; j++ )
    {
      gl_renderable_t *r = &g_array_index(
          state->renderables, gl_renderable_t, trans_indices[j]);

      r->prepare(r->ctx, content.r_max);
      r->render(r->ctx, mvp, r->alpha);

      /* Re-assert blend in case renderable modified GL state */
      glEnable(GL_BLEND);
    }

    glDisable(GL_BLEND);
  }

  /* Post-3D callbacks */
  if( state->scene->post_render )
    state->scene->post_render();

  /* 2D HUD — screen-space, rendered to MSAA FBO */
  if( state->overlay && content.show_gradient )
    gradient_overlay_render(state->overlay);

  /* Render tooltip if active */
  if( state->tooltip_active && state->tooltip_text )
  {
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_text_extents_t extents;
    double x, y, padding;
    int surf_width, surf_height;

    surf_width = (int)(state->viewport_height * state->aspect);
    surf_height = (int)state->viewport_height;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surf_width, surf_height);
    cr = cairo_create(surface);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16.0);

    cairo_text_extents(cr, state->tooltip_text, &extents);

    padding = 12.0;
    x = (surf_width - extents.width) / 2.0 - extents.x_bearing;
    y = (surf_height - extents.height) / 2.0 - extents.y_bearing;

    /* Background box with fade */
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7 * state->tooltip_alpha);
    cairo_rectangle(cr,
        x + extents.x_bearing - padding,
        y + extents.y_bearing - padding,
        extents.width + 2.0 * padding,
        extents.height + 2.0 * padding);
    cairo_fill(cr);

    /* Text with fade */
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, state->tooltip_alpha);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, state->tooltip_text);

    cairo_destroy(cr);

    /* Lazily allocate cached tooltip overlay */
    if( !state->tooltip_overlay )
      state->tooltip_overlay = cairo_gl_overlay_new();

    if( state->tooltip_overlay )
    {
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      cairo_gl_overlay_set_size(state->tooltip_overlay, surf_width, surf_height);
      cairo_gl_overlay_upload(state->tooltip_overlay, surface);
      cairo_gl_overlay_render(state->tooltip_overlay);

      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
    }

    cairo_surface_destroy(surface);
  }

  /* Blit MSAA to default FBO */
  if( state->msaa_fbo )
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, state->msaa_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, default_fbo);
    glBlitFramebuffer(0, 0, state->msaa_width, state->msaa_height,
                      0, 0, state->msaa_width, state->msaa_height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, default_fbo);
  }

  return( TRUE );

} /* on_render() */

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

/* on_button_press()
 *
 * Mouse button press handler
 */
  static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  if( event->button == 1 || event->button == 2 )
  {
    arcball_begin_drag(state->arcball, event->button, event->x, event->y);

    /* Save current alphas and halve all renderables for drag feedback */
    {
      guint ri;

      for( ri = 0; ri < state->renderables->len; ri++ )
      {
        gl_renderable_t *r = &g_array_index(
            state->renderables, gl_renderable_t, ri);

        state->saved_alphas[ri] = r->alpha;
        r->alpha *= 0.5f;
      }
    }
    gtk_widget_queue_draw(widget);

    return( TRUE );
  }

  return( FALSE );

} /* on_button_press() */

/*-----------------------------------------------------------------------*/

/* on_button_release()
 *
 * Mouse button release handler
 */
  static gboolean
on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  arcball_end_drag(state->arcball);

  /* Restore all alphas from drag transparency */
  {
    guint ri;

    for( ri = 0; ri < state->renderables->len; ri++ )
    {
      gl_renderable_t *r = &g_array_index(
          state->renderables, gl_renderable_t, ri);

      r->alpha = state->saved_alphas[ri];
    }
  }
  gtk_widget_queue_draw(widget);

  return( TRUE );

} /* on_button_release() */

/*-----------------------------------------------------------------------*/

/* on_motion()
 *
 * Mouse motion handler. Handles rotation (button 1) via arcball,
 * and pan (button 2) with proper world-space scaling.
 */
  static gboolean
on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  gl_view_state_t *state;
  float dx, dy;
  float scale;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->arcball )
    return( FALSE );

  if( state->arcball->drag_button == 0 )
    return( FALSE );

  if( state->arcball->drag_button == 1 )
  {
    /* Rotation handled by arcball */
    arcball_drag(state->arcball, event->x, event->y, state->viewport_height);
  }
  else if( state->arcball->drag_button == 2 )
  {
    /* Pan: compute pixel delta */
    dx = event->x - state->arcball->last_x;
    dy = event->y - state->arcball->last_y;

    /* Convert pixel delta to world-space delta at model plane.
     * Formula: 2 * distance * tan(fov/2) / viewport_height
     * Pan translation is post-scale in MVP chain, so model_scale omitted.
     * Aspect correction handled by projection matrix, not world-space conversion. */
    scale = 2.0f * state->cached_camera_distance *
            tanf(state->fov_rad / 2.0f) /
            state->viewport_height;

    state->pan_offset[0] += dx * scale;
    state->pan_offset[1] -= dy * scale;

    /* Update arcball last position for next delta */
    state->arcball->last_x = event->x;
    state->arcball->last_y = event->y;

    /* Notify arcball callbacks to trigger cross-view redraw */
    arcball_notify_changed(state->arcball);
  }
  else
  {
    return( FALSE );
  }

  gtk_widget_queue_draw(widget);

  return( TRUE );

} /* on_motion() */

/*-----------------------------------------------------------------------*/

/* on_scroll()
 *
 * Mouse scroll handler.
 * Shift+scroll invokes scene-specific handler if provided.
 * Normal scroll adjusts primary zoom via spinbutton.
 */
  static gboolean
on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
  gl_view_state_t *state;
  GtkSpinButton *spinbutton;
  double value, scale, zoom_percent;
  int viewport_width, viewport_height;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  /* Shift+scroll: invoke scene-specific handler */
  if( (event->state & GDK_SHIFT_MASK) && state->scene->on_shift_scroll )
  {
    return( state->scene->on_shift_scroll(widget, event, state) );
  }

  /* Normal scroll: adjust primary zoom */
  if( !state->zoom_spinbutton || !*state->zoom_spinbutton )
    return( FALSE );

  spinbutton = *state->zoom_spinbutton;
  value = gtk_spin_button_get_value(spinbutton);

  viewport_width = (int)(state->viewport_height * state->aspect);
  viewport_height = (int)state->viewport_height;
  zoom_percent = value;

  scale = compute_zoom_scale(viewport_width, viewport_height, zoom_percent);

  if( event->direction == GDK_SCROLL_UP )
    value *= (1.0 + 0.1 * scale);
  else if( event->direction == GDK_SCROLL_DOWN )
    value /= (1.0 + 0.1 * scale);
  else
    return( FALSE );

  gtk_spin_button_set_value(spinbutton, value);

  return( TRUE );

} /* on_scroll() */

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

  /* saved_alphas allocated after renderables are built in on_realize */

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
  g_signal_connect(gl_area, "render", G_CALLBACK(on_render), state);
  g_signal_connect(gl_area, "resize", G_CALLBACK(on_resize), state);

  gtk_widget_add_events(gl_area,
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
    GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);

  g_signal_connect(gl_area, "button-press-event",
    G_CALLBACK(on_button_press), state);
  g_signal_connect(gl_area, "button-release-event",
    G_CALLBACK(on_button_release), state);
  g_signal_connect(gl_area, "motion-notify-event",
    G_CALLBACK(on_motion), state);
  g_signal_connect(gl_area, "scroll-event",
    G_CALLBACK(on_scroll), state);

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

/*-----------------------------------------------------------------------*/

/* tooltip_update_callback()
 *
 * Timer callback for tooltip fade animation.
 * Tooltip stays visible for tooltip_hold_ms, then fades over 500ms.
 */
  static gboolean
tooltip_update_callback(gpointer user_data)
{
  GtkWidget *widget;
  gl_view_state_t *state;
  gint64 current_time, elapsed_ms;
  double progress;

  widget = GTK_WIDGET(user_data);
  state = gl_view_get_state(widget);

  if( !state || !state->tooltip_active )
    return( FALSE );

  current_time = g_get_monotonic_time();
  elapsed_ms = (current_time - state->tooltip_start_time) / 1000;

  /* Stay visible for configured hold duration */
  if( elapsed_ms < state->tooltip_hold_ms )
  {
    state->tooltip_alpha = 1.0;
    gtk_widget_queue_draw(widget);
    return( TRUE );
  }

  /* Fade over 500ms after hold period */
  progress = (double)(elapsed_ms - state->tooltip_hold_ms) / 500.0;

  if( progress >= 1.0 )
  {
    state->tooltip_active = FALSE;
    state->tooltip_alpha = 0.0;
    state->tooltip_timeout_id = 0;
    gtk_widget_queue_draw(widget);
    return( FALSE );
  }

  /* Linear fade from 1.0 to 0.0 */
  state->tooltip_alpha = 1.0 - progress;

  gtk_widget_queue_draw(widget);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* gl_view_show_tooltip()
 *
 * Display a tooltip message that holds for duration_ms then fades over 500ms
 */
  void
gl_view_show_tooltip(GtkWidget *widget, const char *text, int duration_ms)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state || !text )
    return;

  /* Remove existing tooltip timer */
  if( state->tooltip_timeout_id )
  {
    g_source_remove(state->tooltip_timeout_id);
    state->tooltip_timeout_id = 0;
  }

  g_free(state->tooltip_text);
  state->tooltip_text = g_strdup(text);
  state->tooltip_active = TRUE;
  state->tooltip_alpha = 1.0;
  state->tooltip_hold_ms = duration_ms;
  state->tooltip_start_time = g_get_monotonic_time();

  /* Timer fires every 16ms (60fps) for smooth fade */
  state->tooltip_timeout_id = g_timeout_add(16, tooltip_update_callback, widget);

  gtk_widget_queue_draw(widget);

} /* gl_view_show_tooltip() */

/*-----------------------------------------------------------------------*/

/* Set_MSAA_Samples()
 *
 * Change MSAA sample count for all GL views
 */
  void
Set_MSAA_Samples(int samples)
{
  static char *msaa_widget_names[] = {
    "main_opengl_msaa_off",
    NULL,
    "main_opengl_msaa_2x",
    NULL,
    "main_opengl_msaa_4x",
    NULL, NULL, NULL,
    "main_opengl_msaa_8x",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "main_opengl_msaa_16x"
  };

  GtkWidget *widget;
  gl_view_state_t *state;

  rc_config.opengl_msaa_samples = samples;

  /* Update menu selection */
  if( samples <= 16 && msaa_widget_names[samples] )
  {
    widget = Builder_Get_Object(main_window_builder, msaa_widget_names[samples]);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
  }

  /* Recreate MSAA resources for structure view */
  if( structure_gl_area )
  {
    state = gl_view_get_state(structure_gl_area);
    if( state )
    {
      gtk_gl_area_make_current(GTK_GL_AREA(structure_gl_area));
      gl_view_recreate_msaa(state, samples);
      gtk_widget_queue_draw(structure_gl_area);
    }
  }

  /* Recreate MSAA resources for radiation pattern view */
  if( rdpattern_gl_area )
  {
    state = gl_view_get_state(rdpattern_gl_area);
    if( state )
    {
      gtk_gl_area_make_current(GTK_GL_AREA(rdpattern_gl_area));
      gl_view_recreate_msaa(state, samples);
      gtk_widget_queue_draw(rdpattern_gl_area);
    }
  }

} /* Set_MSAA_Samples() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
