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

/*-----------------------------------------------------------------------*/

/* opengl_cylinder_vertex_count()
 *
 * Calculate required vertex count for a cylinder with given segments
 * Each segment has 2 triangles (6 vertices) for the side
 * Plus 2 triangles for end caps = segments * 3 vertices each
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

/* opengl_lit_cylinder_append()
 *
 * Append lit cylinder triangles with computed normals to existing mesh buffer
 * Normals point radially outward from cylinder axis for side faces,
 * and along axis direction for end caps
 */
  int
opengl_lit_cylinder_append(
    lit_cylinder_mesh_t *mesh,
    int start_vertex,
    double x1, double y1, double z1,
    double x2, double y2, double z2,
    double radius,
    int segments,
    float r, float g, float b, float a)
{
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
    v[vidx].point.x = bv0x;
    v[vidx].point.y = bv0y;
    v[vidx].point.z = bv0z;
    v[vidx].normal.x = nx0;
    v[vidx].normal.y = ny0;
    v[vidx].normal.z = nz0;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = bv1x;
    v[vidx].point.y = bv1y;
    v[vidx].point.z = bv1z;
    v[vidx].normal.x = nx1;
    v[vidx].normal.y = ny1;
    v[vidx].normal.z = nz1;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = tv1x;
    v[vidx].point.y = tv1y;
    v[vidx].point.z = tv1z;
    v[vidx].normal.x = nx1;
    v[vidx].normal.y = ny1;
    v[vidx].normal.z = nz1;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    /* Triangle 2: bottom-left, top-right, top-left */
    v[vidx].point.x = bv0x;
    v[vidx].point.y = bv0y;
    v[vidx].point.z = bv0z;
    v[vidx].normal.x = nx0;
    v[vidx].normal.y = ny0;
    v[vidx].normal.z = nz0;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = tv1x;
    v[vidx].point.y = tv1y;
    v[vidx].point.z = tv1z;
    v[vidx].normal.x = nx1;
    v[vidx].normal.y = ny1;
    v[vidx].normal.z = nz1;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = tv0x;
    v[vidx].point.y = tv0y;
    v[vidx].point.z = tv0z;
    v[vidx].normal.x = nx0;
    v[vidx].normal.y = ny0;
    v[vidx].normal.z = nz0;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;
  }

  /* End cap normals point along axis */
  float cap_nx_bot = (float)(-dx);
  float cap_ny_bot = (float)(-dy);
  float cap_nz_bot = (float)(-dz);
  float cap_nx_top = (float)(dx);
  float cap_ny_top = (float)(dy);
  float cap_nz_top = (float)(dz);

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

    /* Bottom cap triangle */
    v[vidx].point.x = (float)x1;
    v[vidx].point.y = (float)y1;
    v[vidx].point.z = (float)z1;
    v[vidx].normal.x = cap_nx_bot;
    v[vidx].normal.y = cap_ny_bot;
    v[vidx].normal.z = cap_nz_bot;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = (float)(x1 + px1);
    v[vidx].point.y = (float)(y1 + py1);
    v[vidx].point.z = (float)(z1 + pz1);
    v[vidx].normal.x = cap_nx_bot;
    v[vidx].normal.y = cap_ny_bot;
    v[vidx].normal.z = cap_nz_bot;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = (float)(x1 + px0);
    v[vidx].point.y = (float)(y1 + py0);
    v[vidx].point.z = (float)(z1 + pz0);
    v[vidx].normal.x = cap_nx_bot;
    v[vidx].normal.y = cap_ny_bot;
    v[vidx].normal.z = cap_nz_bot;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    /* Top cap triangle */
    v[vidx].point.x = (float)x2;
    v[vidx].point.y = (float)y2;
    v[vidx].point.z = (float)z2;
    v[vidx].normal.x = cap_nx_top;
    v[vidx].normal.y = cap_ny_top;
    v[vidx].normal.z = cap_nz_top;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = (float)(x2 + px0);
    v[vidx].point.y = (float)(y2 + py0);
    v[vidx].point.z = (float)(z2 + pz0);
    v[vidx].normal.x = cap_nx_top;
    v[vidx].normal.y = cap_ny_top;
    v[vidx].normal.z = cap_nz_top;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;

    v[vidx].point.x = (float)(x2 + px1);
    v[vidx].point.y = (float)(y2 + py1);
    v[vidx].point.z = (float)(z2 + pz1);
    v[vidx].normal.x = cap_nx_top;
    v[vidx].normal.y = cap_ny_top;
    v[vidx].normal.z = cap_nz_top;
    v[vidx].color.r = r;
    v[vidx].color.g = g;
    v[vidx].color.b = b;
    v[vidx].color.a = a;
    vidx++;
  }

  return( vidx );

} /* opengl_lit_cylinder_append() */

/*-----------------------------------------------------------------------*/

/* opengl_lit_cylinder_free()
 *
 * Free lit cylinder mesh data
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
