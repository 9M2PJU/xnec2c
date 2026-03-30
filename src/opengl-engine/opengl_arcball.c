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

#include "opengl_arcball.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/* Forward declarations */
static void arcball_rotate(arcball_state_t *ab, float dx, float dy);
static void arcball_rotate_constrained(arcball_state_t *ab, float dx, float dy);

/*-----------------------------------------------------------------------*/

/** arcball_new() - Allocate and initialize arcball camera state
 * @motion_divisor: controls drag sensitivity in constrained mode
 */
  arcball_state_t*
arcball_new(float motion_divisor)
{
  arcball_state_t *ab;

  ab = g_new0(arcball_state_t, 1);
  glm_mat4_identity(ab->rotation);
  ab->last_x = 0.0f;
  ab->last_y = 0.0f;
  ab->drag_button = 0;
  ab->callback_count = 0;
  ab->in_notify = FALSE;
  ab->drag_mode = ARCBALL_DRAG_CONSTRAINED;
  ab->wr_deg = 45.0f;
  ab->wi_deg = 45.0f;
  ab->motion_divisor = motion_divisor;

  return( ab );

} /* arcball_new() */

/*-----------------------------------------------------------------------*/

/** arcball_free() - Free arcball camera state
 * @ab: arcball state to free
 */
  void
arcball_free(arcball_state_t *ab)
{
  if( ab )
    g_free(ab);

} /* arcball_free() */

/*-----------------------------------------------------------------------*/

/** arcball_wrap_angles() - Normalize WR to [0,360) and WI to [-180,180) to match NEC2 convention
 * @ab: arcball state
 */
  static void
arcball_wrap_angles(arcball_state_t *ab)
{
  while( ab->wr_deg < 0.0f )
    ab->wr_deg += 360.0f;
  while( ab->wr_deg >= 360.0f )
    ab->wr_deg -= 360.0f;

  while( ab->wi_deg < -180.0f )
    ab->wi_deg += 360.0f;
  while( ab->wi_deg >= 180.0f )
    ab->wi_deg -= 360.0f;

} /* arcball_wrap_angles() */

/*-----------------------------------------------------------------------*/

/** arcball_extract_angles() - Extract WR/WI from current rotation matrix (ZYZ Euler decomposition)
 * @ab: arcball state
 */
  static void
arcball_extract_angles(arcball_state_t *ab)
{
  float r22, r21, r01, r11;
  float beta, gamma;

  r22 = ab->rotation[2][2];
  r21 = ab->rotation[2][1];
  r01 = ab->rotation[0][1];
  r11 = ab->rotation[1][1];

  beta = acos(glm_clamp(r22, -1.0f, 1.0f));

  if( fabs(sin(beta)) > 0.001f )
  {
    gamma = atan2(r21, r01);
  }
  else
  {
    gamma = atan2(-r11, r01);
  }

  ab->wi_deg = glm_deg(beta) - 90.0f;
  ab->wr_deg = -glm_deg(gamma);

  arcball_wrap_angles(ab);

} /* arcball_extract_angles() */

/*-----------------------------------------------------------------------*/

/** arcball_set_view() - Set arcball model rotation to match Cairo view angles
 * @ab: arcball state
 * @wr_deg: azimuth angle in degrees
 * @wi_deg: elevation angle in degrees
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

  ab->wr_deg = wr_deg;
  ab->wi_deg = wi_deg;

  glm_mat4_identity(ab->rotation);
  glm_rotate(ab->rotation, glm_rad(-90.0f), (vec3){0, 0, 1});
  glm_rotate(ab->rotation, glm_rad(wi_deg - 90.0f), (vec3){0, 1, 0});
  glm_rotate(ab->rotation, glm_rad(-wr_deg), (vec3){0, 0, 1});

} /* arcball_set_view() */

/*-----------------------------------------------------------------------*/

/** arcball_add_callback() - Register callback for arcball changes
 * @ab: arcball state
 * @func: callback function
 * @user_data: user data passed to callback
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

/** arcball_remove_callback() - Unregister callback
 * @ab: arcball state
 * @func: callback function to remove
 * @user_data: user data associated with callback
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

/** arcball_notify_changed() - Notify all registered callbacks of arcball change
 * @ab: arcball state
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

/** arcball_copy_rotation() - Copy rotation state from one arcball to another
 * @dst: destination arcball state
 * @src: source arcball state
 */
  void
