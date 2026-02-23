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

#include "opengl_ground_plane.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

#define GROUND_PLANE_VERTICES 6

static const rgba_f_t ground_color = {0.4f, 0.7f, 0.3f, 1.0f};
static const point_f_3d_t ground_normal = {0.0f, 0.0f, 1.0f};

/*-----------------------------------------------------------------------*/

/* generate_ground_plane_vertices()
 *
 * Create two triangles forming a quad in the XY plane at Z=0.
 * All vertices have upward normals and green color.
 */
  static void
generate_ground_plane_vertices(lit_color_point_t *vertices, float scale)
{
  float extent;
  int i;

  extent = scale * GROUND_PLANE_EXTENT;

  /* Triangle 1: (-x, -y, 0), (-x, +y, 0), (+x, -y, 0) */
  vertices[0].point.x = -extent;
  vertices[0].point.y = -extent;
  vertices[0].point.z = 0.0f;

  vertices[1].point.x = -extent;
  vertices[1].point.y = extent;
  vertices[1].point.z = 0.0f;

  vertices[2].point.x = extent;
  vertices[2].point.y = -extent;
  vertices[2].point.z = 0.0f;

  /* Triangle 2: (-x, +y, 0), (+x, +y, 0), (+x, -y, 0) */
  vertices[3].point.x = -extent;
  vertices[3].point.y = extent;
  vertices[3].point.z = 0.0f;

  vertices[4].point.x = extent;
  vertices[4].point.y = extent;
  vertices[4].point.z = 0.0f;

  vertices[5].point.x = extent;
  vertices[5].point.y = -extent;
  vertices[5].point.z = 0.0f;

  /* All vertices have upward normal and green color */
  for( i = 0; i < GROUND_PLANE_VERTICES; i++ )
  {
    vertices[i].normal = ground_normal;
    vertices[i].color = ground_color;
  }

} /* generate_ground_plane_vertices() */

/*-----------------------------------------------------------------------*/

/* opengl_ground_plane_new()
 *
 * Allocate and initialize ground plane rendering context with VAO, VBO,
 * and checkerboard shader
 */
  opengl_ground_plane_t*
opengl_ground_plane_new(void)
{
  opengl_ground_plane_t *gp;
  lit_color_point_t vertices[GROUND_PLANE_VERTICES];
  gboolean ok;
  int i;
  const char *attrib_names[3] = {"position", "normal", "color"};

  gp = g_new0(opengl_ground_plane_t, 1);

  if( !gp )
    return( NULL );

  ok = gl_shader_load(&gp->shader,
      "/gl/ground-plane-vertex.glsl",
      "/gl/ground-plane-fragment.glsl");

  if( !ok )
  {
    pr_err("Failed to load ground plane shaders\n");
    g_free(gp);
    return( NULL );
  }

  gp->mvp_location = glGetUniformLocation(gp->shader.program, "mvp");
  gp->u_alpha_location = glGetUniformLocation(gp->shader.program, "u_alpha");

  for( i = 0; i < 3; i++ )
  {
    gp->attrib_locations[i] = glGetAttribLocation(
        gp->shader.program,
        attrib_names[i]);
  }

  glGenVertexArrays(1, &gp->vao);
  glGenBuffers(1, &gp->vbo);

  glBindVertexArray(gp->vao);
  glBindBuffer(GL_ARRAY_BUFFER, gp->vbo);

  /* Generate initial vertices with unit scale */
  generate_ground_plane_vertices(vertices, 1.0f);

  glBufferData(GL_ARRAY_BUFFER,
      sizeof(vertices),
      vertices,
      GL_DYNAMIC_DRAW);

  /* Configure vertex attrib pointers in VAO — retained for all renders */
  glEnableVertexAttribArray(gp->attrib_locations[0]);
  glVertexAttribPointer(gp->attrib_locations[0], 3, GL_FLOAT, GL_FALSE,
      sizeof(lit_color_point_t), (void *)offsetof(lit_color_point_t, point));

  glEnableVertexAttribArray(gp->attrib_locations[1]);
  glVertexAttribPointer(gp->attrib_locations[1], 3, GL_FLOAT, GL_FALSE,
      sizeof(lit_color_point_t), (void *)offsetof(lit_color_point_t, normal));

  glEnableVertexAttribArray(gp->attrib_locations[2]);
  glVertexAttribPointer(gp->attrib_locations[2], 4, GL_FLOAT, GL_FALSE,
      sizeof(lit_color_point_t), (void *)offsetof(lit_color_point_t, color));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  gp->scale = 1.0f;
  gp->initialized = TRUE;

  return( gp );

} /* opengl_ground_plane_new() */

/*-----------------------------------------------------------------------*/

/* opengl_ground_plane_free()
 *
 * Free ground plane GL resources and allocated memory
 */
  void
opengl_ground_plane_free(void *ctx)
{
  opengl_ground_plane_t *gp = ctx;

  if( !gp )
    return;

  if( gp->initialized )
  {
    gl_shader_destroy(&gp->shader);
    glDeleteBuffers(1, &gp->vbo);
    glDeleteVertexArrays(1, &gp->vao);
  }

  g_free(gp);

} /* opengl_ground_plane_free() */

/*-----------------------------------------------------------------------*/

/* opengl_ground_plane_prepare()
 *
 * Regenerate ground plane vertices scaled to r_max.
 * Skips update if scale unchanged.
 */
  void
opengl_ground_plane_prepare(void *ctx, float r_max)
{
  opengl_ground_plane_t *gp = ctx;
  lit_color_point_t vertices[GROUND_PLANE_VERTICES];

  if( !gp || !gp->initialized )
    return;

  if( r_max < 0.001f )
    r_max = 1.0f;

  if( gp->scale == r_max )
    return;

  gp->scale = r_max;

  generate_ground_plane_vertices(vertices, r_max);

  glBindBuffer(GL_ARRAY_BUFFER, gp->vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

} /* opengl_ground_plane_prepare() */

/*-----------------------------------------------------------------------*/

/* opengl_ground_plane_far_extent()
 *
 * Return spatial extent for clip plane calculation.
 * Ground plane extends GROUND_PLANE_EXTENT times r_max from origin.
 */
  float
opengl_ground_plane_far_extent(void *ctx, float r_max)
{
  (void)ctx;

  return( r_max * GROUND_PLANE_EXTENT );

} /* opengl_ground_plane_far_extent() */

/*-----------------------------------------------------------------------*/

/* opengl_ground_plane_render()
 *
 * Render ground plane quad with transparent checkerboard pattern
 */
  void
opengl_ground_plane_render(void *ctx, mat4 mvp, float alpha)
{
  opengl_ground_plane_t *gp = ctx;

  if( !gp || !gp->initialized )
    return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  glUseProgram(gp->shader.program);
  glUniform1f(gp->u_alpha_location, alpha);
  glUniformMatrix4fv(gp->mvp_location, 1, GL_FALSE, (float *)mvp);

  /* VAO has attrib config from init — bind and draw */
  glBindVertexArray(gp->vao);
  glDrawArrays(GL_TRIANGLES, 0, GROUND_PLANE_VERTICES);
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

} /* opengl_ground_plane_render() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
