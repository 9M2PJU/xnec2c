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
#include "../opengl-engine/opengl_view.h"

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

/* Arrow template: 7 line segments (14 UV endpoints) at angle=0 (pointing +U).
 *   s0────────s1
 *   │     ┌────a_left
 *   │     │   ╱
 *   │     │  tip
 *   │     │   ╲
 *   │     └────a_right
 *   s3────────s2
 */
#define ARROW_S0U (0.5f + LINE_ARROW_TAIL_OFF)
#define ARROW_S0V (0.5f + LINE_ARROW_SHAFT_HW)
#define ARROW_S1U (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_S1V (0.5f + LINE_ARROW_SHAFT_HW)
#define ARROW_S2U (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_S2V (0.5f - LINE_ARROW_SHAFT_HW)
#define ARROW_S3U (0.5f + LINE_ARROW_TAIL_OFF)
#define ARROW_S3V (0.5f - LINE_ARROW_SHAFT_HW)
#define ARROW_ALU (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_ALV (0.5f + LINE_ARROW_HEAD_HS)
#define ARROW_ARU (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_ARV (0.5f - LINE_ARROW_HEAD_HS)
#define ARROW_TPU (0.5f + LINE_ARROW_TIP_OFF)
#define ARROW_TPV 0.5f

#define ARROW_LINE_COUNT   7
#define ARROW_VERTEX_COUNT 14

static const float arrow_template_uvs[ARROW_VERTEX_COUNT][2] = {
  { ARROW_S0U, ARROW_S0V }, { ARROW_S1U, ARROW_S1V },  /* shaft top */
  { ARROW_S3U, ARROW_S3V }, { ARROW_S2U, ARROW_S2V },  /* shaft bottom */
  { ARROW_S0U, ARROW_S0V }, { ARROW_S3U, ARROW_S3V },  /* tail cap */
  { ARROW_S1U, ARROW_S1V }, { ARROW_ALU, ARROW_ALV },   /* top notch */
  { ARROW_ALU, ARROW_ALV }, { ARROW_TPU, ARROW_TPV },   /* top arm */
  { ARROW_S2U, ARROW_S2V }, { ARROW_ARU, ARROW_ARV },   /* bottom notch */
  { ARROW_ARU, ARROW_ARV }, { ARROW_TPU, ARROW_TPV },   /* bottom arm */
};



/* Per-type draw batches: each batch owns its own vertex allocation and GL mode */
static gl_draw_batch_t batches[GL_VIEW_MAX_BATCHES];
static int batch_count = 0;
static unsigned int structure_geometry_generation = 1;
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;
static float structure_view_scale = 1.0f;

/* Track previous radius scale to detect changes requiring regeneration */
static double structure_last_radius_scale = CYLINDER_RADIUS_SCALE_DEFAULT;


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
  int fstep = calc_data.freq_step;
  if( idx < 0 || !CRNT_FSTEP_AVAILABLE(fstep) )
    return( 0.0 );

  crnt_t *cf = &crnt_fstep[fstep];

  if( mode == STRUCTURE_DRAW_CURRENTS )
    return( cabs(cf->cur[idx]) );

  if( mode == STRUCTURE_DRAW_CHARGES )
    return( cabs(cmplx(cf->bir[idx], cf->bii[idx])) );

  return( 0.0 );
}

/*-----------------------------------------------------------------------*/

/** get_patch_tangent_projections() - Project patch current onto tangent axes as complex phasors
 * @idx: patch index (0-based into data.m)
 * @ct1: output complex projection onto t1
 * @ct2: output complex projection onto t2
 *
 * Preserves phase information needed for polarization analysis.
 * Caller must ensure crnt_fstep[fstep] is valid and idx is in range.
 */
  static void
