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

#include "opengl_rdpattern.h"
#include "opengl_structure.h"
#include "shared.h"
#include "draw.h"
#include "draw_radiation.h"

#ifdef HAVE_OPENGL

#include "opengl_view.h"

/* Triangle buffer for radiation pattern mesh */
static color_triangle_t *rdpat_triangles = NULL;
static int rdpat_triangle_count = 0;

/* Near-field line buffer */
static color_point_t *nf_lines = NULL;
static int nf_line_count = 0;

/* Poynting vector buffers */
static double *pov_x = NULL;
static double *pov_y = NULL;
static double *pov_z = NULL;
static double *pov_r = NULL;

/* Generation counters for change detection */
static unsigned int rdpat_ff_generation = 0;
static unsigned int rdpat_nf_generation = 0;
static unsigned int rdpat_last_ff_gen = 0;

/* Widget pointer for external access */
static GtkWidget *rdpattern_gl_widget = NULL;

/* Arcball for radiation pattern view */
static arcball_state_t *rdpattern_arcball = NULL;

/* Vertex attribute layout for color shader */
static const gl_vertex_attrib_t rdpattern_attribs[] = {
  { "position", 3, 0 },
  { "color",    4, 4 * (int)sizeof(float) }
};

/* Overlay configuration for structure rendering in rdpattern */
static const gl_overlay_config_t rdpattern_overlay_config = {
  .vertex_shader_path = "/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "/gl/lit-color-fragment.glsl",
  .attribs = opengl_structure_attribs,
  .attrib_count = 3
};

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_generate_nf_lines()
 *
 * Generate line geometry for near field vectors
 */
  static int
opengl_rdpattern_generate_nf_lines(void)
{
  int idx, npts, line_idx;
  int total_lines;
  double fscale;
  double dr;
  double red, grn, blu;
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

    mreq = (size_t)npts * sizeof(double);
    mem_realloc((void **)&pov_x, mreq, __LOCATION__);
    mem_realloc((void **)&pov_y, mreq, __LOCATION__);
    mem_realloc((void **)&pov_z, mreq, __LOCATION__);
    mem_realloc((void **)&pov_r, mreq, __LOCATION__);

    for( ipv = 0; ipv < npts; ipv++ )
    {
      /* Poynting vector: E x H */
      pov_x[ipv] =
        near_field.ery[ipv] * near_field.hrz[ipv] -
        near_field.hry[ipv] * near_field.erz[ipv];
      pov_y[ipv] =
        near_field.erz[ipv] * near_field.hrx[ipv] -
        near_field.hrz[ipv] * near_field.erx[ipv];
      pov_z[ipv] =
        near_field.erx[ipv] * near_field.hry[ipv] -
        near_field.hrx[ipv] * near_field.ery[ipv];
      pov_r[ipv] = sqrt(
          pov_x[ipv] * pov_x[ipv] +
          pov_y[ipv] * pov_y[ipv] +
          pov_z[ipv] * pov_z[ipv] );

      if( pov_max < pov_r[ipv] )
        pov_max = pov_r[ipv];
    }
  }

  /* Generate line vertices for all enabled field types */
  line_idx = 0;

  for( idx = 0; idx < npts; idx++ )
  {
    /* Draw Near E Field */
    if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) )
    {
      fscale = dr / near_field.er[idx];

      nf_lines[line_idx].point.x = (float)near_field.px[idx];
      nf_lines[line_idx].point.y = (float)near_field.py[idx];
      nf_lines[line_idx].point.z = (float)near_field.pz[idx];

      nf_lines[line_idx + 1].point.x = (float)(near_field.px[idx] + near_field.erx[idx] * fscale);
      nf_lines[line_idx + 1].point.y = (float)(near_field.py[idx] + near_field.ery[idx] * fscale);
      nf_lines[line_idx + 1].point.z = (float)(near_field.pz[idx] + near_field.erz[idx] * fscale);

      Value_to_Color(&red, &grn, &blu, near_field.er[idx], near_field.max_er);

      nf_lines[line_idx].color.r = (float)red;
      nf_lines[line_idx].color.g = (float)grn;
      nf_lines[line_idx].color.b = (float)blu;
      nf_lines[line_idx].color.a = 1.0f;

      nf_lines[line_idx + 1].color = nf_lines[line_idx].color;

      line_idx += 2;
    }

    /* Draw Near H Field */
    if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    {
      fscale = dr / near_field.hr[idx];

      nf_lines[line_idx].point.x = (float)near_field.px[idx];
      nf_lines[line_idx].point.y = (float)near_field.py[idx];
      nf_lines[line_idx].point.z = (float)near_field.pz[idx];

      nf_lines[line_idx + 1].point.x = (float)(near_field.px[idx] + near_field.hrx[idx] * fscale);
      nf_lines[line_idx + 1].point.y = (float)(near_field.py[idx] + near_field.hry[idx] * fscale);
      nf_lines[line_idx + 1].point.z = (float)(near_field.pz[idx] + near_field.hrz[idx] * fscale);

      Value_to_Color(&red, &grn, &blu, near_field.hr[idx], near_field.max_hr);

      nf_lines[line_idx].color.r = (float)red;
      nf_lines[line_idx].color.g = (float)grn;
      nf_lines[line_idx].color.b = (float)blu;
      nf_lines[line_idx].color.a = 1.0f;

      nf_lines[line_idx + 1].color = nf_lines[line_idx].color;

      line_idx += 2;
    }

    /* Draw Poynting Vector */
    if( isFlagSet(DRAW_POYNTING) && (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    {
      fscale = dr / pov_r[idx];

      nf_lines[line_idx].point.x = (float)near_field.px[idx];
      nf_lines[line_idx].point.y = (float)near_field.py[idx];
      nf_lines[line_idx].point.z = (float)near_field.pz[idx];

      nf_lines[line_idx + 1].point.x = (float)(near_field.px[idx] + pov_x[idx] * fscale);
      nf_lines[line_idx + 1].point.y = (float)(near_field.py[idx] + pov_y[idx] * fscale);
      nf_lines[line_idx + 1].point.z = (float)(near_field.pz[idx] + pov_z[idx] * fscale);

      Value_to_Color(&red, &grn, &blu, pov_r[idx], pov_max);

      nf_lines[line_idx].color.r = (float)red;
      nf_lines[line_idx].color.g = (float)grn;
      nf_lines[line_idx].color.b = (float)blu;
      nf_lines[line_idx].color.a = 1.0f;

      nf_lines[line_idx + 1].color = nf_lines[line_idx].color;

      line_idx += 2;
    }
  }

  nf_line_count = total_lines;
  rdpat_nf_generation++;

  return( nf_line_count );

} /* opengl_rdpattern_generate_nf_lines() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_generate_triangles()
 *
 * Tessellate point_3d buffer into colored triangles
 */
  static int
