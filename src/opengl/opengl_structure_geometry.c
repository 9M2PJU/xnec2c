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

#include "opengl_structure_geometry.h"
#include "opengl_structure.h"
#include "../shared.h"
#include "../draw.h"

#ifdef HAVE_OPENGL

#include "../opengl-engine/opengl_cylinder.h"

/* Minimum radius as fraction of model extent for visible cylinders */
#define CYLINDER_MIN_VISIBLE_FRACTION 0.0015

/* Default scale factor for cylinder radius display */
#define CYLINDER_RADIUS_SCALE_DEFAULT 1.0

/* Number of sides for cylinder rendering */
#define STRUCTURE_CYLINDER_SEGMENTS 24

/* Arrow geometry in UV patch space (0..1 range), 80% of box */
#define LINE_ARROW_TAIL_OFF    -0.24f
#define LINE_ARROW_NECK_OFF     0.04f
#define LINE_ARROW_TIP_OFF      0.28f
#define LINE_ARROW_SHAFT_HW     0.024f
#define LINE_ARROW_HEAD_HS      0.096f


/* Shared geometry buffer accessible by both structure window and overlay */
static structure_vertex_t *structure_vertices = NULL;
static int structure_vertex_count = 0;
static unsigned int structure_geometry_generation = 1;
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;
static float structure_view_scale = 1.0f;
static GLenum structure_draw_mode = GL_TRIANGLES;

/* Track previous radius scale to detect changes requiring regeneration */
static double structure_last_radius_scale = CYLINDER_RADIUS_SCALE_DEFAULT;

/* Current flow phase for line-mode arrow angle computation (radians).
 * Set by Animate_Flow_Phase; triangle-mode uses shader u_phase instead. */
static float structure_flow_phase = 0.0f;

/* Public accessor for shared geometry */
static structure_overlay_data_t shared_overlay_data = { 0 };

/*-----------------------------------------------------------------------*/

/** opengl_structure_get_draw_mode() - Derive draw mode from global flags
 */
  static structure_draw_mode_t
opengl_structure_get_draw_mode(void)
{
  if( isFlagSet(DRAW_CURRENTS) )
    return( STRUCTURE_DRAW_CURRENTS );

  if( isFlagSet(DRAW_CHARGES) )
    return( STRUCTURE_DRAW_CHARGES );

  return( STRUCTURE_DRAW_GEOMETRY );

} /* opengl_structure_get_draw_mode() */

/*-----------------------------------------------------------------------*/

/** get_segment_magnitude() - Compute current or charge magnitude for a segment
 * @idx: segment index into crnt arrays
 * @mode: STRUCTURE_DRAW_CURRENTS or STRUCTURE_DRAW_CHARGES
 */
  static double
get_segment_magnitude(int idx, structure_draw_mode_t mode)
{
  if( !crnt.valid || idx < 0 )
    return( 0.0 );

  if( mode == STRUCTURE_DRAW_CURRENTS && crnt.cur != NULL )
    return( cabs(crnt.cur[idx]) );

  if( mode == STRUCTURE_DRAW_CHARGES && crnt.bir != NULL && crnt.bii != NULL )
    return( cabs(cmplx(crnt.bir[idx], crnt.bii[idx])) );

  return( 0.0 );
}

/*-----------------------------------------------------------------------*/

/** get_patch_tangent_projections() - Project patch current onto tangent axes as complex phasors
 * @idx: patch index (0-based into data.m)
 * @ct1: output complex projection onto t1
 * @ct2: output complex projection onto t2
 *
 * Preserves phase information needed for polarization analysis.
 * Caller must ensure crnt.cur is valid and idx is in range.
 */
  static void
get_patch_tangent_projections(int idx,
    complex double *ct1, complex double *ct2)
{
  int ci = data.n + idx * 3;
  complex double cur_x = crnt.cur[ci];
  complex double cur_y = crnt.cur[ci + 1];
  complex double cur_z = crnt.cur[ci + 2];

  *ct1 = cur_x * data.t1x[idx] + cur_y * data.t1y[idx] + cur_z * data.t1z[idx];
  *ct2 = cur_x * data.t2x[idx] + cur_y * data.t2y[idx] + cur_z * data.t2z[idx];
}

/*-----------------------------------------------------------------------*/

/** get_patch_tangent_magnitudes() - Project patch current onto tangent axes (magnitude only)
 * @idx: patch index (0-based into data.m)
 * @ct1m: output magnitude along t1
 * @ct2m: output magnitude along t2
 *
 * Wrapper around get_patch_tangent_projections() for callers
 * that need only magnitudes (color mapping, RSS computation).
 */
  static void
get_patch_tangent_magnitudes(int idx, double *ct1m, double *ct2m)
{
  complex double ct1, ct2;

  get_patch_tangent_projections(idx, &ct1, &ct2);
  *ct1m = cabs(ct1);
  *ct2m = cabs(ct2);
}

/*-----------------------------------------------------------------------*/

/** get_patch_current_magnitude() - RSS magnitude of patch surface currents
 * @idx: patch index (0-based into data.m)
 */
  static double
