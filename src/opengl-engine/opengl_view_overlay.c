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

#include "opengl_view_overlay.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

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

/* Forward declarations for callbacks */
static void gl_overlay_prepare(void *ctx, float r_max);
static void gl_overlay_render(void *ctx, mat4 mvp, float alpha);
static gboolean gl_overlay_is_active(void *ctx);
static float gl_overlay_far_extent(void *ctx, float r_max);
static void gl_overlay_free(void *ctx);

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

/* gl_view_overlay_renderable_new()
 *
 * Create overlay renderable for second-pass rendering
 */
  gl_renderable_t
gl_view_overlay_renderable_new(gl_view_state_t *state)
{
  const gl_overlay_config_t *ocfg;
  gl_overlay_ctx_t *ovl;
  gl_renderable_t r;
  gboolean ok;
  int i;

  if( !state->scene->overlay_config )
  {
    return( (gl_renderable_t){0} );
  }

  ocfg = state->scene->overlay_config;

  ovl = g_new0(gl_overlay_ctx_t, 1);
  ovl->view = state;

  ok = gl_shader_load(&ovl->shader,
      ocfg->vertex_shader_path,
      ocfg->fragment_shader_path);

  if( !ok )
  {
    pr_err("Failed to load overlay shaders\n");
    g_free(ovl);

    return( (gl_renderable_t){0} );
  }

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
    .transparent_sort_order = 0,
    .transparent_on_drag  = TRUE
  };

  return( r );

} /* gl_view_overlay_renderable_new() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