opengl_rdpattern_generate_triangles(
    point_3d_t *points, int nth, int nph,
    double r_min, double r_range)
{
  int nph_idx, nth_idx, tri_idx;
  int p0, p1, p2, p3;
  double red, grn, blu;
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
      rdpat_triangles[tri_idx].cp[0].point.x = (float)points[p0].x;
      rdpat_triangles[tri_idx].cp[0].point.y = (float)points[p0].y;
      rdpat_triangles[tri_idx].cp[0].point.z = (float)points[p0].z;
      rdpat_triangles[tri_idx].cp[0].point.r = (float)points[p0].r;

      rdpat_triangles[tri_idx].cp[1].point.x = (float)points[p1].x;
      rdpat_triangles[tri_idx].cp[1].point.y = (float)points[p1].y;
      rdpat_triangles[tri_idx].cp[1].point.z = (float)points[p1].z;
      rdpat_triangles[tri_idx].cp[1].point.r = (float)points[p1].r;

      rdpat_triangles[tri_idx].cp[2].point.x = (float)points[p2].x;
      rdpat_triangles[tri_idx].cp[2].point.y = (float)points[p2].y;
      rdpat_triangles[tri_idx].cp[2].point.z = (float)points[p2].z;
      rdpat_triangles[tri_idx].cp[2].point.r = (float)points[p2].r;

      Value_to_Color(&red, &grn, &blu,
          (points[p0].r - r_min) / r_range, 1.0);
      rdpat_triangles[tri_idx].cp[0].color.r = (float)red;
      rdpat_triangles[tri_idx].cp[0].color.g = (float)grn;
      rdpat_triangles[tri_idx].cp[0].color.b = (float)blu;
      rdpat_triangles[tri_idx].cp[0].color.a = 1.0f;

      Value_to_Color(&red, &grn, &blu,
          (points[p1].r - r_min) / r_range, 1.0);
      rdpat_triangles[tri_idx].cp[1].color.r = (float)red;
      rdpat_triangles[tri_idx].cp[1].color.g = (float)grn;
      rdpat_triangles[tri_idx].cp[1].color.b = (float)blu;
      rdpat_triangles[tri_idx].cp[1].color.a = 1.0f;

      Value_to_Color(&red, &grn, &blu,
          (points[p2].r - r_min) / r_range, 1.0);
      rdpat_triangles[tri_idx].cp[2].color.r = (float)red;
      rdpat_triangles[tri_idx].cp[2].color.g = (float)grn;
      rdpat_triangles[tri_idx].cp[2].color.b = (float)blu;
      rdpat_triangles[tri_idx].cp[2].color.a = 1.0f;

      tri_idx++;

      /* Triangle B: p0 -> p2 -> p3 */
      rdpat_triangles[tri_idx].cp[0].point.x = (float)points[p0].x;
      rdpat_triangles[tri_idx].cp[0].point.y = (float)points[p0].y;
      rdpat_triangles[tri_idx].cp[0].point.z = (float)points[p0].z;
      rdpat_triangles[tri_idx].cp[0].point.r = (float)points[p0].r;

      rdpat_triangles[tri_idx].cp[1].point.x = (float)points[p2].x;
      rdpat_triangles[tri_idx].cp[1].point.y = (float)points[p2].y;
      rdpat_triangles[tri_idx].cp[1].point.z = (float)points[p2].z;
      rdpat_triangles[tri_idx].cp[1].point.r = (float)points[p2].r;

      rdpat_triangles[tri_idx].cp[2].point.x = (float)points[p3].x;
      rdpat_triangles[tri_idx].cp[2].point.y = (float)points[p3].y;
      rdpat_triangles[tri_idx].cp[2].point.z = (float)points[p3].z;
      rdpat_triangles[tri_idx].cp[2].point.r = (float)points[p3].r;

      Value_to_Color(&red, &grn, &blu,
          (points[p0].r - r_min) / r_range, 1.0);
      rdpat_triangles[tri_idx].cp[0].color.r = (float)red;
      rdpat_triangles[tri_idx].cp[0].color.g = (float)grn;
      rdpat_triangles[tri_idx].cp[0].color.b = (float)blu;
      rdpat_triangles[tri_idx].cp[0].color.a = 1.0f;

      Value_to_Color(&red, &grn, &blu,
          (points[p2].r - r_min) / r_range, 1.0);
      rdpat_triangles[tri_idx].cp[1].color.r = (float)red;
      rdpat_triangles[tri_idx].cp[1].color.g = (float)grn;
      rdpat_triangles[tri_idx].cp[1].color.b = (float)blu;
      rdpat_triangles[tri_idx].cp[1].color.a = 1.0f;

      Value_to_Color(&red, &grn, &blu,
          (points[p3].r - r_min) / r_range, 1.0);
      rdpat_triangles[tri_idx].cp[2].color.r = (float)red;
      rdpat_triangles[tri_idx].cp[2].color.g = (float)grn;
      rdpat_triangles[tri_idx].cp[2].color.b = (float)blu;
      rdpat_triangles[tri_idx].cp[2].color.a = 1.0f;

      tri_idx++;

    } /* for( nth_idx < nth - 1 ) */
  } /* for( nph_idx < nph ) */

  return( rdpat_triangle_count );

} /* opengl_rdpattern_generate_triangles() */

