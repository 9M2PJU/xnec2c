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

#include "opengl_rdpattern_geometry.h"
#include "../shared.h"
#include "../draw.h"
#include "../draw_radiation.h"

#ifdef HAVE_OPENGL

/* Triangle buffer for radiation pattern mesh */
static color_triangle_t *rdpat_triangles = NULL;
static int rdpat_triangle_count = 0;

/* Near-field line buffer */
static color_point_t *nf_lines = NULL;
static int nf_line_count = 0;

/* Poynting vector per-sample data */
typedef struct { double x, y, z, r; } poynting_vec_t;
static poynting_vec_t *pov = NULL;

/* Generation counters for change detection */
static unsigned int rdpat_ff_generation = 0;
static unsigned int rdpat_nf_generation = 0;

/*-----------------------------------------------------------------------*/

/** nf_line_append() - Fill two consecutive color_point_t entries as a line segment
 * @buf: vertex buffer
 * @line_idx: starting index in buf
 * @origin: line start point
 * @endpoint: line end point
 * @color: RGBA color applied to both vertices
 *
 * Returns line_idx advanced by 2.
 */
  static int
nf_line_append(
    color_point_t *buf, int line_idx,
    point_f_3d_t origin, point_f_3d_t endpoint, rgba_f_t color)
{
  buf[line_idx].point.x     = origin.x;
  buf[line_idx].point.y     = origin.y;
  buf[line_idx].point.z     = origin.z;
  buf[line_idx].color.r     = color.r;
  buf[line_idx].color.g     = color.g;
  buf[line_idx].color.b     = color.b;
  buf[line_idx].color.a     = color.a;

  buf[line_idx + 1].point.x = endpoint.x;
  buf[line_idx + 1].point.y = endpoint.y;
  buf[line_idx + 1].point.z = endpoint.z;
  buf[line_idx + 1].color   = buf[line_idx].color;

  return( line_idx + 2 );
}

/*-----------------------------------------------------------------------*/

/** nf_field_line() - Build a single near-field vector line from origin and field components
 * @buf: vertex buffer
 * @line_idx: starting index in buf
 * @px: origin x coordinate
 * @py: origin y coordinate
 * @pz: origin z coordinate
 * @comp_x: field vector x component
 * @comp_y: field vector y component
 * @comp_z: field vector z component
 * @dr: normalization scale distance
 * @magnitude: field vector magnitude
 * @max_val: maximum magnitude for color mapping
 *
 * Scales vector by dr / magnitude; colors by magnitude / max_val.
 * Returns line_idx advanced by 2.
 */
  static int