arcball_copy_rotation(arcball_state_t *dst, const arcball_state_t *src)
{
  if( !dst || !src )
    return;

  glm_mat4_copy((vec4 *)src->rotation, dst->rotation);

} /* arcball_copy_rotation() */

/*-----------------------------------------------------------------------*/

/** arcball_drag() - Handle mouse drag for rotation (button 1 only)
 * @ab: arcball state
 * @x: current mouse x coordinate
 * @y: current mouse y coordinate
 * @viewport_height: viewport height
 *
 * Pan (button 2) is handled by the view layer.
 */
  void
arcball_drag(arcball_state_t *ab, float x, float y, float viewport_height)
{
  float dx, dy;

  if( !ab || ab->drag_button == 0 )
    return;

  if( ab->drag_button != 1 )
    return;

  dx = x - ab->last_x;
  dy = y - ab->last_y;

  if( ab->drag_mode == ARCBALL_DRAG_CONSTRAINED )
  {
    arcball_rotate_constrained(ab, dx, dy);
  }
  else
  {
    arcball_rotate(ab, dx, dy);
  }

  ab->last_x = x;
  ab->last_y = y;

  arcball_notify_changed(ab);

} /* arcball_drag() */

/*-----------------------------------------------------------------------*/

/** arcball_rotate() - Apply rotation from mouse drag
 * @ab: arcball state
 * @dx: mouse delta x
 * @dy: mouse delta y
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

/** arcball_rotate_constrained() - Constrained rotation updates WR/WI and rebuilds matrix
 * @ab: arcball state
 * @dx: mouse delta x
 * @dy: mouse delta y
 */
  static void
arcball_rotate_constrained(arcball_state_t *ab, float dx, float dy)
{
  ab->wr_deg -= dx / ab->motion_divisor;
  ab->wi_deg += dy / ab->motion_divisor;

  arcball_wrap_angles(ab);

  arcball_set_view(ab, ab->wr_deg, ab->wi_deg);

} /* arcball_rotate_constrained() */

/*-----------------------------------------------------------------------*/

/** arcball_begin_drag() - Begin mouse drag operation
 * @ab: arcball state
 * @button: mouse button number
 * @x: mouse x coordinate
 * @y: mouse y coordinate
 */
  void
arcball_begin_drag(arcball_state_t *ab, int button, float x, float y)
{
  ab->drag_button = button;
  ab->last_x = x;
  ab->last_y = y;

} /* arcball_begin_drag() */

/*-----------------------------------------------------------------------*/

/** arcball_end_drag() - End mouse drag operation
 * @ab: arcball state
 */
  void
arcball_end_drag(arcball_state_t *ab)
{
  ab->drag_button = 0;

} /* arcball_end_drag() */

/*-----------------------------------------------------------------------*/

/** arcball_get_mvp() - Compute model-view-projection and model-view matrices
 * @ab: arcball state
 * @dest: destination MVP matrix (projection * view * model)
 * @mv_dest: destination model-view matrix (view * model, no projection)
 * @pan_offset: pan offset (provided by view layer)
 * @distance: camera distance
 * @model_scale: model scale factor
 * @aspect: viewport aspect ratio
 * @fov_rad: field of view in radians
 * @near_plane: near clipping plane distance
 * @far_plane: far clipping plane distance
 *
 * Pan offset is provided by the view layer as it is per-view state.
 */
  void
