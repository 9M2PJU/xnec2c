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

#include "opengl_view_render.h"
#include "opengl_gradient_overlay.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/* gl_view_setup_attribs()
 *
 * Configure vertex attribute pointers in VAO. Called once during prepare
 * when VBO data changes. VAO retains this state for subsequent renders.
 */
  void
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
  void
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

/* on_render()
 *
 * GtkGLArea render signal handler
 */
  static gboolean
on_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
  gl_view_state_t *state;
  gl_view_content_t content = {0};
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

    /* clip_extent accounts for translation offsets; all providers set it */
    if( effective_far < content.clip_extent )
    {
      effective_far = content.clip_extent;
    }

    nearest_point = camera_distance - content.clip_extent;
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

    state->cached_near_plane = near_plane;
    state->cached_far_plane = far_plane;

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

    float eff_alpha = r->alpha *
        (state->drag_active && r->transparent_on_drag
         ? state->drag_alpha_factor : 1.0f);

    if( eff_alpha < 1.0f )
      continue;

    if( !(active_mask & (1u << i)) )
      continue;

    r->prepare(r->ctx, content.r_max);
    r->render(r->ctx, mvp, eff_alpha);
  }

  /* Transparent pass — sorted by priority then back-to-front depth */
  {
    int trans_indices[state->renderables->len];
    int trans_orders[state->renderables->len];
    float trans_alphas[state->renderables->len];
    float trans_depths[state->renderables->len];
    int trans_count, j, k;

    trans_count = 0;

    for( i = 0; i < state->renderables->len; i++ )
    {
      gl_renderable_t *r = &g_array_index(
          state->renderables, gl_renderable_t, i);

      float eff_alpha = r->alpha *
          (state->drag_active && r->transparent_on_drag
           ? state->drag_alpha_factor : 1.0f);

      if( eff_alpha >= 1.0f )
        continue;

      if( !(active_mask & (1u << i)) )
        continue;

      trans_alphas[trans_count] = eff_alpha;
      trans_orders[trans_count] = r->transparent_sort_order;
      {
        float view_z[3];

        arcball_get_rotation_col(state->arcball, 2, view_z);
        trans_depths[trans_count] =
            view_z[0] * r->origin[0] +
            view_z[1] * r->origin[1] +
            view_z[2] * r->origin[2];
      }
      trans_indices[trans_count] = (int)i;
      trans_count++;
    }

    /* Insertion sort: ascending sort_order, then ascending depth
     * (farthest first within same priority) */
    for( j = 1; j < trans_count; j++ )
    {
      int key_order = trans_orders[j];
      float key_alpha = trans_alphas[j];
      float key_depth = trans_depths[j];
      int key_idx = trans_indices[j];

      k = j - 1;

      while( k >= 0 &&
             (trans_orders[k] > key_order ||
              (trans_orders[k] == key_order &&
               trans_depths[k] > key_depth)) )
      {
        trans_orders[k + 1] = trans_orders[k];
        trans_alphas[k + 1] = trans_alphas[k];
        trans_depths[k + 1] = trans_depths[k];
        trans_indices[k + 1] = trans_indices[k];
        k--;
      }

      trans_orders[k + 1] = key_order;
      trans_alphas[k + 1] = key_alpha;
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
      r->render(r->ctx, mvp, trans_alphas[j]);

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
    int surf_width, surf_height;

    surf_width = (int)(state->viewport_height * state->aspect);
    surf_height = (int)state->viewport_height;

    gl_view_render_tooltip(state, surf_width, surf_height);
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

/* gl_view_render_connect()
 *
 * Wire the render signal handler to a GL area widget.
 */
  void
gl_view_render_connect(GtkWidget *gl_area, gl_view_state_t *state)
{
  g_signal_connect(gl_area, "render", G_CALLBACK(on_render), state);

} /* gl_view_render_connect() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
