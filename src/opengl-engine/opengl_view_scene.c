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

#include "opengl_view_scene.h"
#include "opengl_view_peel.h"
#include "opengl_gradient_overlay.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/* Scene rendering context — owns shader and GL resources for primary geometry */
typedef struct
{
  gl_view_state_t *view;
  gl_shader_t shader;
  GLuint vao[GL_VIEW_MAX_BATCHES];
  GLuint vbo[GL_VIEW_MAX_BATCHES];
  GLint mvp_location;
  GLint u_mv_location;
  GLint u_alpha_location;
  GLint u_color_dim_location;
  GLint flow_mode_location;
  GLint u_phase_location;
  GLint u_cos_phase_location;
  GLint u_sin_phase_location;
  GLint noise_tex_location;
  gl_peel_uniform_locs_t peel_locs;
  GLint *attrib_locations;
  GLint depth_bias_attrib_loc;

} gl_scene_ctx_t;

/* Forward declarations for callbacks */
static void gl_scene_prepare(void *ctx, float r_max);
static void gl_scene_render(void *ctx, const gl_render_params_t *params);
static gboolean gl_scene_is_active(void *ctx);
static float gl_scene_far_extent(void *ctx, float r_max);
static void gl_scene_free(void *ctx);

/*-----------------------------------------------------------------------*/

/** gl_scene_prepare() - Upload scene vertex data on generation change
 * @ctx: scene context
 * @r_max: unused
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

  /* Upload each batch to its own VBO and configure its VAO */
  {
    int i;

    for( i = 0; i < c->batch_count; i++ )
    {
      if( c->batches[i].vertex_count > 0 )
      {
        glBindBuffer(GL_ARRAY_BUFFER, sc->vbo[i]);
        glBufferData(GL_ARRAY_BUFFER,
            c->batches[i].vertex_count * c->vertex_stride,
            c->batches[i].vertices,
            GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        gl_view_setup_attribs(sc->vao[i], sc->vbo[i],
            view->config->attribs, sc->attrib_locations,
            view->config->attrib_count, c->vertex_stride);
      }
    }
  }

  if( view->overlay )
    gradient_overlay_mark_dirty(view->overlay);

  view->last_generation = c->generation;

} /* gl_scene_prepare() */

/*-----------------------------------------------------------------------*/

/** gl_scene_render() - Render scene geometry using generic draw pass
 * @ctx: scene context
 * @params: per-frame render parameters
 */
  static void
gl_scene_render(void *ctx, const gl_render_params_t *params)
{
  gl_scene_ctx_t *sc = ctx;
  gl_view_state_t *view = sc->view;

  /* Set uniforms before draw pass */
  glUseProgram(sc->shader.program);
  glUniformMatrix4fv(sc->u_mv_location, 1, GL_FALSE,
      (const float *)params->mv);
  glUniform1f(sc->u_alpha_location, params->alpha);

  /* Flow direction mode and phase animation offset.
   * Locations are -1 for shaders without these uniforms (no-op). */
  glUniform1i(sc->flow_mode_location, rc_config.opengl_flow_direction_mode);
  glUniform1f(sc->u_phase_location, view->flow_phase);
  glUniform1f(sc->u_cos_phase_location, cosf(view->flow_phase));
  glUniform1f(sc->u_sin_phase_location, sinf(view->flow_phase));

  /* Bind LIC noise texture to unit 1 */
  if( view->noise_tex != 0 )
  {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, view->noise_tex);
    glUniform1i(sc->noise_tex_location, 1);
    glActiveTexture(GL_TEXTURE0);
  }

  gl_view_set_peel_uniforms(&sc->peel_locs, params);

  /* Depth priority handled uniformly by gl_FragDepth in the fragment
   * shader via per-vertex depth_bias: wires carry 0 (natural depth),
   * patches carry per-index positive bias (pushed behind wires). */
  glUniformMatrix4fv(sc->mvp_location, 1, GL_FALSE,
      (const float *)params->mvp);

  /* Draw each batch with its own VAO and GL mode */
  {
    int i;

    for( i = 0; i < view->content.batch_count; i++ )
    {
      if( view->content.batches[i].vertex_count > 0 )
      {
        glBindVertexArray(sc->vao[i]);

        /* Set per-batch depth bias as generic vertex attribute.
         * When the VAO has no depth_bias bound (e.g. rdpattern
         * 3-attrib layout), this sets the shader input directly. */
        if( sc->depth_bias_attrib_loc >= 0 )
          glVertexAttrib1f(sc->depth_bias_attrib_loc,
              view->content.batches[i].depth_bias);

        glUniform1f(sc->u_color_dim_location,
            view->content.batches[i].color_dim);

        glDrawArrays(view->content.batches[i].draw_mode, 0,
            view->content.batches[i].vertex_count);
      }
    }

    glBindVertexArray(0);
  }

} /* gl_scene_render() */

/*-----------------------------------------------------------------------*/

