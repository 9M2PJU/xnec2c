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
 * Compiles a shader from GResource path or filesystem path
 */
  static GLuint
compile_shader(GLenum type, const char *path)
{
  GBytes *bytes;
  const char *source;
  char *file_source;
  gsize file_size;
  GLuint shader;
  GLint status, len;
  char *log;
  GError *error;
  gboolean from_file;

  error = NULL;
  bytes = g_resources_lookup_data(path, 0, &error);
  from_file = FALSE;
  file_source = NULL;

  if( !bytes )
  {
    g_clear_error(&error);

    if( !g_file_get_contents(path, &file_source, &file_size, &error) )
    {
      pr_err("Failed to load shader: %s (%s)\n", path,
        error ? error->message : "unknown error");
      g_clear_error(&error);
      return( 0 );
    }

    source = file_source;
    from_file = TRUE;
  }
  else
  {
    source = g_bytes_get_data(bytes, NULL);
  }

  shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  if( from_file )
    g_free(file_source);
  else
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
arcball_new(float aspect, float fov_degrees)
{
  arcball_state_t *ab;

  ab = g_new0(arcball_state_t, 1);
  glm_mat4_identity(ab->rotation);
  glm_vec2_zero(ab->pan_offset);
  ab->last_x = 0.0f;
  ab->last_y = 0.0f;
  ab->drag_button = 0;
  ab->aspect = aspect;
  ab->viewport_height = 1.0f;
  ab->fov_rad = glm_rad(fov_degrees);
  ab->callback_count = 0;
  ab->in_notify = FALSE;

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


/*-----------------------------------------------------------------------*/

/* arcball_set_view()
 *
 * Set arcball model rotation to match Cairo view angles.
 *
 * Coordinate system differences:
 *   Cairo:   Z-up world, camera at spherical(Wr,Wi) looking at origin
 *   Arcball: Y-up screen, camera fixed at +Z, model rotates instead
 *
 * Cairo angles:
 *   Wr = azimuth in XY plane from +X axis (0=+X, 90=+Y)
 *   Wi = elevation from XY plane toward +Z (0=horizontal, 90=top-down)
 *
 * View button mappings:
 *   X button (Wr=0,  Wi=0):  +X toward camera, +Z up, +Y right
 *   Y button (Wr=90, Wi=0):  +Y toward camera, +Z up, +X left
 *   Z button (Wr=0,  Wi=90): +Z toward camera, +Y right, +X down
 *
 * Rotation formula: Rz(-90) * Ry(Wi-90) * Rz(-Wr)
 *   Rz(-90):    Base alignment of Cairo X-axis reference to arcball
 *   Ry(Wi-90):  Elevation around Y (not X) because Cairo Z is up
 *   Rz(-Wr):    Azimuth in model space, applied first to vertices
 */
  void
arcball_set_view(arcball_state_t *ab, float wr_deg, float wi_deg)
{
  if( !ab )
    return;

  glm_mat4_identity(ab->rotation);
  glm_rotate(ab->rotation, glm_rad(-90.0f), (vec3){0, 0, 1});
  glm_rotate(ab->rotation, glm_rad(wi_deg - 90.0f), (vec3){0, 1, 0});
  glm_rotate(ab->rotation, glm_rad(-wr_deg), (vec3){0, 0, 1});

} /* arcball_set_view() */

/*-----------------------------------------------------------------------*/

/* arcball_add_callback()
 *
 * Register callback for arcball changes
 */
  void
arcball_add_callback(arcball_state_t *ab, arcball_callback_fn func, gpointer user_data)
{
  int i;

  if( !ab || !func || ab->callback_count >= ARCBALL_MAX_CALLBACKS )
    return;

  for( i = 0; i < ab->callback_count; i++ )
  {
    if( ab->callbacks[i].func == func && ab->callbacks[i].user_data == user_data )
      return;
  }

  ab->callbacks[ab->callback_count].func = func;
  ab->callbacks[ab->callback_count].user_data = user_data;
  ab->callback_count++;

} /* arcball_add_callback() */

/*-----------------------------------------------------------------------*/

/* arcball_remove_callback()
 *
 * Unregister callback
 */
  void
arcball_remove_callback(arcball_state_t *ab, arcball_callback_fn func, gpointer user_data)
{
  int i, j;

  if( !ab || !func )
    return;

  for( i = 0; i < ab->callback_count; i++ )
  {
    if( ab->callbacks[i].func == func && ab->callbacks[i].user_data == user_data )
    {
      for( j = i; j < ab->callback_count - 1; j++ )
        ab->callbacks[j] = ab->callbacks[j + 1];

      ab->callback_count--;
      return;
    }
  }

} /* arcball_remove_callback() */

/*-----------------------------------------------------------------------*/

/* arcball_notify_changed()
 *
 * Notify all registered callbacks of arcball change
 */
  void
arcball_notify_changed(arcball_state_t *ab)
{
  int i;

  if( !ab || ab->in_notify )
    return;

  ab->in_notify = TRUE;

  for( i = 0; i < ab->callback_count; i++ )
  {
    if( ab->callbacks[i].func )
      ab->callbacks[i].func(ab, ab->callbacks[i].user_data);
  }

  ab->in_notify = FALSE;

} /* arcball_notify_changed() */

/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/

/* arcball_set_viewport()
 *
 * Update viewport dimensions
 */
  void
arcball_set_viewport(arcball_state_t *ab, float height)
{
  if( height > 0.0f )
    ab->viewport_height = height;

} /* arcball_set_viewport() */

/*-----------------------------------------------------------------------*/

/* arcball_sync_view()
 *
 * Sync arcball rotation from projection parameters.
 * Handles null checks for gl_instance and arcball.
 */
  void
arcball_sync_view(gl_instance_t *gl, double wr, double wi)
{
  if( !gl || !gl->arcball )
    return;

  arcball_set_view(gl->arcball, (float)wr, (float)wi);
}

/*-----------------------------------------------------------------------*/

/* arcball_set_preset_view()
 *
 * Set arcball to preset view angles and reset pan.
 * Used by view preset buttons (X/Y/Z axis, default view).
 */
  void
arcball_set_preset_view(gl_instance_t *gl, double wr, double wi)
{
  arcball_sync_view(gl, wr, wi);

  if( gl && gl->arcball )
    arcball_reset_pan(gl->arcball);
}

/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/

static void arcball_rotate(arcball_state_t *ab, float dx, float dy);
static void arcball_pan(arcball_state_t *ab, float dx, float dy);

/*-----------------------------------------------------------------------*/

/* arcball_drag()
 *
 * Handle mouse drag - dispatches to rotate or pan based on button
 */
  void
arcball_drag(arcball_state_t *ab, float x, float y)
{
  float dx, dy;

  if( !ab || ab->drag_button == 0 )
    return;

  dx = x - ab->last_x;
  dy = y - ab->last_y;

  if( ab->drag_button == 1 )
  {
    arcball_rotate(ab, dx, dy);
  }
  else if( ab->drag_button == 2 )
  {
    arcball_pan(ab, dx / ab->viewport_height, dy / ab->viewport_height);
  }
  else
  {
    return;
  }

  ab->last_x = x;
  ab->last_y = y;

  arcball_notify_changed(ab);

} /* arcball_drag() */

/*-----------------------------------------------------------------------*/

/* arcball_rotate()
 *
 * Apply rotation from mouse drag
 */
  static void
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
 * Pan camera view - stores normalized pan offset
 */
  static void
arcball_pan(arcball_state_t *ab, float dx, float dy)
{
  ab->pan_offset[0] += dx;
  ab->pan_offset[1] -= dy;

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

/* arcball_reset_pan()
 *
 * Reset pan offset to center view
 */
  void
arcball_reset_pan(arcball_state_t *ab)
{
  if( !ab )
    return;

  glm_vec2_zero(ab->pan_offset);

} /* arcball_reset_pan() */

/*-----------------------------------------------------------------------*/

/* arcball_get_mvp()
 *
 * Compute model-view-projection matrix from arcball state and scene parameters
 */
  void
arcball_get_mvp(arcball_state_t *ab, mat4 dest, float distance, float model_scale)
{
  mat4 view, proj, model, trans;
  vec3 eye_pos, center_pos, up;

  glm_mat4_identity(model);
  glm_mat4_copy(ab->rotation, model);
  glm_scale(model, (vec3){model_scale, model_scale, model_scale});

  glm_mat4_identity(trans);
  glm_translate(trans, (vec3){ab->pan_offset[0], ab->pan_offset[1], 0.0f});
  glm_mat4_mul(trans, model, model);

  glm_vec3_copy((vec3){0.0f, 0.0f, distance}, eye_pos);
  glm_vec3_zero(center_pos);
  glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);

  glm_lookat(eye_pos, center_pos, up, view);
  glm_perspective(ab->fov_rad, ab->aspect, 0.1f, 100.0f, proj);

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

  inst->arcball = arcball_new(aspect, 60.0f);
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

/* gl_area_cleanup_state()
 *
 * Free GL state while context is still current. Must be called from
 * on_unrealize handler, not GDestroyNotify, because the GL context
 * is destroyed before widget finalization.
 *
 * Uses g_object_steal_data to remove without triggering destroy notify.
 */
  void
gl_area_cleanup_state(GtkGLArea *area, const char *key,
    void (*free_func)(void *))
{
  void *state;

  gtk_gl_area_make_current(area);

  state = g_object_steal_data(G_OBJECT(area), key);
  if( state )
    free_func(state);

} /* gl_area_cleanup_state() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
