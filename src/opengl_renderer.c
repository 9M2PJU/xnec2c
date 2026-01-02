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

#include "opengl_renderer.h"
#include "shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/* compile_shader()
 *
 * Compiles a shader from GResource path
 */
  static GLuint
compile_shader(GLenum type, const char *path)
{
  GBytes *bytes;
  const char *source;
  GLuint shader;
  GLint status, len;
  char *log;

  bytes = g_resources_lookup_data(path, 0, NULL);
  if( !bytes )
  {
    pr_err("Failed to load shader: %s\n", path);
    return( 0 );
  }

  source = g_bytes_get_data(bytes, NULL);
  shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  g_bytes_unref(bytes);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if( status == GL_FALSE )
  {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    log = g_malloc(len + 1);
    glGetShaderInfoLog(shader, len, NULL, log);
    pr_err("Shader compile error: %s\n", log);
    g_free(log);
    glDeleteShader(shader);
    return( 0 );
  }

  return( shader );

} /* compile_shader() */

/*-----------------------------------------------------------------------*/

/* gl_shader_load()
 *
 * Loads and compiles vertex and fragment shaders
 */
  gboolean
gl_shader_load(gl_shader_t *shader,
  const char *vertex_path, const char *fragment_path)
{
  GLint status;

  shader->vertex = compile_shader(GL_VERTEX_SHADER, vertex_path);
  if( !shader->vertex )
    return( FALSE );

  shader->fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_path);
  if( !shader->fragment )
  {
    glDeleteShader(shader->vertex);
    return( FALSE );
  }

  shader->program = glCreateProgram();
  glAttachShader(shader->program, shader->vertex);
  glAttachShader(shader->program, shader->fragment);
  glLinkProgram(shader->program);

  glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
  if( status == GL_FALSE )
  {
    pr_err("Shader link failed\n");
    glDeleteProgram(shader->program);
    glDeleteShader(shader->vertex);
    glDeleteShader(shader->fragment);
    return( FALSE );
  }

  return( TRUE );

} /* gl_shader_load() */

/*-----------------------------------------------------------------------*/

/* gl_shader_destroy()
 *
 * Cleanup shader resources
 */
  void
gl_shader_destroy(gl_shader_t *shader)
{
  if( shader->program )
    glDeleteProgram(shader->program);
  if( shader->vertex )
    glDeleteShader(shader->vertex);
  if( shader->fragment )
    glDeleteShader(shader->fragment);

  shader->program = shader->vertex = shader->fragment = 0;

} /* gl_shader_destroy() */

/*-----------------------------------------------------------------------*/

/* arcball_init()
 *
 * Initialize arcball camera state
 */
  void
arcball_init(arcball_state_t *state, float distance)
{
  glm_vec3_copy((vec3){0, 0, distance}, state->eye);
  glm_vec3_zero(state->center);
  glm_mat4_identity(state->rotation);
  state->distance = distance;
  state->zoom = 1.0f;
  state->last_x = 0.0f;
  state->last_y = 0.0f;

} /* arcball_init() */

/*-----------------------------------------------------------------------*/

/* arcball_rotate()
 *
 * Apply rotation from mouse drag
 */
  void
arcball_rotate(arcball_state_t *state, float dx, float dy)
{
  mat4 rot_x, rot_y;
  vec3 up = {0, 1, 0};
  vec3 right = {1, 0, 0};
  float angle_x, angle_y;

  angle_x = dx * 0.01f;
  angle_y = dy * 0.01f;

  glm_mat4_identity(rot_y);
  glm_rotate(rot_y, angle_y, right);

  glm_mat4_identity(rot_x);
  glm_rotate(rot_x, angle_x, up);

  glm_mat4_mul(rot_x, state->rotation, state->rotation);
  glm_mat4_mul(rot_y, state->rotation, state->rotation);

} /* arcball_rotate() */

/*-----------------------------------------------------------------------*/

/* arcball_get_mvp()
 *
 * Compute model-view-projection matrix
 */
  void
arcball_get_mvp(arcball_state_t *state, mat4 dest,
  float aspect, float fov)
{
  mat4 view, proj, model;
  vec3 up = {0, 1, 0};

  glm_mat4_identity(model);
  glm_mat4_copy(state->rotation, model);

  glm_lookat(state->eye, state->center, up, view);
  glm_perspective(glm_rad(fov), aspect, 0.1f, 100.0f, proj);

  glm_mat4_mul(proj, view, dest);
  glm_mat4_mul(dest, model, dest);

} /* arcball_get_mvp() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