get_patch_current_magnitude(int idx)
{
  double ct1m, ct2m;

  get_patch_tangent_magnitudes(idx, &ct1m, &ct2m);

  return( sqrt(ct1m * ct1m + ct2m * ct2m) );
}

/*-----------------------------------------------------------------------*/

/** get_patch_normal() - Surface normal via cross product of t1 and t2
 * @idx: patch index (0-based into data.m)
 */
  static void
get_patch_normal(int idx, float *nx, float *ny, float *nz)
{
  *nx = (float)(data.t1y[idx] * data.t2z[idx] - data.t1z[idx] * data.t2y[idx]);
  *ny = (float)(data.t1z[idx] * data.t2x[idx] - data.t1x[idx] * data.t2z[idx]);
  *nz = (float)(data.t1x[idx] * data.t2y[idx] - data.t1y[idx] * data.t2x[idx]);
}

/*-----------------------------------------------------------------------*/

/** get_patch_color() - Determine patch display color based on draw mode
 * @idx: patch index (0-based into data.m)
 * @mode: current structure draw mode
 * @cmax: maximum current magnitude for color scaling
 */
  static void
get_patch_color(int idx, structure_draw_mode_t mode, double cmax,
    float *out_r, float *out_g, float *out_b)
{
  if( mode == STRUCTURE_DRAW_CURRENTS && cmax > 0.0 && crnt.valid && crnt.cur != NULL )
  {
    double mag = get_patch_current_magnitude(idx);
    double red, grn, blu;

    Value_to_Color(&red, &grn, &blu, mag, cmax);
    *out_r = (float)red;
    *out_g = (float)grn;
    *out_b = (float)blu;
  }
  else if( mode == STRUCTURE_DRAW_CHARGES )
  {
    *out_r = 0.5f;
    *out_g = 0.5f;
    *out_b = 0.5f;
  }
  else
  {
    *out_r = 0.2f;
    *out_g = 0.4f;
    *out_b = 0.9f;
  }
}

/*-----------------------------------------------------------------------*/

/** get_patch_flow_data() - Populate flow_data with complex tangent projections
 * @idx: patch index (0-based into data.m)
 * @mode: current structure draw mode
 * @cmax: maximum current magnitude for ratio scaling
 * @fd: output array of 4 floats {Re(ct1), Im(ct1), Re(ct2), Im(ct2)}
 *      scaled by mag_ratio so the shader can derive both direction and intensity
 *
 * All four components are zero when current data is absent or below threshold.
 * The shader computes instantaneous direction at arbitrary phase from these
 * complex components, enabling all visualization modes without CPU recomputation.
 */
  static void
get_patch_flow_data(int idx, structure_draw_mode_t mode, double cmax,
    float *fd)
{
  if( mode == STRUCTURE_DRAW_CURRENTS && cmax > 0.0 &&
      crnt.valid && crnt.cur != NULL )
  {
    complex double ct1, ct2;
    double scale;

    get_patch_tangent_projections(idx, &ct1, &ct2);
    scale = 1.0 / cmax;

    /* Normalize by cmax so sqrt(dot(fd,fd)) recovers mag_ratio (0..1).
     * Phase relationships preserved; shader computes direction at any phase. */
    fd[0] = (float)(creal(ct1) * scale);
    fd[1] = (float)(cimag(ct1) * scale);
    fd[2] = (float)(creal(ct2) * scale);
    fd[3] = (float)(cimag(ct2) * scale);
  }
  else
  {
    fd[0] = 0.0f;
    fd[1] = 0.0f;
    fd[2] = 0.0f;
    fd[3] = 0.0f;
  }
}

/*-----------------------------------------------------------------------*/

/** set_structure_vertex() - Set all fields of an extended structure vertex
 * @fd0..fd3: flow_data components {Re(ct1), Im(ct1), Re(ct2), Im(ct2)}
 */
  static void
set_structure_vertex(structure_vertex_t *sv,
    float px, float py, float pz,
    float nx, float ny, float nz,
    float cr, float cg, float cb, float ca,
    float u, float v_coord,
    float fd0, float fd1, float fd2, float fd3)
{
  sv->point.x = px;
  sv->point.y = py;
  sv->point.z = pz;
  sv->normal.x = nx;
  sv->normal.y = ny;
  sv->normal.z = nz;
  sv->color.r = cr;
  sv->color.g = cg;
  sv->color.b = cb;
  sv->color.a = ca;
  sv->uv[0] = u;
  sv->uv[1] = v_coord;
  sv->flow_data[0] = fd0;
  sv->flow_data[1] = fd1;
  sv->flow_data[2] = fd2;
  sv->flow_data[3] = fd3;
}

/*-----------------------------------------------------------------------*/

/** get_segment_color() - Determine color for a segment based on type and draw mode
 * @idx: segment index
 * @mode: current draw mode (geometry, currents, or charges)
 * @cmax: maximum magnitude for heat map scaling
 * @r: output red component
 * @g: output green component
 * @b: output blue component
 *
 * GEOMETRY mode uses get_segment_color_type() from draw.c;
 * CURRENTS/CHARGES mode uses heat map based on magnitude.
 */
  static void
