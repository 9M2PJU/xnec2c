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
 * view_angles: rotation matrix construction/decomposition and the NEC2
 * angle API (WR/WI, theta/phi) for view_t.
 *
 * Build_View_Rotation_Matrix and Extract_View_Angles are the canonical
 * encode/decode pair; clamp_d and extract_view_axis are shared only
 * between the two decomposition consumers and are not exported.
 */
#include "view_core.h"

static double clamp_d(double v, double lo, double hi);
static void extract_view_axis(mat4 R,
                              double *ux, double *uy, double *uz,
                              double *xy_sq);

/*-----------------------------------------------------------------------
 * Rotation math
 *----------------------------------------------------------------------*/

/**
 * Build_View_Rotation_Matrix() - Canonical Rz(-90)*Ry(wi-90)*Rz(-wr)
 * @R:      output rotation matrix
 * @wr_deg: azimuth (degrees)
 * @wi_deg: elevation (degrees)
 *
 * Coordinate system:
 *   Cairo:   Z-up world, camera at spherical(Wr,Wi) looking at origin
 *   Screen:  Y-up, camera fixed at +Z, model rotates instead
 *
 * View button mappings:
 *   X (Wr=0,  Wi=0):  +X toward camera, +Z up, +Y right
 *   Y (Wr=90, Wi=0):  +Y toward camera, +Z up, +X left
 *   Z (Wr=0,  Wi=90): +Z toward camera, +Y right, +X down
 */
  void
Build_View_Rotation_Matrix(mat4 R, double wr_deg, double wi_deg)
{
  glm_mat4_identity(R);
  glm_rotate(R, glm_rad(-90.0f),               (vec3){0.0f, 0.0f, 1.0f});
  glm_rotate(R, glm_rad((float)wi_deg - 90.0f), (vec3){0.0f, 1.0f, 0.0f});
  glm_rotate(R, glm_rad(-(float)wr_deg),        (vec3){0.0f, 0.0f, 1.0f});

} /* Build_View_Rotation_Matrix() */

/**
 * Extract_View_Angles() - ZYZ decomposition of R into (WR, WI)
 * @R:      rotation matrix
 * @wr_deg: receives azimuth (degrees); NAN at pole
 * @wi_deg: receives elevation (degrees); always finite
 *
 * At the pole the viewing axis is along +Z or -Z and azimuth is
 * undefined; callers must test isnan() on *wr_deg before use.
 */
  void
Extract_View_Angles(mat4 R, double *wr_deg, double *wi_deg)
{
  double ux, uy, uz, xy_sq;

  extract_view_axis(R, &ux, &uy, &uz, &xy_sq);

  /* Elevation (Wi) is always defined: asin(u_z) converts +Z-up camera
   * axis into NEC2 elevation above the XY plane. */
  *wi_deg = asin(clamp_d(uz, -1.0, 1.0)) * (180.0 / M_PI);

  if( xy_sq < (double)VIEW_POLE_EPS_SQ )
    *wr_deg = NAN;
  else
    *wr_deg = atan2(uy, ux) * (180.0 / M_PI);

} /* Extract_View_Angles() */

/*-----------------------------------------------------------------------
 * Rotation API
 *----------------------------------------------------------------------*/

/**
 * view_set_angles() - Replace rotation with the canonical (WR, WI) matrix
 *
 * Drag accumulator is seeded from the new angles so a constrained drag
 * started immediately afterwards is continuous from this point.
 */
  void
view_set_angles(view_t *v, double wr_deg, double wi_deg)
{
  view_t *owner = view_rotation_owner(v);
  double cur_wr, cur_wi;

  /* Idempotence guard mirrors view_set_zoom(): a Build/Extract
   * round-trip drifts in the last float bit, so without an epsilon
   * check a spin-handler feedback path never converges.  The drag
   * accumulator is re-seeded even on the early-return path so any
   * stale accumulator left by a prior free-mode drag cannot resurface
   * on the next constrained drag. */
  v->drag_wr_deg = wr_deg;
  v->drag_wi_deg = wi_deg;

  /* When a follower edits the master's matrix, keep the master's
   * accumulators in sync so view_update_spin_display on the master
   * reads the exact double-precision angles, not a lossy float
   * roundtrip through Extract_View_Angles. */
  if( owner != v )
  {
    owner->drag_wr_deg = wr_deg;
    owner->drag_wi_deg = wi_deg;
  }

  Extract_View_Angles(owner->R, &cur_wr, &cur_wi);
  if( !isnan(cur_wr) &&
      fabs(cur_wr - wr_deg) < VIEW_ANGLE_EPS &&
      fabs(cur_wi - wi_deg) < VIEW_ANGLE_EPS )
    return;

  Build_View_Rotation_Matrix(owner->R, wr_deg, wi_deg);
  view_notify_change(owner);

} /* view_set_angles() */

/** view_get_angles() - Decompose current rotation into (WR, WI) */
  void
view_get_angles(view_t *v, double *wr_deg, double *wi_deg)
{
  Extract_View_Angles(view_R(v), wr_deg, wi_deg);

} /* view_get_angles() */

/** view_get_theta_phi() - Spherical coordinates of the viewing axis */
  void
view_get_theta_phi(view_t *v, double *theta, double *phi)
{
  double ux, uy, uz, xy_sq;

  extract_view_axis(view_R(v), &ux, &uy, &uz, &xy_sq);

  *theta = acos(clamp_d(uz, -1.0, 1.0)) * (180.0 / M_PI);

  if( xy_sq < (double)VIEW_POLE_EPS_SQ )
    *phi = NAN;
  else
    *phi = atan2(uy, ux) * (180.0 / M_PI);

} /* view_get_theta_phi() */

/*-----------------------------------------------------------------------
 * Local helpers
 *----------------------------------------------------------------------*/

/** clamp_d() - Clamp a double to [lo, hi] */
  static double
clamp_d(double v, double lo, double hi)
{
  if( v < lo ) return( lo );
  if( v > hi ) return( hi );
  return( v );

} /* clamp_d() */

/**
 * extract_view_axis() - Read the viewing axis (column 2 of R) and the
 * squared magnitude of its XY projection.
 *
 * Shared by Extract_View_Angles() and view_get_theta_phi(); both
 * consumers need the same pole test against VIEW_POLE_EPS_SQ.
 */
  static void
extract_view_axis(mat4 R,
                  double *ux, double *uy, double *uz,
                  double *xy_sq)
{
  *ux = (double)R[0][2];
  *uy = (double)R[1][2];
  *uz = (double)R[2][2];
  *xy_sq = (*ux) * (*ux) + (*uy) * (*uy);

} /* extract_view_axis() */

/*-----------------------------------------------------------------------*/