/*-----------------------------------------------------------------------*/

/* rdpattern_scene_generate()
 *
 * Scene provider generate callback.
 * Handles both near-field (lines) and far-field (triangles) modes.
 */
  static gboolean
rdpattern_scene_generate(gl_view_content_t *out)
{
  float zoom;
  float r_max;

  /* Zoom from rdpattern spinbutton, normalized to multiplier */
  zoom = 1.0f;
  if( rdpattern_zoom )
    zoom = (float)gtk_spin_button_get_value(rdpattern_zoom) / 100.0f;

  if( zoom < 0.01f )
    zoom = 0.01f;

  /* Near-field rendering path */
  if( isFlagSet(DRAW_EHFIELD) && isFlagSet(ENABLE_NEAREH) && near_field.valid )
  {
    int line_count;

    line_count = opengl_rdpattern_generate_nf_lines();
    if( line_count <= 0 )
      return( FALSE );

    r_max = (float)near_field.r_max;

    out->vertices = nf_lines;
    out->vertex_count = nf_line_count * 2;
    out->vertex_stride = (int)sizeof(color_point_t);
    out->draw_mode = GL_LINES;
    out->r_max = r_max;
    out->zoom = zoom;
    out->model_scale = 1.0f;
    out->show_gradient = FALSE;
    out->generation = rdpat_nf_generation;

    return( TRUE );
  }

  /* Far-field (gain) rendering path */
  if( isFlagSet(DRAW_GAIN) )
  {
    unsigned int current_gen;
    double r_min, r_range;
    rdpattern_data_t rd_data;

    current_gen = Generate_Rdpattern_Data(&r_min, &r_range);
    if( current_gen == 0 )
      return( FALSE );

    if( !Get_Radiation_Pattern_Data(&rd_data) || !rd_data.valid )
      return( FALSE );

    r_max = (float)rd_data.r_max;

    /* Regenerate triangles on data change */
    if( current_gen != rdpat_last_ff_gen )
    {
      int tri_count = opengl_rdpattern_generate_triangles(
          rd_data.points, rd_data.nth, rd_data.nph, r_min, r_range);

      if( tri_count <= 0 )
        return( FALSE );

      rdpat_ff_generation++;
      rdpat_last_ff_gen = current_gen;
    }

    if( rdpat_triangle_count == 0 )
      return( FALSE );

    out->vertices = rdpat_triangles;
    out->vertex_count = rdpat_triangle_count * 3;
    out->vertex_stride = (int)sizeof(color_point_t);
    out->draw_mode = GL_TRIANGLES;
    out->r_max = r_max;
    out->zoom = zoom;
    out->model_scale = 1.0f;
    out->show_gradient = isFlagSet(DRAW_GAIN) && rc_config.rdpattern_gradient_key;
    out->generation = rdpat_ff_generation;

    return( TRUE );
  }

  return( FALSE );
}