nf_field_line(
    color_point_t *buf, int line_idx,
    double px, double py, double pz,
    double comp_x, double comp_y, double comp_z,
    double dr, double magnitude, double max_val)
{
  double fscale;
  double red, grn, blu;

  point_f_3d_t org = { (float)px, (float)py, (float)pz };

  fscale = dr / magnitude;
  point_f_3d_t end = {
    (float)(px + comp_x * fscale),
    (float)(py + comp_y * fscale),
    (float)(pz + comp_z * fscale)
  };

  Value_to_Color(&red, &grn, &blu, magnitude, max_val);
  rgba_f_t col = { (float)red, (float)grn, (float)blu, 1.0f };

  return( nf_line_append(buf, line_idx, org, end, col) );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_generate_nf_lines() - Generate line geometry for near-field vectors
 *
 * Returns total line count, or -1 if near-field data is unavailable.
 */
  int
opengl_rdpattern_generate_nf_lines(void)
{
  int idx, npts, line_idx;
  int total_lines;
  double dr;
  double pov_max;
  size_t mreq;

  if( isFlagClear(ENABLE_NEAREH) || !near_field.valid )
    return( -1 );

  npts = fpat.nrx * fpat.nry * fpat.nrz;

  /* Count total lines needed for all enabled field types */
  total_lines = 0;
  if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) )
    total_lines += npts;
  if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    total_lines += npts;
  if( isFlagSet(DRAW_POYNTING) && (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    total_lines += npts;

  if( total_lines == 0 )
    return( -1 );

  /* Normalization scale factor */
  if( fpat.near )
    dr = (double)fpat.dxnr;
  else
    dr = sqrt(
        (double)fpat.dxnr * (double)fpat.dxnr +
        (double)fpat.dynr * (double)fpat.dynr +
        (double)fpat.dznr * (double)fpat.dznr ) / 1.75;

  /* Allocate line buffer for all enabled fields */
  mreq = (size_t)total_lines * 2 * sizeof(color_point_t);
  mem_realloc((void **)&nf_lines, mreq, __LOCATION__);

  /* Calculate Poynting vector if enabled */
  pov_max = 0.0;
  if( isFlagSet(DRAW_POYNTING) &&
      (fpat.nfeh & NEAR_EFIELD) &&
      (fpat.nfeh & NEAR_HFIELD) )
  {
    int ipv;

    mreq = (size_t)npts * sizeof(poynting_vec_t);
    mem_realloc((void **)&pov, mreq, __LOCATION__);

    for( ipv = 0; ipv < npts; ipv++ )
    {
      /* Poynting vector: E x H */
      pov[ipv].x =
        near_field.ery[ipv] * near_field.hrz[ipv] -
        near_field.hry[ipv] * near_field.erz[ipv];
      pov[ipv].y =
        near_field.erz[ipv] * near_field.hrx[ipv] -
        near_field.hrz[ipv] * near_field.erx[ipv];
      pov[ipv].z =
        near_field.erx[ipv] * near_field.hry[ipv] -
        near_field.hrx[ipv] * near_field.ery[ipv];
      pov[ipv].r = sqrt(
          pov[ipv].x * pov[ipv].x +
          pov[ipv].y * pov[ipv].y +
          pov[ipv].z * pov[ipv].z );

      if( pov_max < pov[ipv].r )
        pov_max = pov[ipv].r;
    }
  }

  /* Generate line vertices for all enabled field types */
  line_idx = 0;

  for( idx = 0; idx < npts; idx++ )
  {
    /* Draw Near E Field */
    if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) )
      line_idx = nf_field_line(nf_lines, line_idx,
          near_field.px[idx], near_field.py[idx], near_field.pz[idx],
          near_field.erx[idx], near_field.ery[idx], near_field.erz[idx],
          dr, near_field.er[idx], near_field.max_er);

    /* Draw Near H Field */
    if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) )
      line_idx = nf_field_line(nf_lines, line_idx,
          near_field.px[idx], near_field.py[idx], near_field.pz[idx],
          near_field.hrx[idx], near_field.hry[idx], near_field.hrz[idx],
          dr, near_field.hr[idx], near_field.max_hr);

    /* Draw Poynting Vector */
    if( isFlagSet(DRAW_POYNTING) && (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) )
      line_idx = nf_field_line(nf_lines, line_idx,
          near_field.px[idx], near_field.py[idx], near_field.pz[idx],
          pov[idx].x, pov[idx].y, pov[idx].z,
          dr, pov[idx].r, pov_max);
  }

  nf_line_count = total_lines;
  rdpat_nf_generation++;

  return( nf_line_count );

} /* opengl_rdpattern_generate_nf_lines() */

/*-----------------------------------------------------------------------*/

/** fill_tri_vertex() - Fill one vertex slot of a color_triangle_t from a point_3d_t
 * @tri: triangle to populate
 * @vi: vertex index within triangle (0, 1, or 2)
 * @pt: source point with position and radius
 * @r_min: minimum radius for color normalization
 * @r_range: radius range for color normalization
 *
 * Color is derived from the normalized (r - r_min) / r_range ratio.
 */
  static void
