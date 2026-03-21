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

#include "opengl_cylinder.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/* Sub-visual inset of end cap vertices along cylinder axis to prevent
 * z-fighting when a wire endpoint lies on a surface patch */
#define CYLINDER_CAP_INSET 1e-6

/*-----------------------------------------------------------------------*/

/** set_lit_vertex() - Set all fields of a lit vertex (position, normal, color)
 * @v: vertex structure to populate
 * @px: position x coordinate
 * @py: position y coordinate
 * @pz: position z coordinate
 * @nx: normal x component
 * @ny: normal y component
 * @nz: normal z component
 * @cr: color red component
 * @cg: color green component
 * @cb: color blue component
 * @ca: color alpha component
 */
  void
set_lit_vertex(lit_color_point_t *v,
    float px, float py, float pz,
    float nx, float ny, float nz,
    float cr, float cg, float cb, float ca)
{
  v->point.x = px;
  v->point.y = py;
  v->point.z = pz;
  v->normal.x = nx;
  v->normal.y = ny;
  v->normal.z = nz;
  v->color.r = cr;
  v->color.g = cg;
  v->color.b = cb;
  v->color.a = ca;

} /* set_lit_vertex() */

/*-----------------------------------------------------------------------*/

/** opengl_cylinder_vertex_count() - Calculate required vertex count for a cylinder with given segments
 * @segments: number of segments around cylinder circumference
 */
  int
opengl_cylinder_vertex_count(int segments)
{
  int side_vertices;
  int cap_vertices;

  if( segments < 3 )
    segments = 3;

  /* Side: 2 triangles per segment = 6 vertices per segment */
  side_vertices = segments * 6;

  /* End caps: triangle fan = segments triangles * 3 vertices each, times 2 caps */
  cap_vertices = segments * 3 * 2;

  return( side_vertices + cap_vertices );

} /* opengl_cylinder_vertex_count() */

/*-----------------------------------------------------------------------*/

/** opengl_lit_cylinder_append() - Append lit cylinder triangles with computed normals to existing mesh buffer
 * @mesh: mesh buffer to append to
 * @start_vertex: starting vertex index in mesh
 * @p1: cylinder bottom endpoint
 * @p2: cylinder top endpoint
 * @radius: cylinder radius
 * @segments: number of segments around circumference
 * @color: cylinder color
 */
  int