/*-----------------------------------------------------------------------*/

/* rdpattern_scene_post_render()
 *
 * Scene provider post-render callback
 */
  static void
rdpattern_scene_post_render(void)
{
  Update_Rdpattern_UI();
}

/*-----------------------------------------------------------------------*/

/* rdpattern_scene_cleanup()
 *
 * Scene provider cleanup callback
 */
  static void
rdpattern_scene_cleanup(void)
{
  free_ptr((void **)&rdpat_triangles);
  rdpat_triangle_count = 0;

  free_ptr((void **)&nf_lines);
  nf_line_count = 0;

  free_ptr((void **)&pov_x);
  free_ptr((void **)&pov_y);
  free_ptr((void **)&pov_z);
  free_ptr((void **)&pov_r);
}

/*-----------------------------------------------------------------------*/

/* rdpattern_overlay_generate()
 *
 * Overlay provider generate callback.
 * Returns structure geometry for overlay when OVERLAY_STRUCT is set.
 */
  static gboolean
rdpattern_overlay_generate(gl_view_content_t *out)
{
  const structure_overlay_data_t *geom;

  if( isFlagClear(OVERLAY_STRUCT) || data.n <= 0 )
    return( FALSE );

  /* Ensure shared geometry is fresh */
  opengl_structure_update_shared_geometry();
  geom = opengl_structure_get_shared_geometry();

  if( !geom || geom->vertex_count <= 0 )
    return( FALSE );

  out->vertices = geom->vertices;
  out->vertex_count = geom->vertex_count;
  out->vertex_stride = geom->vertex_stride;
  out->draw_mode = GL_TRIANGLES;
  out->show_gradient = FALSE;

  /* Overlay pass shares primary content MVP; these fields are unused */
  out->r_max = 0.0f;
  out->zoom = 1.0f;
  out->model_scale = 1.0f;
  out->generation = geom->generation;

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* Static scene configuration and provider */
static gl_view_config_t rdpattern_view_config = {
  .vertex_shader_path = "/gl/color-vertex.glsl",
  .fragment_shader_path = "/gl/color-fragment.glsl",
  .attribs = rdpattern_attribs,
  .attrib_count = 2,
  .vertex_stride = (int)sizeof(color_point_t),
  .has_gradient = TRUE,
  .gradient_draw = Draw_Color_Legend_Overlay
};

static gl_scene_provider_t rdpattern_scene_provider = {
  .generate = rdpattern_scene_generate,
  .post_render = rdpattern_scene_post_render,
  .cleanup = rdpattern_scene_cleanup,
  .overlay_config = &rdpattern_overlay_config,
  .overlay_generate = rdpattern_overlay_generate,
  .overlay_cleanup = NULL
};

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_create_widget()
 *
 * Create the OpenGL radiation pattern widget using the generic view engine
 */
  GtkWidget*
opengl_rdpattern_create_widget(void)
{
  if( !rdpattern_arcball )
    rdpattern_arcball = arcball_new();

  rdpattern_gl_widget = gl_view_create_widget(
      &rdpattern_view_config,
      &rdpattern_scene_provider,
      rdpattern_arcball,
      &rdpattern_zoom);

  gtk_widget_show(rdpattern_gl_widget);

  return( rdpattern_gl_widget );
}

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_get_arcball()
 *
 * Return reference to rdpattern arcball
 */
  arcball_state_t*
opengl_rdpattern_get_arcball(void)
{
  return( rdpattern_arcball );
}

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_get_widget()
 *
 * Return the rdpattern GL widget
 */
  GtkWidget*
opengl_rdpattern_get_widget(void)
{
  return( rdpattern_gl_widget );
}

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_cleanup()
 *
 * Free resources
 */
  void
opengl_rdpattern_cleanup(void)
{
  rdpattern_scene_cleanup();
  rdpattern_gl_widget = NULL;
}

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