get_patch_tangent_projections(int idx,
    complex double *ct1, complex double *ct2)
{
  int fstep = calc_data.freq_step;
  crnt_t *cf = &crnt_fstep[fstep];
  int ci = data.n + idx * 3;
  complex double cur_x = cf->cur[ci];
  complex double cur_y = cf->cur[ci + 1];
  complex double cur_z = cf->cur[ci + 2];

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
  int fstep = calc_data.freq_step;
  if( mode == STRUCTURE_DRAW_CURRENTS && cmax > 0.0
      && CRNT_FSTEP_AVAILABLE(fstep) )
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
  int fstep = calc_data.freq_step;
  if( mode == STRUCTURE_DRAW_CURRENTS && cmax > 0.0
      && CRNT_FSTEP_AVAILABLE(fstep) )
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

/* Vertices are initialized via C99 compound literal assignment:
 *   verts[vidx++] = (structure_vertex_t){ .point = {…}, .normal = {…}, … };
 * Omitted fields default to zero (tangent1/tangent2 for flat vertices). */

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

/** generate_segments_lines() - Emit GL_LINES vertices for wire segments
 * @batch: draw batch with pre-allocated vertices buffer
 * @mode: draw mode (geometry, currents, or charges)
 * @cmax: maximum current/charge magnitude for color scaling
 *
 * Populates batch with 2 vertices per segment. Sets vertex_count.
 */
  static void
generate_segments_lines(gl_draw_batch_t *batch, structure_draw_mode_t mode, double cmax)
{
  int idx, vidx = 0;
  float r, g, b;
  structure_vertex_t *verts = (structure_vertex_t *)batch->vertices;

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
    verts[vidx++] = (structure_vertex_t){
        .point  = {(float)data.x1[idx], (float)data.y1[idx], (float)data.z1[idx]},
        .normal = {nx, ny, nz},
        .color  = {r, g, b, 1.0f},
    };

    /* Endpoint 2 */
    verts[vidx++] = (structure_vertex_t){
        .point  = {(float)data.x2[idx], (float)data.y2[idx], (float)data.z2[idx]},
        .normal = {nx, ny, nz},
        .color  = {r, g, b, 1.0f},
    };
  }

  batch->vertex_count = vidx;
}

/*-----------------------------------------------------------------------*/

/** generate_patches_wireframe() - Emit GL_LINES vertices for patch outlines and arrows
 * @batch: draw batch with pre-allocated vertices buffer
 * @mode: draw mode (geometry, currents, or charges)
 * @cmax: maximum current/charge magnitude for color scaling
 *
 * Emits box outline (8 vertices) per patch.  When mode is STRUCTURE_DRAW_CURRENTS
 * and magnitude exceeds threshold, also emits directional arrow (up to 14 vertices).
 * Sets batch vertex_count.
 */
  static void
