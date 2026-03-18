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

/* Shared geometry buffer accessible by both structure window and overlay */
static lit_color_point_t *structure_vertices = NULL;
static int structure_vertex_count = 0;
static unsigned int structure_geometry_generation = 1;
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;
static float structure_view_scale = 1.0f;
static GLenum structure_draw_mode = GL_TRIANGLES;

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
  if( !crnt.valid || idx < 0 )
    return( 0.0 );

  if( mode == STRUCTURE_DRAW_CURRENTS && crnt.cur != NULL )
    return( cabs(crnt.cur[idx]) );

  if( mode == STRUCTURE_DRAW_CHARGES && crnt.bir != NULL && crnt.bii != NULL )
    return( cabs(cmplx(crnt.bir[idx], crnt.bii[idx])) );

  return( 0.0 );
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

  if( data.n <= 0 )
  {
    structure_vertex_count = 0;
    structure_view_scale = 1.0f;
    structure_draw_mode = GL_TRIANGLES;
    return( 0 );
  }

  line_mode = (cylinder_radius_scale < CYLINDER_SCALE_LINE_THRESHOLD);

  /* Vertex budget depends on rendering mode */
  if( line_mode )
    total_vertices = data.n * 2;
  else
    total_vertices = data.n * opengl_cylinder_vertex_count(STRUCTURE_CYLINDER_SEGMENTS);

  mreq = (size_t)total_vertices * sizeof(lit_color_point_t);
  mem_realloc((void **)&structure_vertices, mreq, __LOCATION__);

  /* Find max magnitude for color scaling */
  cmax = 0.0;
  if( (mode == STRUCTURE_DRAW_CURRENTS || mode == STRUCTURE_DRAW_CHARGES) && crnt.valid )
  {
    for( idx = 0; idx < data.n; idx++ )
    {
      double mag = get_segment_magnitude(idx, mode);
      if( mag > cmax )
        cmax = mag;
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
      set_lit_vertex(&structure_vertices[vidx],
          (float)data.x1[idx], (float)data.y1[idx], (float)data.z1[idx],
          nx, ny, nz, r, g, b, 1.0f);
      vidx++;

      /* Endpoint 2 */
      set_lit_vertex(&structure_vertices[vidx],
          (float)data.x2[idx], (float)data.y2[idx], (float)data.z2[idx],
          nx, ny, nz, r, g, b, 1.0f);
      vidx++;
    }

    structure_draw_mode = GL_LINES;
  }
  else
  {
    /* Cylinder mode: full 3D cylinders with minimum visible radius.
     * Scale minimum proportionally so ctrl+scroll affects zero-radius wires too. */
    double scale_factor = cylinder_radius_scale / CYLINDER_RADIUS_SCALE_DEFAULT;
    double min_visible = CYLINDER_MIN_VISIBLE_FRACTION * r_max * scale_factor;

    for( idx = 0; idx < data.n; idx++ )
    {
      lit_cylinder_mesh_t mesh;
      double radius;

      mesh.vertices = structure_vertices;
      mesh.vertex_count = total_vertices;

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

        vidx = opengl_lit_cylinder_append(&mesh, vidx,
            &seg_p1, &seg_p2, radius, STRUCTURE_CYLINDER_SEGMENTS, &seg_color);
      }
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
    shared_overlay_data.vertex_stride = (int)sizeof(lit_color_point_t);
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

#endif /* HAVE_OPENGL */