opengl_lit_cylinder_append(
    lit_cylinder_mesh_t *mesh,
    int start_vertex,
    const point_f_3d_t *p1,
    const point_f_3d_t *p2,
    double radius,
    int segments,
    const rgba_f_t *color)
{
  double x1 = p1->x, y1 = p1->y, z1 = p1->z;
  double x2 = p2->x, y2 = p2->y, z2 = p2->z;
  float r = color->r, g = color->g, b = color->b, a = color->a;
  double dx, dy, dz, length;
  double ax, ay, az;
  double bx, by, bz;
  double mag;
  int i, vidx;
  lit_color_point_t *v;

  if( !mesh || !mesh->vertices )
    return( -1 );

  if( segments < 3 )
    segments = 3;

  if( radius < CYLINDER_MIN_RADIUS )
    radius = CYLINDER_MIN_RADIUS;

  /* Calculate cylinder axis vector */
  dx = x2 - x1;
  dy = y2 - y1;
  dz = z2 - z1;
  length = sqrt(dx * dx + dy * dy + dz * dz);

  if( length < 1e-10 )
    return( start_vertex );

  /* Normalize axis */
  dx /= length;
  dy /= length;
  dz /= length;

  /* Find perpendicular vectors for cylinder cross-section */
  if( fabs(dx) < 0.9 )
  {
    ax = 0.0;
    ay = -dz;
    az = dy;
  }
  else
  {
    ax = -dz;
    ay = 0.0;
    az = dx;
  }

  mag = sqrt(ax * ax + ay * ay + az * az);
  ax /= mag;
  ay /= mag;
  az /= mag;

  /* Second perpendicular via cross product */
  bx = dy * az - dz * ay;
  by = dz * ax - dx * az;
  bz = dx * ay - dy * ax;

  v = mesh->vertices;
  vidx = start_vertex;

  /* Generate cylinder side triangles with radial normals */
  for( i = 0; i < segments; i++ )
  {
    double angle0 = 2.0 * M_PI * (double)i / (double)segments;
    double angle1 = 2.0 * M_PI * (double)(i + 1) / (double)segments;
    double cos0 = cos(angle0);
    double sin0 = sin(angle0);
    double cos1 = cos(angle1);
    double sin1 = sin(angle1);

    /* Radial direction for normal at each angle */
    float nx0 = (float)(cos0 * ax + sin0 * bx);
    float ny0 = (float)(cos0 * ay + sin0 * by);
    float nz0 = (float)(cos0 * az + sin0 * bz);

    float nx1 = (float)(cos1 * ax + sin1 * bx);
    float ny1 = (float)(cos1 * ay + sin1 * by);
    float nz1 = (float)(cos1 * az + sin1 * bz);

    /* Circle points at each end */
    double px0 = radius * (cos0 * ax + sin0 * bx);
    double py0 = radius * (cos0 * ay + sin0 * by);
    double pz0 = radius * (cos0 * az + sin0 * bz);

    double px1 = radius * (cos1 * ax + sin1 * bx);
    double py1 = radius * (cos1 * ay + sin1 * by);
    double pz1 = radius * (cos1 * az + sin1 * bz);

    /* Bottom vertices */
    float bv0x = (float)(x1 + px0);
    float bv0y = (float)(y1 + py0);
    float bv0z = (float)(z1 + pz0);

    float bv1x = (float)(x1 + px1);
    float bv1y = (float)(y1 + py1);
    float bv1z = (float)(z1 + pz1);

    /* Top vertices */
    float tv0x = (float)(x2 + px0);
    float tv0y = (float)(y2 + py0);
    float tv0z = (float)(z2 + pz0);

    float tv1x = (float)(x2 + px1);
    float tv1y = (float)(y2 + py1);
    float tv1z = (float)(z2 + pz1);

    /* Triangle 1: bottom-left, bottom-right, top-right */
    set_lit_vertex(&v[vidx++], bv0x, bv0y, bv0z, nx0, ny0, nz0, r, g, b, a);
    set_lit_vertex(&v[vidx++], bv1x, bv1y, bv1z, nx1, ny1, nz1, r, g, b, a);
    set_lit_vertex(&v[vidx++], tv1x, tv1y, tv1z, nx1, ny1, nz1, r, g, b, a);

    /* Triangle 2: bottom-left, top-right, top-left */
    set_lit_vertex(&v[vidx++], bv0x, bv0y, bv0z, nx0, ny0, nz0, r, g, b, a);
    set_lit_vertex(&v[vidx++], tv1x, tv1y, tv1z, nx1, ny1, nz1, r, g, b, a);
    set_lit_vertex(&v[vidx++], tv0x, tv0y, tv0z, nx0, ny0, nz0, r, g, b, a);
  }

  /* End cap normals point along axis */
  float cap_nx_bot = (float)(-dx);
  float cap_ny_bot = (float)(-dy);
  float cap_nz_bot = (float)(-dz);
  float cap_nx_top = (float)(dx);
  float cap_ny_top = (float)(dy);
  float cap_nz_top = (float)(dz);

  /* Inset cap vertices along the axis to prevent z-fighting
   * when a wire endpoint lies on a surface patch */
  double cap_inset = length * CYLINDER_CAP_INSET;
  double bot_cx = x1 + cap_inset * dx;
  double bot_cy = y1 + cap_inset * dy;
  double bot_cz = z1 + cap_inset * dz;
  double top_cx = x2 - cap_inset * dx;
  double top_cy = y2 - cap_inset * dy;
  double top_cz = z2 - cap_inset * dz;

  /* Generate end cap triangles */
  for( i = 0; i < segments; i++ )
  {
    double angle0 = 2.0 * M_PI * (double)i / (double)segments;
    double angle1 = 2.0 * M_PI * (double)(i + 1) / (double)segments;
    double cos0 = cos(angle0);
    double sin0 = sin(angle0);
    double cos1 = cos(angle1);
    double sin1 = sin(angle1);

    double px0 = radius * (cos0 * ax + sin0 * bx);
    double py0 = radius * (cos0 * ay + sin0 * by);
    double pz0 = radius * (cos0 * az + sin0 * bz);

    double px1 = radius * (cos1 * ax + sin1 * bx);
    double py1 = radius * (cos1 * ay + sin1 * by);
    double pz1 = radius * (cos1 * az + sin1 * bz);

    /* Bottom cap triangle (inset along axis) */
    set_lit_vertex(&v[vidx++],
        (float)bot_cx, (float)bot_cy, (float)bot_cz,
        cap_nx_bot, cap_ny_bot, cap_nz_bot, r, g, b, a);
    set_lit_vertex(&v[vidx++],
        (float)(bot_cx + px1), (float)(bot_cy + py1), (float)(bot_cz + pz1),
        cap_nx_bot, cap_ny_bot, cap_nz_bot, r, g, b, a);
    set_lit_vertex(&v[vidx++],
        (float)(bot_cx + px0), (float)(bot_cy + py0), (float)(bot_cz + pz0),
        cap_nx_bot, cap_ny_bot, cap_nz_bot, r, g, b, a);

    /* Top cap triangle (inset along axis) */
    set_lit_vertex(&v[vidx++],
        (float)top_cx, (float)top_cy, (float)top_cz,
        cap_nx_top, cap_ny_top, cap_nz_top, r, g, b, a);
    set_lit_vertex(&v[vidx++],
        (float)(top_cx + px0), (float)(top_cy + py0), (float)(top_cz + pz0),
        cap_nx_top, cap_ny_top, cap_nz_top, r, g, b, a);
    set_lit_vertex(&v[vidx++],
        (float)(top_cx + px1), (float)(top_cy + py1), (float)(top_cz + pz1),
        cap_nx_top, cap_ny_top, cap_nz_top, r, g, b, a);
  }

  return( vidx );

} /* opengl_lit_cylinder_append() */

/*-----------------------------------------------------------------------*/

/** opengl_lit_cylinder_free() - Free lit cylinder mesh data
 * @mesh: mesh structure to free
 */
  void
opengl_lit_cylinder_free(lit_cylinder_mesh_t *mesh)
{
  if( !mesh )
    return;

  free_ptr((void **)&mesh->vertices);
  mesh->vertex_count = 0;
  mesh->triangle_count = 0;

} /* opengl_lit_cylinder_free() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