generate_patches_wireframe(gl_draw_batch_t *batch, structure_draw_mode_t mode, double cmax)
{
  int idx, vidx = 0;
  structure_vertex_t *verts = (structure_vertex_t *)batch->vertices;

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
    verts[vidx++] = (structure_vertex_t){
        .point = {c0x, c0y, c0z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c1x, c1y, c1z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };

    verts[vidx++] = (structure_vertex_t){
        .point = {c1x, c1y, c1z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c2x, c2y, c2z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };

    verts[vidx++] = (structure_vertex_t){
        .point = {c2x, c2y, c2z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c3x, c3y, c3z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };

    verts[vidx++] = (structure_vertex_t){
        .point = {c3x, c3y, c3z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c0x, c0y, c0z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
    };

    /* GPU-driven arrow: store template UV + tangent frame + flow data.
     * The vertex shader rotates template UVs by the phase-derived flow
     * angle, then transforms to world space via the tangent frame.
     * Arrow only rendered when current data is active and above threshold. */
    {
      gboolean emit_arrow = FALSE;
      float fd[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

      if( mode == STRUCTURE_DRAW_CURRENTS )
      {
        float mag_ratio;

        get_patch_flow_data(idx, mode, cmax, fd);

        mag_ratio = (float)sqrt(
            fd[0] * fd[0] + fd[1] * fd[1] +
            fd[2] * fd[2] + fd[3] * fd[3]);

        if( mag_ratio > 0.01f )
          emit_arrow = TRUE;
      }

      if( emit_arrow )
      {
        float acx = (float)save.xtemp[j];
        float acy = (float)save.ytemp[j];
        float acz = (float)save.ztemp[j];
        float st1x = (float)(s * data.t1x[idx]);
        float st1y = (float)(s * data.t1y[idx]);
        float st1z = (float)(s * data.t1z[idx]);
        float st2x = (float)(s * data.t2x[idx]);
        float st2y = (float)(s * data.t2y[idx]);
        float st2z = (float)(s * data.t2z[idx]);
        int k;

        for( k = 0; k < ARROW_VERTEX_COUNT; k++ )
        {
          verts[vidx++] = (structure_vertex_t){
              .point = {acx, acy, acz}, .normal = {nx, ny, nz},
              .color = {p_r, p_g, p_b, 1.0f},
              .uv = {arrow_template_uvs[k][0], arrow_template_uvs[k][1]},
              .flow_data = {fd[0], fd[1], fd[2], fd[3]},
              .tangent1 = {st1x, st1y, st1z},
              .tangent2 = {st2x, st2y, st2z},
          };
        }
      }
    }
  }

  batch->vertex_count = vidx;
}

/*-----------------------------------------------------------------------*/

/** generate_segments_cylinders() - Emit GL_TRIANGLES vertices for cylinder wire segments
 * @batch: draw batch with pre-allocated vertices buffer
 * @mode: draw mode (geometry, currents, or charges)
 * @cmax: maximum current/charge magnitude for color scaling
 * @cylinder_radius_scale: user-adjustable radius multiplier
 * @r_max: maximum model extent for minimum-visible scaling
 *
 * Populates batch with cylinder vertices per segment. Sets vertex_count.
 */
  static void
generate_segments_cylinders(gl_draw_batch_t *batch, structure_draw_mode_t mode, double cmax,
    double cylinder_radius_scale, double r_max)
{
  int idx, vidx = 0;
  float r, g, b;
  structure_vertex_t *verts = (structure_vertex_t *)batch->vertices;

  /* Scale minimum proportionally so ctrl+scroll affects zero-radius wires too */
  double scale_factor = cylinder_radius_scale / CYLINDER_RADIUS_SCALE_DEFAULT;
  double min_visible = CYLINDER_MIN_VISIBLE_FRACTION * r_max * scale_factor;

  /* Temp buffer for one cylinder's vertices (segments x 12) */
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

    /* Expand lit_color_point_t -> structure_vertex_t (zero extended fields) */
    for( j = 0; j < cyl_vidx; j++ )
    {
      memset(&verts[vidx], 0, sizeof(verts[vidx]));
      verts[vidx].point  = cyl_temp[j].point;
      verts[vidx].normal = cyl_temp[j].normal;
      verts[vidx].color  = cyl_temp[j].color;
      vidx++;
    }
  }

  batch->vertex_count = vidx;
}

/*-----------------------------------------------------------------------*/

/** generate_patches_triangles() - Emit GL_TRIANGLES vertices for filled patch quads
 * @batch: draw batch with pre-allocated vertices buffer
 * @mode: draw mode (geometry, currents, or charges)
 * @cmax: maximum current/charge magnitude for color scaling
 *
 * Emits 6 vertices (2 triangles) per patch with UV and flow data.
 * Sets batch vertex_count.
 */
  static void
generate_patches_triangles(gl_draw_batch_t *batch, structure_draw_mode_t mode, double cmax)
{
  int idx, vidx = 0;
  structure_vertex_t *verts = (structure_vertex_t *)batch->vertices;

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
    verts[vidx++] = (structure_vertex_t){
        .point = {c0x, c0y, c0z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
        .uv = {1.0f, 1.0f}, .flow_data = {fd[0], fd[1], fd[2], fd[3]},

    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c1x, c1y, c1z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
        .uv = {0.0f, 1.0f}, .flow_data = {fd[0], fd[1], fd[2], fd[3]},

    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c2x, c2y, c2z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
        .uv = {0.0f, 0.0f}, .flow_data = {fd[0], fd[1], fd[2], fd[3]},

    };

    /* Triangle 2: c0(1,1), c2(0,0), c3(1,0) */
    verts[vidx++] = (structure_vertex_t){
        .point = {c0x, c0y, c0z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
        .uv = {1.0f, 1.0f}, .flow_data = {fd[0], fd[1], fd[2], fd[3]},

    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c2x, c2y, c2z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
        .uv = {0.0f, 0.0f}, .flow_data = {fd[0], fd[1], fd[2], fd[3]},

    };
    verts[vidx++] = (structure_vertex_t){
        .point = {c3x, c3y, c3z}, .normal = {nx, ny, nz},
        .color = {p_r, p_g, p_b, 1.0f},
        .uv = {1.0f, 0.0f}, .flow_data = {fd[0], fd[1], fd[2], fd[3]},

    };
  }

  batch->vertex_count = vidx;
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_generate_geometry() - Generate geometry for antenna wire segments and patches
 * @mode: draw mode (geometry, currents, or charges)
 * @cylinder_radius_scale: user-adjustable radius multiplier
 *
 * Populates batches[0] (segments) and batches[1] (patches) with independent
 * vertex allocations and GL draw modes.
 */
  static void
opengl_structure_generate_geometry(
    structure_draw_mode_t mode,
    double cylinder_radius_scale)
{
  int idx;
  int seg_verts, patch_verts;
  double cmax;
  double r_max;
  gboolean seg_line_mode, patch_wireframe;

  if( data.n <= 0 && data.m <= 0 )
  {
    batch_count = 0;
    structure_view_scale = 1.0f;
    return;
  }

  seg_line_mode = (cylinder_radius_scale < CYLINDER_SCALE_LINE_THRESHOLD);
  patch_wireframe = (rc_config.current_flow_visualization_mode == FLOW_DIR_WIREFRAME);

  /* Per-batch vertex budgets */
  seg_verts = seg_line_mode
    ? data.n * 2
    : data.n * opengl_cylinder_vertex_count(STRUCTURE_CYLINDER_SEGMENTS);
  patch_verts = patch_wireframe
    ? data.m * 22
    : data.m * 6;

  /* Allocate each batch independently */
  mem_realloc((void **)&batches[0].vertices,
      (size_t)seg_verts * sizeof(structure_vertex_t), __LOCATION__);

  if( data.m > 0 )
    mem_realloc((void **)&batches[1].vertices,
        (size_t)patch_verts * sizeof(structure_vertex_t), __LOCATION__);

  /* Find max magnitude for color scaling (wires + patch currents) */
  cmax = 0.0;
  int fstep = calc_data.freq_step;
  if( (mode == STRUCTURE_DRAW_CURRENTS || mode == STRUCTURE_DRAW_CHARGES)
      && CRNT_FSTEP_AVAILABLE(fstep) )
  {
    for( idx = 0; idx < data.n; idx++ )
    {
      double mag = get_segment_magnitude(idx, mode);
      if( mag > cmax )
        cmax = mag;
    }

    /* Patch current magnitudes (currents mode only; no patch charge data) */
    if( mode == STRUCTURE_DRAW_CURRENTS )
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

  /* Generate segment batch */
  if( seg_line_mode )
    generate_segments_lines(&batches[0], mode, cmax);
  else
    generate_segments_cylinders(&batches[0], mode, cmax,
        cylinder_radius_scale, r_max);

  batches[0].draw_mode = seg_line_mode ? GL_LINES : GL_TRIANGLES;

  /* Generate patch batch when patches exist */
  if( data.m > 0 )
  {
    if( patch_wireframe )
      generate_patches_wireframe(&batches[1], mode, cmax);
    else
      generate_patches_triangles(&batches[1], mode, cmax);

    batches[1].draw_mode = patch_wireframe ? GL_LINES : GL_TRIANGLES;
    batches[1].polygon_offset = TRUE;
  }

  batch_count = (data.m > 0) ? GL_VIEW_MAX_BATCHES : 1;

  structure_last_radius_scale = cylinder_radius_scale;
  structure_geometry_generation++;
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

  static int last_fstep = -1;

  /* Track freq_mhz separately: freq_step stays at steps_total for all
   * left-click (arbitrary) frequencies, so step index alone is not sufficient
   * to detect data changes in the extra slot. */
  static double last_freq_mhz = -1.0;

  gboolean extra_slot_changed =
    (calc_data.freq_step == calc_data.steps_total &&
     calc_data.freq_mhz != last_freq_mhz);

  /* Regenerate on mode change, empty buffer, or new current data */
  if( current_mode != structure_last_mode ||
      batch_count == 0 ||
      cylinder_radius_scale != structure_last_radius_scale ||
      (is_current_mode && (calc_data.freq_step != last_fstep || extra_slot_changed)) )
  {
    structure_last_mode = current_mode;
    opengl_structure_generate_geometry(current_mode, cylinder_radius_scale);

    /* Prevent redundant regeneration on subsequent expose events */
    if( is_current_mode )
    {
      last_fstep = calc_data.freq_step;
      last_freq_mhz = calc_data.freq_mhz;
    }

    /* Update shared overlay data after regeneration */
    memcpy(shared_overlay_data.batches, batches,
        (size_t)batch_count * sizeof(batches[0]));
    shared_overlay_data.batch_count = batch_count;
    shared_overlay_data.vertex_stride = (int)sizeof(structure_vertex_t);
    shared_overlay_data.view_scale = structure_view_scale;
    shared_overlay_data.generation = structure_geometry_generation;
    shared_overlay_data.has_excitation_center =
        calculate_excitation_center(
            &shared_overlay_data.excitation_center_x,
            &shared_overlay_data.excitation_center_y,
            &shared_overlay_data.excitation_center_z);
  }

  /* Per-batch visual parameters read from rc_config every frame.
   * Set unconditionally so brightness/transparency slider changes
   * take effect without geometry regeneration. */
  batches[0].color_dim = rc_config.brightness_segments;
  batches[0].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_segments);
  if( batch_count > 1 )
  {
    batches[1].color_dim = rc_config.brightness_patches;
    batches[1].alpha = TRANSPARENCY_TO_ALPHA(rc_config.transparency_patches);
  }

  /* Propagate visual params to overlay (cheap field copy) */
  {
    int i;
    for( i = 0; i < batch_count; i++ )
    {
      shared_overlay_data.batches[i].color_dim = batches[i].color_dim;
      shared_overlay_data.batches[i].alpha = batches[i].alpha;
    }
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
  int i;

  for( i = 0; i < GL_VIEW_MAX_BATCHES; i++ )
    free_ptr((void **)&batches[i].vertices);

  batch_count = 0;
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_geometry_invalidate() - Mark cached geometry stale so next render regenerates from NEC2 data
 */
  void
opengl_structure_geometry_invalidate(void)
{
  batch_count = 0;
}

/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