/** gl_scene_is_active() - Returns TRUE when scene has vertex data to render
 * @ctx: scene context
 */
  static gboolean
gl_scene_is_active(void *ctx)
{
  gl_scene_ctx_t *sc = ctx;

  return( sc->view->content.batch_count > 0 );

} /* gl_scene_is_active() */

/*-----------------------------------------------------------------------*/

/** gl_scene_far_extent() - Returns the scene geometry extent for clip plane calculation
 * @ctx: scene context
 * @r_max: default extent
 *
 * Uses clip_extent when set (accounts for translation offsets), falls back to r_max.
 */
  static float
gl_scene_far_extent(void *ctx, float r_max)
{
  gl_scene_ctx_t *sc = ctx;
  float result, clip_ext;

  result = r_max;
  clip_ext = sc->view->content.clip_extent;

  if( clip_ext > r_max )
  {
    result = clip_ext;
  }

  return( result );

} /* gl_scene_far_extent() */

/*-----------------------------------------------------------------------*/

/** gl_scene_free() - Free scene rendering context and GL resources
 * @ctx: scene context
 */
  static void
gl_scene_free(void *ctx)
{
  gl_scene_ctx_t *sc = ctx;

  if( !sc )
    return;

  if( sc->view->scene && sc->view->scene->cleanup )
    sc->view->scene->cleanup();

  glDeleteBuffers(GL_VIEW_MAX_BATCHES, sc->vbo);
  glDeleteVertexArrays(GL_VIEW_MAX_BATCHES, sc->vao);

  gl_shader_destroy(&sc->shader);

  g_free(sc->attrib_locations);
  g_free(sc);

} /* gl_scene_free() */

/*-----------------------------------------------------------------------*/

/** gl_view_scene_renderable_new() - Create scene renderable for primary geometry rendering
 * @state: view state
 */
  gl_renderable_t
gl_view_scene_renderable_new(gl_view_state_t *state)
{
  gl_scene_ctx_t *sc;
  gl_renderable_t r;
  gboolean ok;
  int i;

  sc = g_new0(gl_scene_ctx_t, 1);
  sc->view = state;

  ok = gl_shader_load(&sc->shader,
      state->config->vertex_shader_path,
      state->config->fragment_shader_path);

  if( !ok )
  {
    pr_err("Failed to load shaders\n");
    g_free(sc);

    return( (gl_renderable_t){0} );
  }

  sc->mvp_location = glGetUniformLocation(sc->shader.program, "mvp");
  sc->u_mv_location = glGetUniformLocation(sc->shader.program, "u_mv");
  sc->u_alpha_location = glGetUniformLocation(sc->shader.program, "u_alpha");
  sc->u_color_dim_location = glGetUniformLocation(sc->shader.program, "u_color_dim");
  sc->flow_mode_location = glGetUniformLocation(sc->shader.program, "flow_mode");
  sc->u_phase_location = glGetUniformLocation(sc->shader.program, "u_phase");
  sc->u_cos_phase_location = glGetUniformLocation(sc->shader.program, "u_cos_phase");
  sc->u_sin_phase_location = glGetUniformLocation(sc->shader.program, "u_sin_phase");
  sc->noise_tex_location = glGetUniformLocation(sc->shader.program, "noise_tex");
  gl_view_peel_locs_init(&sc->peel_locs, sc->shader.program);

  glGenVertexArrays(GL_VIEW_MAX_BATCHES, sc->vao);
  glGenBuffers(GL_VIEW_MAX_BATCHES, sc->vbo);

  sc->attrib_locations = g_new(GLint, state->config->attrib_count);

  for( i = 0; i < state->config->attrib_count; i++ )
  {
    sc->attrib_locations[i] = glGetAttribLocation(
        sc->shader.program,
        state->config->attribs[i].name);
  }

  /* Override default generic value for flow_data attribute.
   * OpenGL defaults unbound vec4 attributes to (0,0,0,1); the w=1
   * causes mag_sq=1.0 in the fragment shader, falsely activating
   * the flow/chevron/LIC block for non-patch vertices (rdpattern
   * shell, wire segments using 3-attrib config). */
  {
    GLint flow_loc = glGetAttribLocation(sc->shader.program, "flow_data");

    if( flow_loc >= 0 )
      glVertexAttrib4f(flow_loc, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  /* Cache depth_bias attribute location for per-batch generic attrib */
  sc->depth_bias_attrib_loc =
    glGetAttribLocation(sc->shader.program, "depth_bias");

  r = (gl_renderable_t){
    .render               = gl_scene_render,
    .prepare              = gl_scene_prepare,
    .destroy              = gl_scene_free,
    .is_active            = gl_scene_is_active,
    .far_extent           = gl_scene_far_extent,
    .ctx                  = sc,
    .alpha                = 1.0f,
    .origin               = {0.0f, 0.0f, 0.0f},
    .transparent_sort_order = 1,
    .transparent_on_drag  = TRUE
  };

  return( r );

} /* gl_view_scene_renderable_new() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
