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

/* arcball_new()
 *
 * Allocate and initialize arcball camera state
 */
  arcball_state_t*
arcball_new(float distance, float aspect)
{
  arcball_state_t *ab;

  ab = g_new0(arcball_state_t, 1);
  glm_vec3_copy((vec3){0, 0, distance}, ab->eye);
  glm_vec3_zero(ab->center);
  glm_vec3_copy((vec3){0, 1, 0}, ab->up);
  glm_mat4_identity(ab->rotation);
  ab->distance = distance;
  ab->zoom = 1.0f;
  glm_vec2_zero(ab->pan_offset);
  ab->last_x = 0.0f;
  ab->last_y = 0.0f;
  ab->drag_button = 0;
  ab->aspect = aspect;

  return( ab );

} /* arcball_new() */

/*-----------------------------------------------------------------------*/

/* arcball_free()
 *
 * Free arcball camera state
 */
  void
arcball_free(arcball_state_t *ab)
{
  if( ab )
    g_free(ab);

} /* arcball_free() */

/*-----------------------------------------------------------------------*/

/* arcball_set_aspect()
 *
 * Update aspect ratio
 */
  void
arcball_set_aspect(arcball_state_t *ab, float aspect)
{
  ab->aspect = aspect;

} /* arcball_set_aspect() */

/*-----------------------------------------------------------------------*/

/* arcball_set_distance()
 *
 * Set camera distance from origin
 */
  void
arcball_set_distance(arcball_state_t *ab, float distance)
{
  ab->distance = distance;
  ab->eye[2] = distance;

} /* arcball_set_distance() */

/*-----------------------------------------------------------------------*/

/* arcball_rotate()
 *
 * Apply rotation from mouse drag
 */
  void
arcball_rotate(arcball_state_t *ab, float dx, float dy)
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

  glm_mat4_mul(rot_x, ab->rotation, ab->rotation);
  glm_mat4_mul(rot_y, ab->rotation, ab->rotation);

} /* arcball_rotate() */

/*-----------------------------------------------------------------------*/

/* arcball_pan()
 *
 * Pan camera view
 */
  void
arcball_pan(arcball_state_t *ab, float dx, float dy)
{
  float scale;

  scale = ab->distance * 0.001f;
  ab->pan_offset[0] -= dx * scale;
  ab->pan_offset[1] += dy * scale;

} /* arcball_pan() */

/*-----------------------------------------------------------------------*/

/* arcball_begin_drag()
 *
 * Begin mouse drag operation
 */
  void
arcball_begin_drag(arcball_state_t *ab, int button, float x, float y)
{
  ab->drag_button = button;
  ab->last_x = x;
  ab->last_y = y;

} /* arcball_begin_drag() */

/*-----------------------------------------------------------------------*/

/* arcball_drag()
 *
 * Process mouse drag motion
 */
  void
arcball_drag(arcball_state_t *ab, float x, float y)
{
  float dx, dy;

  dx = x - ab->last_x;
  dy = y - ab->last_y;

  if( ab->drag_button == 1 )
  {
    arcball_rotate(ab, dx, dy);
  }
  else if( ab->drag_button == 2 )
  {
    arcball_pan(ab, dx, dy);
  }
  else
  {
    /* No drag active */
    return;
  }

  ab->last_x = x;
  ab->last_y = y;

} /* arcball_drag() */

/*-----------------------------------------------------------------------*/

/* arcball_end_drag()
 *
 * End mouse drag operation
 */
  void
arcball_end_drag(arcball_state_t *ab)
{
  ab->drag_button = 0;

} /* arcball_end_drag() */

/*-----------------------------------------------------------------------*/

/* arcball_zoom()
 *
 * Zoom camera
 */
  void
arcball_zoom(arcball_state_t *ab, float delta)
{
  ab->distance += delta;

  if( ab->distance < 0.5f )
    ab->distance = 0.5f;
  if( ab->distance > 20.0f )
    ab->distance = 20.0f;

  ab->eye[2] = ab->distance;

} /* arcball_zoom() */

/*-----------------------------------------------------------------------*/

/* arcball_get_mvp()
 *
 * Compute model-view-projection matrix
 */
  void
arcball_get_mvp(arcball_state_t *ab, mat4 dest, float zoom, float fov)
{
  mat4 view, proj, model;
  vec3 eye_pos, center_pos;

  glm_mat4_identity(model);
  glm_mat4_copy(ab->rotation, model);
  glm_scale(model, (vec3){zoom, zoom, zoom});

  glm_vec3_copy(ab->eye, eye_pos);
  glm_vec3_copy(ab->center, center_pos);
  center_pos[0] += ab->pan_offset[0];
  center_pos[1] += ab->pan_offset[1];

  glm_lookat(eye_pos, center_pos, ab->up, view);
  glm_perspective(glm_rad(fov), ab->aspect, 0.1f, 100.0f, proj);

  glm_mat4_mul(proj, view, dest);
  glm_mat4_mul(dest, model, dest);

} /* arcball_get_mvp() */

/*-----------------------------------------------------------------------*/

/* gl_instance_new()
 *
 * Allocate and initialize GL instance
 */
  gl_instance_t*
gl_instance_new(const char *vert_shader, const char *frag_shader,
  float arcball_distance, float aspect)
{
  gl_instance_t *inst;
  gboolean ok;

  inst = g_new0(gl_instance_t, 1);

  ok = gl_shader_load(&inst->shader, vert_shader, frag_shader);
  if( !ok )
  {
    g_free(inst);
    return( NULL );
  }

  inst->mvp_location = glGetUniformLocation(inst->shader.program, "mvp");

  glGenVertexArrays(1, &inst->vao);
  glGenBuffers(1, &inst->vbo);

  inst->arcball = arcball_new(arcball_distance, aspect);
  inst->initialized = TRUE;

  return( inst );

} /* gl_instance_new() */

/*-----------------------------------------------------------------------*/

/* gl_instance_free()
 *
 * Free GL instance resources
 */
  void
gl_instance_free(gl_instance_t *inst)
{
  if( !inst )
    return;

  if( inst->vbo )
    glDeleteBuffers(1, &inst->vbo);
  if( inst->vao )
    glDeleteVertexArrays(1, &inst->vao);

  gl_shader_destroy(&inst->shader);
  arcball_free(inst->arcball);

  g_free(inst);

} /* gl_instance_free() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