get_segment_color(
    int idx,
    structure_draw_mode_t mode,
    double cmax,
    float *r, float *g, float *b)
{
  if( mode == STRUCTURE_DRAW_GEOMETRY )
  {
    /* Use shared color logic from draw.c (idx+1 for 1-indexed seg_num) */
    segment_color_type_t type = get_segment_color_type(idx + 1);
    segment_type_to_rgb(type, r, g, b);
    return;
  }

  /* Currents or charges mode: color by magnitude using heat map */
  if( cmax > 0.0 )
  {
    double mag = get_segment_magnitude(idx, mode);
    double red, grn, blu;

    Value_to_Color(&red, &grn, &blu, mag, cmax);
    *r = (float)red;
    *g = (float)grn;
    *b = (float)blu;
  }
  else
  {
    /* No data: default gray */
    *r = 0.5f;
    *g = 0.5f;
    *b = 0.5f;
  }
}

/*-----------------------------------------------------------------------*/

/** calculate_excitation_center() - Calculate the 3D center point of all excitation segments
 * @cx: output x coordinate of center
 * @cy: output y coordinate of center
 * @cz: output z coordinate of center
 *
 * Returns TRUE if at least one excitation segment was found.
 */
  static gboolean
calculate_excitation_center(double *cx, double *cy, double *cz)
{
  int idx, seg_idx;
  int count;
  double sum_x, sum_y, sum_z;

  sum_x = 0.0;
  sum_y = 0.0;
  sum_z = 0.0;
  count = 0;

  for( idx = 0; idx < vsorc.nsant; idx++ )
  {
    seg_idx = vsorc.isant[idx] - 1;
    if( seg_idx >= 0 && seg_idx < data.n )
    {
      sum_x += (data.x1[seg_idx] + data.x2[seg_idx]) / 2.0;
      sum_y += (data.y1[seg_idx] + data.y2[seg_idx]) / 2.0;
      sum_z += (data.z1[seg_idx] + data.z2[seg_idx]) / 2.0;
      count++;
    }
  }

  for( idx = 0; idx < vsorc.nvqd; idx++ )
  {
    seg_idx = vsorc.ivqd[idx] - 1;
    if( seg_idx >= 0 && seg_idx < data.n )
    {
      sum_x += (data.x1[seg_idx] + data.x2[seg_idx]) / 2.0;
      sum_y += (data.y1[seg_idx] + data.y2[seg_idx]) / 2.0;
      sum_z += (data.z1[seg_idx] + data.z2[seg_idx]) / 2.0;
      count++;
    }
  }

  if( count == 0 )
    return( FALSE );

  *cx = sum_x / (double)count;
  *cy = sum_y / (double)count;
  *cz = sum_z / (double)count;

  return( TRUE );

}

/*-----------------------------------------------------------------------*/

/** patch_uv_to_3d() - Convert UV coordinates on patch surface to 3D world position
 * @idx: patch index (0-based into data.m)
 * @s: half-side length of the patch (sqrt(pbi)/2)
 * @u: horizontal UV coordinate (0..1)
 * @v: vertical UV coordinate (0..1)
 * @out_x: output x position
 * @out_y: output y position
 * @out_z: output z position
 *
 * Maps UV in [0,1]^2 to world space via:
 *   P(u,v) = center + (2u-1)*s*t1 + (2v-1)*s*t2
 */
  static void