arcball_get_mvp(arcball_state_t *ab, mat4 dest, mat4 mv_dest,
    const vec2 pan_offset, float distance, float model_scale, float aspect,
    float fov_rad, float near_plane, float far_plane)
{
  mat4 view, proj, model, trans;
  vec3 eye_pos, center_pos, up;

  glm_mat4_identity(model);
  glm_mat4_copy(ab->rotation, model);
  glm_scale(model, (vec3){model_scale, model_scale, model_scale});

  glm_mat4_identity(trans);
  glm_translate(trans, (vec3){pan_offset[0], pan_offset[1], 0.0f});
  glm_mat4_mul(trans, model, model);

  glm_vec3_copy((vec3){0.0f, 0.0f, distance}, eye_pos);
  glm_vec3_zero(center_pos);
  glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);

  glm_lookat(eye_pos, center_pos, up, view);

  /* Match perspective visible height at camera distance so scene size is
   * unchanged when toggling between projection modes. */
  if( rc_config.opengl_orthographic )
  {
    float half_h = distance * tanf(fov_rad / 2.0f);
    glm_ortho(-half_h * aspect, half_h * aspect, -half_h, half_h,
        near_plane, far_plane, proj);
  }
  else
  {
    glm_perspective(fov_rad, aspect, near_plane, far_plane, proj);
  }

  /* Model-view (no projection) for lighting normal/position transforms */
  glm_mat4_mul(view, model, mv_dest);

  glm_mat4_mul(proj, view, dest);
  glm_mat4_mul(dest, model, dest);

} /* arcball_get_mvp() */

/*-----------------------------------------------------------------------*/

/** arcball_set_drag_mode() - Set rotation mode, extract angles if switching to constrained
 * @ab: arcball state
 * @mode: drag mode to set
 */
  void
arcball_set_drag_mode(arcball_state_t *ab, arcball_drag_mode_t mode)
{
  if( !ab )
    return;

  if( mode == ARCBALL_DRAG_CONSTRAINED && ab->drag_mode != ARCBALL_DRAG_CONSTRAINED )
    arcball_extract_angles(ab);

  ab->drag_mode = mode;

} /* arcball_set_drag_mode() */

/*-----------------------------------------------------------------------*/

/** arcball_get_drag_mode() - Get current rotation mode
 * @ab: arcball state
 */
  arcball_drag_mode_t
arcball_get_drag_mode(arcball_state_t *ab)
{
  if( !ab )
    return( ARCBALL_DRAG_FREE );

  return( ab->drag_mode );

} /* arcball_get_drag_mode() */

/*-----------------------------------------------------------------------*/

/** arcball_get_angles() - Get current WR/WI angles (meaningful in constrained mode)
 * @ab: arcball state
 * @wr: pointer to store WR angle
 * @wi: pointer to store WI angle
 */
  void
arcball_get_angles(arcball_state_t *ab, float *wr, float *wi)
{
  if( !ab )
    return;

  if( wr )
    *wr = ab->wr_deg;
  if( wi )
    *wi = ab->wi_deg;

} /* arcball_get_angles() */

/*-----------------------------------------------------------------------*/

/** arcball_get_drag_button() - Return the currently held mouse button (0 if no drag active)
 * @ab: arcball state
 */
  int
arcball_get_drag_button(arcball_state_t *ab)
{
  if( !ab )
    return( 0 );

  return( ab->drag_button );

} /* arcball_get_drag_button() */

/*-----------------------------------------------------------------------*/

/** arcball_get_last_pos() - Retrieve the last recorded mouse position
 * @ab: arcball state
 * @x: pointer to store x coordinate
 * @y: pointer to store y coordinate
 */
  void
arcball_get_last_pos(arcball_state_t *ab, float *x, float *y)
{
  if( !ab )
    return;

  if( x )
    *x = ab->last_x;
  if( y )
    *y = ab->last_y;

} /* arcball_get_last_pos() */

/*-----------------------------------------------------------------------*/

/** arcball_update_last_pos() - Update the stored last mouse position for delta computation
 * @ab: arcball state
 * @x: new x coordinate
 * @y: new y coordinate
 */
  void
arcball_update_last_pos(arcball_state_t *ab, float x, float y)
{
  if( !ab )
    return;

  ab->last_x = x;
  ab->last_y = y;

} /* arcball_update_last_pos() */

/*-----------------------------------------------------------------------*/

/** arcball_get_rotation_col() - Extract one column from the rotation matrix (e.g. col=2 for view Z axis)
 * @ab: arcball state
 * @col: column index
 * @out: output array for column values
 */
  void
arcball_get_rotation_col(arcball_state_t *ab, int col, float out[3])
{
  if( !ab || col < 0 || col > 3 )
    return;

  out[0] = ab->rotation[0][col];
  out[1] = ab->rotation[1][col];
  out[2] = ab->rotation[2][col];

} /* arcball_get_rotation_col() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