fill_tri_vertex(
    color_triangle_t *tri, int vi,
    point_3d_t *pt, double r_min, double r_range)
{
  double red, grn, blu;

  tri->cp[vi].point.x = (float)pt->x;
  tri->cp[vi].point.y = (float)pt->y;
  tri->cp[vi].point.z = (float)pt->z;
  tri->cp[vi].point.r = (float)pt->r;

  Value_to_Color(&red, &grn, &blu, (pt->r - r_min) / r_range, 1.0);
  tri->cp[vi].color.r = (float)red;
  tri->cp[vi].color.g = (float)grn;
  tri->cp[vi].color.b = (float)blu;
  tri->cp[vi].color.a = 1.0f;
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_generate_triangles() - Tessellate point_3d buffer into colored triangles
 * @points: array of radiation pattern sample points
 * @nth: number of theta samples
 * @nph: number of phi samples
 * @r_min: minimum radius for color normalization
 * @r_range: radius range for color normalization
 *
 * Returns triangle count, or -1 on invalid input.
 */
  int
opengl_rdpattern_generate_triangles(
    point_3d_t *points, int nth, int nph,
    double r_min, double r_range)
{
  int nph_idx, nth_idx, tri_idx;
  int p0, p1, p2, p3;
  size_t mreq;

  if( !points || nth < 2 || nph < 1 )
    return( -1 );

  /* Calculate triangle count: 2 per grid cell */
  rdpat_triangle_count = 2 * nph * (nth - 1);

  mreq = (size_t)rdpat_triangle_count * sizeof(color_triangle_t);
  mem_realloc((void **)&rdpat_triangles, mreq, __LOCATION__);

  tri_idx = 0;

  /* Generate triangles for each grid cell */
  for( nph_idx = 0; nph_idx < nph; nph_idx++ )
  {
    for( nth_idx = 0; nth_idx < nth - 1; nth_idx++ )
    {
      /* Calculate vertex indices with phi wrapping */
      p0 = nth_idx + nph_idx * nth;
      p1 = nth_idx + ((nph_idx + 1) % nph) * nth;
      p2 = (nth_idx + 1) + ((nph_idx + 1) % nph) * nth;
      p3 = (nth_idx + 1) + nph_idx * nth;

      /* Triangle A: p0 -> p1 -> p2 */
      fill_tri_vertex(&rdpat_triangles[tri_idx], 0, &points[p0], r_min, r_range);
      fill_tri_vertex(&rdpat_triangles[tri_idx], 1, &points[p1], r_min, r_range);
      fill_tri_vertex(&rdpat_triangles[tri_idx], 2, &points[p2], r_min, r_range);
      tri_idx++;

      /* Triangle B: p0 -> p2 -> p3 */
      fill_tri_vertex(&rdpat_triangles[tri_idx], 0, &points[p0], r_min, r_range);
      fill_tri_vertex(&rdpat_triangles[tri_idx], 1, &points[p2], r_min, r_range);
      fill_tri_vertex(&rdpat_triangles[tri_idx], 2, &points[p3], r_min, r_range);
      tri_idx++;

    } /* for( nth_idx < nth - 1 ) */
  } /* for( nph_idx < nph ) */

  rdpat_ff_generation++;

  return( rdpat_triangle_count );

} /* opengl_rdpattern_generate_triangles() */

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_get_nf_lines() - Return near-field line vertex buffer and line count
 * @count: output line count
 */
  color_point_t*
opengl_rdpattern_get_nf_lines(int *count)
{
  *count = nf_line_count;
  return( nf_lines );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_get_triangles() - Return far-field triangle buffer and triangle count
 * @count: output triangle count
 */
  color_triangle_t*
opengl_rdpattern_get_triangles(int *count)
{
  *count = rdpat_triangle_count;
  return( rdpat_triangles );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_get_ff_generation() - Return current far-field generation counter
 */
  unsigned int
opengl_rdpattern_get_ff_generation(void)
{
  return( rdpat_ff_generation );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_get_nf_generation() - Return current near-field generation counter
 */
  unsigned int
opengl_rdpattern_get_nf_generation(void)
{
  return( rdpat_nf_generation );
}

/*-----------------------------------------------------------------------*/

/** opengl_rdpattern_geometry_cleanup() - Free all geometry buffers and reset state
 */
  void
opengl_rdpattern_geometry_cleanup(void)
{
  free_ptr((void **)&rdpat_triangles);
  rdpat_triangle_count = 0;

  free_ptr((void **)&nf_lines);
  nf_line_count = 0;

  free_ptr((void **)&pov);
}

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
