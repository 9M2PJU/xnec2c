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

/* Default structure overlay extent as multiple of radiation pattern r_max */
#define OVERLAY_DEFAULT_EXTENT 1.25f

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

/* Track translation state to detect changes */
static gboolean rdpat_last_translate_state = FALSE;

/* Cache overlay model scale adjustment for translation */
static double rdpat_ovl_scale_adj = 1.0;
static double rdpat_last_ovl_scale_adj = 1.0;

/* Translated far-field points buffer for excitation center offset */
static point_3d_t *rdpat_translated_points = NULL;
static int rdpat_translated_capacity = 0;

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

/* rdpattern_overlay_base_scale()
 *
 * Calculate the base scale factor mapping structure coordinates
 * to radiation pattern coordinate space
 */
  static float
rdpattern_overlay_base_scale(float r_max, float view_scale)
{
  if( view_scale > 0.001f )
    return( OVERLAY_DEFAULT_EXTENT * r_max / view_scale );

  return( 1.0f );
}

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
  gboolean translate_to_excitation;
  float offset_x, offset_y, offset_z;
  const structure_overlay_data_t *geom = NULL;

  /* Zoom from rdpattern spinbutton, normalized to multiplier */
  zoom = 1.0f;
  if( rdpattern_zoom )
    zoom = (float)gtk_spin_button_get_value(rdpattern_zoom) / 100.0f;

  if( zoom < 0.01f )
    zoom = 0.01f;

  /* Check if we need to translate pattern to excitation center */
  translate_to_excitation = FALSE;
  offset_x = 0.0f;
  offset_y = 0.0f;
  offset_z = 0.0f;

  if( isFlagSet(OVERLAY_STRUCT) )
  {
    geom = opengl_structure_get_shared_geometry();
    if( geom && geom->has_excitation_center )
    {
      translate_to_excitation = TRUE;

      offset_x = (float)geom->excitation_center_x;
      offset_y = (float)geom->excitation_center_y;
      offset_z = (float)geom->excitation_center_z;
    }
  }

  /* Near-field rendering path */
  if( isFlagSet(DRAW_EHFIELD) && isFlagSet(ENABLE_NEAREH) && near_field.valid )
  {
    int line_count;

    line_count = opengl_rdpattern_generate_nf_lines();
    if( line_count <= 0 )
      return( FALSE );

    r_max = (float)near_field.r_max;

    /* Near-field positions directly overlap structure in same coordinate space;
     * no translation needed (excitation already aligned) */

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
    point_3d_t *points_to_use;

    current_gen = Generate_Rdpattern_Data(&r_min, &r_range);
    if( current_gen == 0 )
      return( FALSE );

    if( !Get_Radiation_Pattern_Data(&rd_data) || !rd_data.valid )
      return( FALSE );

    r_max = (float)rd_data.r_max;

    points_to_use = rd_data.points;

    if( translate_to_excitation )
    {
      int npts, i;
      float model_scale;
      float scaled_off_x, scaled_off_y, scaled_off_z;

      npts = rd_data.nth * rd_data.nph;

      /* Scale offset from structure coords to radiation pattern coords */
      model_scale = rdpattern_overlay_base_scale(r_max, geom->view_scale)
          * (float)rdpat_ovl_scale_adj;

      scaled_off_x = offset_x * model_scale;
      scaled_off_y = offset_y * model_scale;
      scaled_off_z = offset_z * model_scale;

      if( npts > rdpat_translated_capacity )
      {
        size_t mreq = (size_t)npts * sizeof(point_3d_t);
        mem_realloc((void **)&rdpat_translated_points, mreq, __LOCATION__);
        rdpat_translated_capacity = npts;
      }

      for( i = 0; i < npts; i++ )
      {
        rdpat_translated_points[i].x = rd_data.points[i].x + (double)scaled_off_x;
        rdpat_translated_points[i].y = rd_data.points[i].y + (double)scaled_off_y;
        rdpat_translated_points[i].z = rd_data.points[i].z + (double)scaled_off_z;
        rdpat_translated_points[i].r = rd_data.points[i].r;
      }

      points_to_use = rdpat_translated_points;
    }

    /* Regenerate triangles on data change or translation state change or scale change */
    if( current_gen != rdpat_last_ff_gen ||
        translate_to_excitation != rdpat_last_translate_state ||
        (translate_to_excitation && rdpat_ovl_scale_adj != rdpat_last_ovl_scale_adj) )
    {
      int tri_count = opengl_rdpattern_generate_triangles(
          points_to_use, rd_data.nth, rd_data.nph, r_min, r_range);

      if( tri_count <= 0 )
        return( FALSE );

      rdpat_ff_generation++;
      rdpat_last_ff_gen = current_gen;
      rdpat_last_translate_state = translate_to_excitation;
      rdpat_last_ovl_scale_adj = rdpat_ovl_scale_adj;
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

  free_ptr((void **)&rdpat_translated_points);
  rdpat_translated_capacity = 0;
}

/*-----------------------------------------------------------------------*/

/* rdpattern_on_shift_scroll()
 *
 * Shift+scroll handler for adjusting overlay structure scale
 */
  static gboolean
rdpattern_on_shift_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer view_state)
{
  gl_view_state_t *state;
  double scale;

  state = (gl_view_state_t *)view_state;

  if( !state || isFlagClear(OVERLAY_STRUCT) )
    return( FALSE );

  /* Scale adjustment only applies to far-field (gain) view */
  if( isFlagClear(DRAW_GAIN) )
    return( FALSE );

  /* Compute scale factor matching zoom behavior */
  scale = compute_zoom_scale(
      (int)(state->viewport_height * state->aspect),
      (int)state->viewport_height,
      state->ovl_model_scale_adj * 100.0);

  if( event->direction == GDK_SCROLL_UP )
    state->ovl_model_scale_adj *= (1.0 + 0.1 * scale);
  else if( event->direction == GDK_SCROLL_DOWN )
    state->ovl_model_scale_adj /= (1.0 + 0.1 * scale);
  else
    return( FALSE );

  /* Cache for use in translation calculation */
  rdpat_ovl_scale_adj = state->ovl_model_scale_adj;

  gtk_widget_queue_draw(widget);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* rdpattern_overlay_generate()
 *
 * Overlay provider generate callback.
 * Returns structure geometry for overlay when OVERLAY_STRUCT is set.
 * Computes physical scale ratio from structure to radiation pattern.
 */
  static gboolean
rdpattern_overlay_generate(const gl_view_content_t *primary, gl_view_content_t *out)
{
  const structure_overlay_data_t *geom;
  static gboolean tooltip_shown = FALSE;

  if( isFlagClear(OVERLAY_STRUCT) || data.n <= 0 || !primary )
    return( FALSE );

  /* Show tooltip on first activation */
  if( !tooltip_shown && rdpattern_gl_widget )
  {
    gl_view_show_tooltip(rdpattern_gl_widget, "Shift Scroll to Scale Structure", 2500);
    tooltip_shown = TRUE;
  }

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

  /* Scale structure to match radiation pattern space for far-field,
   * use 1:1 for near-field (both already in meters) */
  if( isFlagSet(DRAW_GAIN) )
    out->model_scale = rdpattern_overlay_base_scale(primary->r_max, geom->view_scale);
  else
    out->model_scale = 1.0f;

  /* Unused by overlay rendering (shares primary camera) */
  out->r_max = 0.0f;
  out->zoom = 1.0f;
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
  .overlay_cleanup = NULL,
  .on_shift_scroll = rdpattern_on_shift_scroll
};

/*-----------------------------------------------------------------------*/

/* rdpattern_arcball_changed_cb()
 *
 * Arcball change callback for constrained rotation mode.
 * Syncs arcball WR/WI angles back to rdpattern_proj_params and
 * spin button display text when COMMON_PROJECTION is off.
 */
  static void
rdpattern_arcball_changed_cb(arcball_state_t *ab, gpointer _user_data)
{
  float wr, wi;

  (void)_user_data;

  if( arcball_get_drag_mode(ab) != ARCBALL_DRAG_CONSTRAINED )
    return;

  /* When common projection is active, the structure arcball is
   * the single point of truth — structure callback handles sync */
  if( isFlagSet(COMMON_PROJECTION) )
    return;

  arcball_get_angles(ab, &wr, &wi);

  rdpattern_proj_params.Wr = (double)wr;
  rdpattern_proj_params.Wi = (double)wi;

  opengl_update_spin_display( rotate_rdpattern, rdpattern_proj_params.Wr );
  opengl_update_spin_display( incline_rdpattern, rdpattern_proj_params.Wi );
}

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_create_widget()
 *
 * Create the OpenGL radiation pattern widget using the generic view engine
 */
  GtkWidget*
opengl_rdpattern_create_widget(void)
{
  if( !rdpattern_arcball )
  {
    rdpattern_arcball = arcball_new();

    /* Initialize mode from rc_config */
    arcball_set_drag_mode(rdpattern_arcball,
        rc_config.arcball_constrained_rotation ?
        ARCBALL_DRAG_CONSTRAINED : ARCBALL_DRAG_FREE);

    /* Sync constrained rotation back to WR/WI and spin buttons */
    arcball_add_callback(rdpattern_arcball,
        rdpattern_arcball_changed_cb, NULL);
  }

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
