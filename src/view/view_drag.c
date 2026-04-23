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

/*
 * view_drag: pointer-input accumulation for view_t rotation and pan.
 *
 * Constrained drag accumulates WR/WI deltas in drag_wr_deg/drag_wi_deg,
 * which are seeded from view_set_angles() and kept live across drag
 * sessions without re-decomposing the matrix.  Free drag pre-multiplies
 * an axis-angle rotation and re-seeds the accumulators at begin_drag
 * since the matrix is mutated independently of the accumulators.  Pan
 * accumulates screen-pixel deltas directly in pan_offset.
 */
#include "view_core.h"

/*-----------------------------------------------------------------------
 * Drag
 *----------------------------------------------------------------------*/

/** view_set_drag_mode() - Select constrained vs free rotation */
  void
view_set_drag_mode(view_t *v, drag_mode_t mode)
{
  v->drag_mode = mode;

} /* view_set_drag_mode() */

/**
 * view_begin_drag() - Record button and pointer origin
 *
 * In constrained mode the drag-session accumulators (drag_wr_deg,
 * drag_wi_deg) are already in sync from the last view_set_angles()
 * call and are left untouched so the spinner can track continuously
 * past ±90° without the asin() fold-back that Extract_View_Angles()
 * would introduce.  In free mode the matrix is mutated by axis-angle
 * pre-multiply without updating the accumulators, so they are
 * re-seeded here from Extract_View_Angles().
 */
  void
view_begin_drag(view_t *v, drag_button_t button, float x, float y)
{
  v->drag_button = button;
  v->last_ptr_x  = x;
  v->last_ptr_y  = y;

  if( v->drag_mode != VIEW_DRAG_CONSTRAINED )
  {
    view_t *owner = view_rotation_owner(v);
    double wr, wi;

    Extract_View_Angles(owner->R, &wr, &wi);
    if( isnan(wr) )
      wr = 0.0;

    v->drag_wr_deg = wr;
    v->drag_wi_deg = wi;
  }

} /* view_begin_drag() */

/** view_end_drag() - Release drag button */
  void
view_end_drag(view_t *v)
{
  v->drag_button = VIEW_DRAG_NONE;

} /* view_end_drag() */

/**
 * view_update_drag() - Dispatch drag or pan from current pointer position
 * @v: view
 * @x: pointer x (pixels)
 * @y: pointer y (pixels)
 *
 * Caller must call view_begin_drag() first; this function consumes
 * the pointer delta since the last call and updates last_ptr_*.
 */
  void
view_update_drag(view_t *v, float x, float y)
{
  float dx, dy;

  if( v->drag_button == VIEW_DRAG_NONE )
    return;

  dx = x - v->last_ptr_x;
  dy = y - v->last_ptr_y;
  v->last_ptr_x = x;
  v->last_ptr_y = y;

  if( v->drag_button == VIEW_DRAG_ROTATE )
    view_apply_drag(v, dx, dy);
  else
    view_apply_pan_delta(v, dx, dy);

} /* view_update_drag() */

/**
 * view_apply_drag() - Accumulate rotation delta from pointer delta
 * @v:  view
 * @dx: pointer delta X (pixels)
 * @dy: pointer delta Y (pixels)
 *
 * In constrained mode the current angles are extracted, the delta is
 * applied to (WR, WI), and the matrix is rebuilt.  At the pole the
 * azimuth delta is dropped (WR undefined) and only the elevation
 * delta is applied.  In free mode the delta is pre-multiplied as an
 * axis-angle rotation about screen-X and screen-Y.
 */
  void
view_apply_drag(view_t *v, float dx, float dy)
{
  view_t *owner = view_rotation_owner(v);

  if( v->drag_mode == VIEW_DRAG_CONSTRAINED )
  {
    /* Accumulate onto the drag-session angles seeded in
     * view_begin_drag().  Using the accumulator instead of a fresh
     * Extract_View_Angles() avoids the atan2() azimuth wrap at
     * +/-180 and the asin() elevation clamp at +/-90 that would
     * otherwise snap the rotation on every event. */
    v->drag_wr_deg -= (double)dx / (double)v->motion_divisor;
    v->drag_wi_deg += (double)dy / (double)v->motion_divisor;
    Build_View_Rotation_Matrix(owner->R, v->drag_wr_deg, v->drag_wi_deg);

    /* Sync accumulators to owner so view_update_spin_display on the
     * master reads exact values and the view_notify_change propagation
     * to the follower copies fresh values rather than stale ones. */
    view_sync_drag_to_owner(v);
  }
  else
  {
    /* Pre-multiply: R' = Ry * Rx * R.  cglm's glm_rotate() post-
     * multiplies into its matrix argument, so composing Ry then Rx
     * into an identity-seeded matrix yields (Ry * Rx), which we then
     * pre-multiply onto R with a single glm_mat4_mul. */
    mat4 rot;
    float angle_x = dx * VIEW_FREE_DRAG_RAD_PER_PX;
    float angle_y = dy * VIEW_FREE_DRAG_RAD_PER_PX;

    glm_mat4_identity(rot);
    glm_rotate(rot, angle_y, (vec3){1.0f, 0.0f, 0.0f});
    glm_rotate(rot, angle_x, (vec3){0.0f, 1.0f, 0.0f});
    glm_mat4_mul(rot, owner->R, owner->R);
  }

  view_notify_change(owner);

} /* view_apply_drag() */

/** view_apply_pan_delta() - Accumulate screen-pixel pan delta */
  void
view_apply_pan_delta(view_t *v, float dx, float dy)
{
  v->pan_offset[0] += dx;
  v->pan_offset[1] -= dy;
  view_notify_change(v);

} /* view_apply_pan_delta() */

/** view_reset_pan() - Clear pan offset and notify observers */
  void
view_reset_pan(view_t *v)
{
  v->pan_offset[0] = 0.0f;
  v->pan_offset[1] = 0.0f;
  view_notify_change(v);

} /* view_reset_pan() */

/*-----------------------------------------------------------------------*/