patch_uv_to_3d(int idx, double s, float u, float v,
    float *out_x, float *out_y, float *out_z)
{
  int j = idx + data.n;
  double du = 2.0 * (double)u - 1.0;
  double dv = 2.0 * (double)v - 1.0;

  /* Use save.xtemp/ytemp/ztemp (original unscaled coordinates) instead of
   * data.px/py/pz which are wavelength-scaled by Frequency_Scale_Geometry().
   * Wire endpoints (data.x1/y1/z1/x2/y2/z2) are not wavelength-scaled,
   * so patches must also use unscaled coords to stay dimensionally consistent. */
  *out_x = (float)(save.xtemp[j] + du * s * data.t1x[idx]
                                  + dv * s * data.t2x[idx]);
  *out_y = (float)(save.ytemp[j] + du * s * data.t1y[idx]
                                  + dv * s * data.t2y[idx]);
  *out_z = (float)(save.ztemp[j] + du * s * data.t1z[idx]
                                  + dv * s * data.t2z[idx]);
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_generate_geometry() - Generate cylinder geometry for antenna wire segments
 * @mode: draw mode (geometry, currents, or charges)
 * @cylinder_radius_scale: user-adjustable radius multiplier
 *
 * Returns vertex count written to structure_vertices.
 */
  static int
opengl_structure_generate_geometry(
    structure_draw_mode_t mode,
    double cylinder_radius_scale)
{
  int idx, vidx;
  int total_vertices;
  size_t mreq;
  double cmax;
  double r_max;
  float r, g, b;
  gboolean line_mode;

  if( data.n <= 0 && data.m <= 0 )
  {
    structure_vertex_count = 0;
    structure_view_scale = 1.0f;
    structure_draw_mode = GL_TRIANGLES;
    return( 0 );
  }

  line_mode = (cylinder_radius_scale < CYLINDER_SCALE_LINE_THRESHOLD);

  /* Vertex budget depends on rendering mode.
   * Patches: line mode = box(8) + arrow(14) = 22,
   * cylinder mode = 6 verts (2 triangles). */
  if( line_mode )
    total_vertices = data.n * 2 + data.m * 22;
  else
    total_vertices = data.n * opengl_cylinder_vertex_count(STRUCTURE_CYLINDER_SEGMENTS) + data.m * 6;

  mreq = (size_t)total_vertices * sizeof(structure_vertex_t);
  mem_realloc((void **)&structure_vertices, mreq, __LOCATION__);

  /* Find max magnitude for color scaling (wires + patch currents) */
  cmax = 0.0;
  if( (mode == STRUCTURE_DRAW_CURRENTS || mode == STRUCTURE_DRAW_CHARGES) && crnt.valid )
  {
    for( idx = 0; idx < data.n; idx++ )
    {
      double mag = get_segment_magnitude(idx, mode);
      if( mag > cmax )
        cmax = mag;
    }

    /* Patch current magnitudes (currents mode only; no patch charge data) */
    if( mode == STRUCTURE_DRAW_CURRENTS && crnt.cur != NULL )
    {
      for( idx = 0; idx < data.m; idx++ )
      {
        double mag = get_patch_current_magnitude(idx);

        if( mag > cmax )
          cmax = mag;
      }
    }
  }

  /* Find maximum distance from origin for scaling */
  r_max = 0.0;
  for( idx = 0; idx < data.n; idx++ )
  {
    double r1 = sqrt(
        data.x1[idx] * data.x1[idx] +
        data.y1[idx] * data.y1[idx] +
        data.z1[idx] * data.z1[idx]);
    double r2 = sqrt(
        data.x2[idx] * data.x2[idx] +
        data.y2[idx] * data.y2[idx] +
        data.z2[idx] * data.z2[idx]);

    if( r1 > r_max )
      r_max = r1;

    if( r2 > r_max )
      r_max = r2;
  }

  /* Extend r_max to include patch corners (unscaled coordinates) */
  for( idx = 0; idx < data.m; idx++ )
  {
    int j = idx + data.n;
    double s = sqrt(save.bitemp[j]) / 2.0;
    double dx1 = s * data.t1x[idx];
    double dy1 = s * data.t1y[idx];
    double dz1 = s * data.t1z[idx];
    double dx2 = s * data.t2x[idx];
    double dy2 = s * data.t2y[idx];
    double dz2 = s * data.t2z[idx];
    int corner;

    for( corner = 0; corner < 4; corner++ )
    {
      double sx1 = (corner & 1) ? 1.0 : -1.0;
      double sx2 = (corner & 2) ? 1.0 : -1.0;
      double cpx = save.xtemp[j] + sx1 * dx1 + sx2 * dx2;
      double cpy = save.ytemp[j] + sx1 * dy1 + sx2 * dy2;
      double cpz = save.ztemp[j] + sx1 * dz1 + sx2 * dz2;
      double rd = sqrt(cpx * cpx + cpy * cpy + cpz * cpz);

      if( rd > r_max )
        r_max = rd;
    }
  }

  if( r_max < 0.001 )
    r_max = 1.0;

  structure_view_scale = (float)r_max;

  vidx = 0;

  if( line_mode )
  {
    /* Line mode: 2 vertices per segment, drawn as GL_LINES */
    for( idx = 0; idx < data.n; idx++ )
    {
      double dx, dy, dz, len;
      float nx, ny, nz;

      get_segment_color(idx, mode, cmax, &r, &g, &b);

      /* Normal perpendicular to segment axis for consistent lighting.
       * Matches cylinder cross-section logic so lines shade like
       * the visible side of the cylinder they replace. */
      dx = data.x2[idx] - data.x1[idx];
      dy = data.y2[idx] - data.y1[idx];
      dz = data.z2[idx] - data.z1[idx];
      len = sqrt(dx * dx + dy * dy + dz * dz);

      if( len > 1e-10 )
      {
        double ax_d = dx / len;
        double ay_d = dy / len;
        double az_d = dz / len;
        double px, py, pz, pmag;

        /* Find perpendicular via cross with least-parallel basis vector */
        if( fabs(ax_d) < 0.9 )
        {
          px = 0.0;
          py = -az_d;
          pz = ay_d;
        }
        else
        {
          px = -az_d;
          py = 0.0;
          pz = ax_d;
        }

        pmag = sqrt(px * px + py * py + pz * pz);
        nx = (float)(px / pmag);
        ny = (float)(py / pmag);
        nz = (float)(pz / pmag);
      }
      else
      {
        nx = 0.0f;
        ny = 0.0f;
        nz = 1.0f;
      }

      /* Endpoint 1 */
      set_structure_vertex(&structure_vertices[vidx],
          (float)data.x1[idx], (float)data.y1[idx], (float)data.z1[idx],
          nx, ny, nz, r, g, b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;

      /* Endpoint 2 */
      set_structure_vertex(&structure_vertices[vidx],
          (float)data.x2[idx], (float)data.y2[idx], (float)data.z2[idx],
          nx, ny, nz, r, g, b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;
    }

    /* Patch outlines in line mode: box edges + interior fill */
    for( idx = 0; idx < data.m; idx++ )
    {
      int j = idx + data.n;
      double s = sqrt(save.bitemp[j]) / 2.0;
      float nx, ny, nz;
      float p_r, p_g, p_b;
      float c0x, c0y, c0z, c1x, c1y, c1z;
      float c2x, c2y, c2z, c3x, c3y, c3z;

      get_patch_normal(idx, &nx, &ny, &nz);
      get_patch_color(idx, mode, cmax, &p_r, &p_g, &p_b);

      /* Quad corners: c0(+t1,+t2) c1(-t1,+t2) c2(-t1,-t2) c3(+t1,-t2) */
      c0x = (float)(save.xtemp[j] + s * data.t1x[idx] + s * data.t2x[idx]);
      c0y = (float)(save.ytemp[j] + s * data.t1y[idx] + s * data.t2y[idx]);
      c0z = (float)(save.ztemp[j] + s * data.t1z[idx] + s * data.t2z[idx]);
      c1x = (float)(save.xtemp[j] - s * data.t1x[idx] + s * data.t2x[idx]);
      c1y = (float)(save.ytemp[j] - s * data.t1y[idx] + s * data.t2y[idx]);
      c1z = (float)(save.ztemp[j] - s * data.t1z[idx] + s * data.t2z[idx]);
      c2x = (float)(save.xtemp[j] - s * data.t1x[idx] - s * data.t2x[idx]);
      c2y = (float)(save.ytemp[j] - s * data.t1y[idx] - s * data.t2y[idx]);
      c2z = (float)(save.ztemp[j] - s * data.t1z[idx] - s * data.t2z[idx]);
      c3x = (float)(save.xtemp[j] + s * data.t1x[idx] - s * data.t2x[idx]);
      c3y = (float)(save.ytemp[j] + s * data.t1y[idx] - s * data.t2y[idx]);
      c3z = (float)(save.ztemp[j] + s * data.t1z[idx] - s * data.t2z[idx]);

      /* Box outline: 4 edges as GL_LINES pairs (8 vertices) */
      set_structure_vertex(&structure_vertices[vidx],
          c0x, c0y, c0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c1x, c1y, c1z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;

      set_structure_vertex(&structure_vertices[vidx],
          c1x, c1y, c1z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c2x, c2y, c2z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;

      set_structure_vertex(&structure_vertices[vidx],
          c2x, c2y, c2z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c3x, c3y, c3z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;

      set_structure_vertex(&structure_vertices[vidx],
          c3x, c3y, c3z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c0x, c0y, c0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vidx++;

      /* Interior fill: chevron V-marks or stipple dots */
      {
        gboolean use_chevrons = FALSE;
        float fd[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float mag_ratio = 0.0f;

        if( mode == STRUCTURE_DRAW_CURRENTS )
        {
          get_patch_flow_data(idx, mode, cmax, fd);

          /* Recover mag_ratio from scaled components for threshold check */
          mag_ratio = (float)sqrt(
              fd[0] * fd[0] + fd[1] * fd[1] +
              fd[2] * fd[2] + fd[3] * fd[3]);

          if( mag_ratio > 0.01f )
            use_chevrons = TRUE;
        }

        if( use_chevrons )
        {
          /* Arrow only rendered when current data is active and above threshold */
          /* Bold arrow: rectangle shaft + wide arrowhead (7 lines, 14 vertices)
           *
           *   s0────────s1
           *   │     ┌────a_left
           *   │     │   ╱
           *   │     │  tip
           *   │     │   ╲
           *   │     └────a_right
           *   s3────────s2
           */
          /* Derive arrow angle from complex tangent projections.
           * Mode dispatch selects the appropriate angle computation. */
          float flow_angle;
          int fdir_mode = rc_config.opengl_flow_direction_mode;

          if( fdir_mode == FLOW_DIR_POLARIZATION_TILT )
          {
            /* Tilt axis: psi = 0.5 * atan2(2*Re(ct1*conj(ct2)), |ct1|^2 - |ct2|^2) */
            float ct1_sq = fd[0] * fd[0] + fd[1] * fd[1];
            float ct2_sq = fd[2] * fd[2] + fd[3] * fd[3];
            float cross = fd[0] * fd[2] + fd[1] * fd[3];
            flow_angle = 0.5f * atan2f(2.0f * cross, ct1_sq - ct2_sq);
          }
          else if( fdir_mode == FLOW_DIR_PEAK_MAGNITUDE )
          {
            /* Peak instant: P = ct1^2 + ct2^2, alpha0 = -0.5*arg(P) */
            float p_re = (fd[0] * fd[0] - fd[1] * fd[1])
                       + (fd[2] * fd[2] - fd[3] * fd[3]);
            float p_im = 2.0f * (fd[0] * fd[1] + fd[2] * fd[3]);
            float alpha0 = -0.5f * atan2f(p_im, p_re);
            float ca0 = cosf(alpha0);
            float sa0 = sinf(alpha0);
            flow_angle = atan2f(fd[2] * ca0 - fd[3] * sa0,
                                fd[0] * ca0 - fd[1] * sa0);
          }
          else
          {
            /* Reference phase (Mode 0): real parts at current flow phase.
             * Static when not animating (phase=0); rotates when animating. */
            float cp = cosf(structure_flow_phase);
            float sp = sinf(structure_flow_phase);
            flow_angle = atan2f(fd[2] * cp - fd[3] * sp,
                                fd[0] * cp - fd[1] * sp);
          }

          float fca = cosf(flow_angle);
          float fsa = sinf(flow_angle);

          /* Polarization tilt: bidirectional shaft, no arrowhead */
          if( fdir_mode == FLOW_DIR_POLARIZATION_TILT )
          {
            float e0u, e0v, e1u, e1v;
            float e0x, e0y, e0z, e1x, e1y, e1z;

            e0u = 0.5f - LINE_ARROW_TIP_OFF * fca;
            e0v = 0.5f - LINE_ARROW_TIP_OFF * fsa;
            e1u = 0.5f + LINE_ARROW_TIP_OFF * fca;
            e1v = 0.5f + LINE_ARROW_TIP_OFF * fsa;

            patch_uv_to_3d(idx, s, e0u, e0v, &e0x, &e0y, &e0z);
            patch_uv_to_3d(idx, s, e1u, e1v, &e1x, &e1y, &e1z);

            /* Double-ended shaft: single line through center */
            set_structure_vertex(&structure_vertices[vidx],
                e0x, e0y, e0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vidx++;
            set_structure_vertex(&structure_vertices[vidx],
                e1x, e1y, e1z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vidx++;
          }
          else
          {
          /* Unidirectional arrow: shaft + arrowhead.
           * Modes 0 (reference phase) and 3 (LIC) both use this path;
           * LIC is a fragment-shader-only effect with no line-mode equivalent. */
          float pu = -fsa;
          float pv = fca;
          float s0u, s0v, s1u, s1v, s2u, s2v, s3u, s3v;
          float alu, alv, aru, arv, tipu, tipv;
          float s0x, s0y, s0z, s1x, s1y, s1z;
          float s2x, s2y, s2z, s3x, s3y, s3z;
          float alx, aly, alz, arx, ary, arz;
          float tipx, tipy, tipz;

          /* Shaft corners in UV space */
          s0u = 0.5f + LINE_ARROW_TAIL_OFF * fca + LINE_ARROW_SHAFT_HW * pu;
          s0v = 0.5f + LINE_ARROW_TAIL_OFF * fsa + LINE_ARROW_SHAFT_HW * pv;
          s1u = 0.5f + LINE_ARROW_NECK_OFF * fca + LINE_ARROW_SHAFT_HW * pu;
          s1v = 0.5f + LINE_ARROW_NECK_OFF * fsa + LINE_ARROW_SHAFT_HW * pv;
          s2u = 0.5f + LINE_ARROW_NECK_OFF * fca - LINE_ARROW_SHAFT_HW * pu;
          s2v = 0.5f + LINE_ARROW_NECK_OFF * fsa - LINE_ARROW_SHAFT_HW * pv;
          s3u = 0.5f + LINE_ARROW_TAIL_OFF * fca - LINE_ARROW_SHAFT_HW * pu;
          s3v = 0.5f + LINE_ARROW_TAIL_OFF * fsa - LINE_ARROW_SHAFT_HW * pv;

          /* Arrowhead points in UV space */
          alu = 0.5f + LINE_ARROW_NECK_OFF * fca + LINE_ARROW_HEAD_HS * pu;
          alv = 0.5f + LINE_ARROW_NECK_OFF * fsa + LINE_ARROW_HEAD_HS * pv;
          aru = 0.5f + LINE_ARROW_NECK_OFF * fca - LINE_ARROW_HEAD_HS * pu;
          arv = 0.5f + LINE_ARROW_NECK_OFF * fsa - LINE_ARROW_HEAD_HS * pv;
          tipu = 0.5f + LINE_ARROW_TIP_OFF * fca;
          tipv = 0.5f + LINE_ARROW_TIP_OFF * fsa;

          /* Convert all points to 3D */
          patch_uv_to_3d(idx, s, s0u, s0v, &s0x, &s0y, &s0z);
          patch_uv_to_3d(idx, s, s1u, s1v, &s1x, &s1y, &s1z);
          patch_uv_to_3d(idx, s, s2u, s2v, &s2x, &s2y, &s2z);
          patch_uv_to_3d(idx, s, s3u, s3v, &s3x, &s3y, &s3z);
          patch_uv_to_3d(idx, s, alu, alv, &alx, &aly, &alz);
          patch_uv_to_3d(idx, s, aru, arv, &arx, &ary, &arz);
          patch_uv_to_3d(idx, s, tipu, tipv, &tipx, &tipy, &tipz);

          /* Shaft top edge: s0 to s1 */
          set_structure_vertex(&structure_vertices[vidx],
              s0x, s0y, s0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              s1x, s1y, s1z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;

          /* Shaft bottom edge: s3 to s2 */
          set_structure_vertex(&structure_vertices[vidx],
              s3x, s3y, s3z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              s2x, s2y, s2z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;

          /* Tail cap: s0 to s3 */
          set_structure_vertex(&structure_vertices[vidx],
              s0x, s0y, s0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              s3x, s3y, s3z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;

          /* Top notch: s1 to a_left */
          set_structure_vertex(&structure_vertices[vidx],
              s1x, s1y, s1z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              alx, aly, alz, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;

          /* Top arm: a_left to tip */
          set_structure_vertex(&structure_vertices[vidx],
              alx, aly, alz, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              tipx, tipy, tipz, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;

          /* Bottom notch: s2 to a_right */
          set_structure_vertex(&structure_vertices[vidx],
              s2x, s2y, s2z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              arx, ary, arz, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;

          /* Bottom arm: a_right to tip */
          set_structure_vertex(&structure_vertices[vidx],
              arx, ary, arz, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          set_structure_vertex(&structure_vertices[vidx],
              tipx, tipy, tipz, nx, ny, nz, p_r, p_g, p_b, 1.0f,
              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
          vidx++;
          } /* end unidirectional arrow else block */
        }
      }
    }

    structure_draw_mode = GL_LINES;
  }
  else
  {
    /* Cylinder mode: full 3D cylinders with minimum visible radius.
     * Scale minimum proportionally so ctrl+scroll affects zero-radius wires too. */
    double scale_factor = cylinder_radius_scale / CYLINDER_RADIUS_SCALE_DEFAULT;
    double min_visible = CYLINDER_MIN_VISIBLE_FRACTION * r_max * scale_factor;

    /* Temp buffer for one cylinder's vertices (segments × 12) */
    int cyl_vcount = opengl_cylinder_vertex_count(STRUCTURE_CYLINDER_SEGMENTS);
    lit_color_point_t cyl_temp[STRUCTURE_CYLINDER_SEGMENTS * 12];

    for( idx = 0; idx < data.n; idx++ )
    {
      lit_cylinder_mesh_t mesh;
      double radius;
      int cyl_vidx, j;

      mesh.vertices = cyl_temp;
      mesh.vertex_count = cyl_vcount;

      get_segment_color(idx, mode, cmax, &r, &g, &b);

      /* Scale radius with fabs for safety, clamp to minimum visible */
      radius = fabs(data.bi[idx]) * cylinder_radius_scale;

      if( radius < min_visible )
      {
        radius = min_visible;
      }

      {
        point_f_3d_t seg_p1 = {(float)data.x1[idx], (float)data.y1[idx], (float)data.z1[idx]};
        point_f_3d_t seg_p2 = {(float)data.x2[idx], (float)data.y2[idx], (float)data.z2[idx]};
        rgba_f_t seg_color = {r, g, b, 1.0f};

        cyl_vidx = opengl_lit_cylinder_append(&mesh, 0,
            &seg_p1, &seg_p2, radius, STRUCTURE_CYLINDER_SEGMENTS, &seg_color);
      }

      /* Expand lit_color_point_t -> structure_vertex_t */
      for( j = 0; j < cyl_vidx; j++ )
      {
        structure_vertices[vidx].point  = cyl_temp[j].point;
        structure_vertices[vidx].normal = cyl_temp[j].normal;
        structure_vertices[vidx].color  = cyl_temp[j].color;
        structure_vertices[vidx].uv[0] = 0.0f;
        structure_vertices[vidx].uv[1] = 0.0f;
        structure_vertices[vidx].flow_data[0] = 0.0f;
        structure_vertices[vidx].flow_data[1] = 0.0f;
        structure_vertices[vidx].flow_data[2] = 0.0f;
        structure_vertices[vidx].flow_data[3] = 0.0f;
        vidx++;
      }
    }

    /* Patch quads in cylinder mode: 2 triangles per patch with UV + flow data */
    for( idx = 0; idx < data.m; idx++ )
    {
      int j = idx + data.n;
      double s = sqrt(save.bitemp[j]) / 2.0;
      float nx, ny, nz;
      float p_r, p_g, p_b;
      float fd[4];
      float c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z, c3x, c3y, c3z;

      get_patch_normal(idx, &nx, &ny, &nz);

      /* Quad corners: c0(+t1,+t2) c1(-t1,+t2) c2(-t1,-t2) c3(+t1,-t2) */
      c0x = (float)(save.xtemp[j] + s * data.t1x[idx] + s * data.t2x[idx]);
      c0y = (float)(save.ytemp[j] + s * data.t1y[idx] + s * data.t2y[idx]);
      c0z = (float)(save.ztemp[j] + s * data.t1z[idx] + s * data.t2z[idx]);
      c1x = (float)(save.xtemp[j] - s * data.t1x[idx] + s * data.t2x[idx]);
      c1y = (float)(save.ytemp[j] - s * data.t1y[idx] + s * data.t2y[idx]);
      c1z = (float)(save.ztemp[j] - s * data.t1z[idx] + s * data.t2z[idx]);
      c2x = (float)(save.xtemp[j] - s * data.t1x[idx] - s * data.t2x[idx]);
      c2y = (float)(save.ytemp[j] - s * data.t1y[idx] - s * data.t2y[idx]);
      c2z = (float)(save.ztemp[j] - s * data.t1z[idx] - s * data.t2z[idx]);
      c3x = (float)(save.xtemp[j] + s * data.t1x[idx] - s * data.t2x[idx]);
      c3y = (float)(save.ytemp[j] + s * data.t1y[idx] - s * data.t2y[idx]);
      c3z = (float)(save.ztemp[j] + s * data.t1z[idx] - s * data.t2z[idx]);

      get_patch_color(idx, mode, cmax, &p_r, &p_g, &p_b);
      get_patch_flow_data(idx, mode, cmax, fd);

      /* Triangle 1: c0(1,1), c1(0,1), c2(0,0) */
      set_structure_vertex(&structure_vertices[vidx],
          c0x, c0y, c0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          1.0f, 1.0f, fd[0], fd[1], fd[2], fd[3]);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c1x, c1y, c1z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 1.0f, fd[0], fd[1], fd[2], fd[3]);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c2x, c2y, c2z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, fd[0], fd[1], fd[2], fd[3]);
      vidx++;

      /* Triangle 2: c0(1,1), c2(0,0), c3(1,0) */
      set_structure_vertex(&structure_vertices[vidx],
          c0x, c0y, c0z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          1.0f, 1.0f, fd[0], fd[1], fd[2], fd[3]);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c2x, c2y, c2z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          0.0f, 0.0f, fd[0], fd[1], fd[2], fd[3]);
      vidx++;
      set_structure_vertex(&structure_vertices[vidx],
          c3x, c3y, c3z, nx, ny, nz, p_r, p_g, p_b, 1.0f,
          1.0f, 0.0f, fd[0], fd[1], fd[2], fd[3]);
      vidx++;
    }

    structure_draw_mode = GL_TRIANGLES;
  }

  structure_vertex_count = vidx;
  structure_last_radius_scale = cylinder_radius_scale;
  structure_geometry_generation++;

  return( structure_vertex_count );
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_update_shared_geometry() - Check staleness of shared geometry and regenerate if needed
 *
 * Called by both structure window and rdpattern overlay before rendering.
 */
  void
opengl_structure_update_shared_geometry(void)
{
  structure_draw_mode_t current_mode;
  double cylinder_radius_scale;

  current_mode = opengl_structure_get_draw_mode();
  cylinder_radius_scale = opengl_structure_get_radius_scale();

  /* Check if current mode requires current/charge data */
  gboolean is_current_mode =
    (current_mode == STRUCTURE_DRAW_CURRENTS ||
     current_mode == STRUCTURE_DRAW_CHARGES);

  /* Regenerate on mode change, empty buffer, or new current data */
  if( current_mode != structure_last_mode ||
      structure_vertex_count == 0 ||
      cylinder_radius_scale != structure_last_radius_scale ||
      (is_current_mode && crnt.newer) )
  {
    structure_last_mode = current_mode;
    opengl_structure_generate_geometry(current_mode, cylinder_radius_scale);

    /* Prevent redundant regeneration on subsequent expose events */
    if( crnt.newer && is_current_mode )
      crnt.newer = 0;

    /* Clear redraw flag for SUPPRESS_INTERMEDIATE_REDRAWS guard */
    need_structure_redraw = 0;

    /* Update shared overlay data after regeneration */
    shared_overlay_data.vertices = structure_vertices;
    shared_overlay_data.vertex_count = structure_vertex_count;
    shared_overlay_data.vertex_stride = (int)sizeof(structure_vertex_t);
    shared_overlay_data.view_scale = structure_view_scale;
    shared_overlay_data.draw_mode = structure_draw_mode;
    shared_overlay_data.generation = structure_geometry_generation;
    shared_overlay_data.has_excitation_center =
        calculate_excitation_center(
            &shared_overlay_data.excitation_center_x,
            &shared_overlay_data.excitation_center_y,
            &shared_overlay_data.excitation_center_z);
  }
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_get_shared_geometry() - Return read-only pointer to shared structure geometry data
 */
  const structure_overlay_data_t*
opengl_structure_get_shared_geometry(void)
{
  return( &shared_overlay_data );
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_geometry_cleanup() - Free vertex buffer and reset geometry state
 */
  void
opengl_structure_geometry_cleanup(void)
{
  free_ptr((void **)&structure_vertices);
  structure_vertex_count = 0;
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_geometry_invalidate() - Mark cached geometry stale so next render regenerates from NEC2 data
 */
  void
opengl_structure_geometry_invalidate(void)
{
  structure_vertex_count = 0;
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_geometry_set_flow_phase() - Set flow phase for line-mode arrow angle computation
 * @phase: phase offset in radians
 *
 * Called by Animate_Flow_Phase before invalidating geometry.
 * Triangle-mode patches use the shader u_phase uniform instead.
 */
  void
opengl_structure_geometry_set_flow_phase(float phase)
{
  structure_flow_phase = phase;
}

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
