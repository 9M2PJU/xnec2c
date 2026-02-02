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
#include "shared.h"
#include "draw.h"
#include "draw_radiation.h"
#include "opengl_axes.h"

#ifdef HAVE_OPENGL

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

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_state_new()
 *
 * Allocate and initialize radiation pattern GL state
 */
  rdpattern_gl_state_t*
opengl_rdpattern_state_new(float aspect)
{
  rdpattern_gl_state_t *state;

  state = g_new0(rdpattern_gl_state_t, 1);

  state->gl = gl_instance_new("/gl/color-vertex.glsl", "/gl/color-fragment.glsl",
    6.0f, aspect);

  if( !state->gl )
  {
    g_free(state);
    return( NULL );
  }

  state->position_location = glGetAttribLocation(
    state->gl->shader.program, "position");
  state->color_location = glGetAttribLocation(
    state->gl->shader.program, "color");
  state->triangle_count = 0;
  state->gl_last_gen = 0;

  state->axes = opengl_axes_new(&state->gl->shader);
  if( !state->axes )
  {
    pr_err("Failed to create axes renderer\n");
    gl_instance_free(state->gl);
    g_free(state);
    return NULL;
  }

  state->overlay = gradient_overlay_new();
  if( !state->overlay )
  {
    pr_err("Failed to create gradient overlay\n");
    opengl_axes_free(state->axes);
    gl_instance_free(state->gl);
    g_free(state);
    return NULL;
  }

  return( state );

} /* opengl_rdpattern_state_new() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_state_free()
 *
 * Free radiation pattern GL state
 */
  void
opengl_rdpattern_state_free(rdpattern_gl_state_t *state)
{
  if( !state )
    return;

  gradient_overlay_free(state->overlay);
  opengl_axes_free(state->axes);
  gl_instance_free(state->gl);
  g_free(state);

} /* opengl_rdpattern_state_free() */

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

  return( nf_line_count );

} /* opengl_rdpattern_generate_nf_lines() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_update_nf_buffers()
 *
 * Upload near-field line data to GPU
 */
  static void
opengl_rdpattern_update_nf_buffers(rdpattern_gl_state_t *state)
{
  size_t buffer_size;

  if( !state || !state->gl || !state->gl->initialized || !nf_lines || nf_line_count == 0 )
    return;

  glBindVertexArray(state->gl->vao);
  glBindBuffer(GL_ARRAY_BUFFER, state->gl->vbo);

  buffer_size = (size_t)nf_line_count * 2 * sizeof(color_point_t);
  glBufferData(GL_ARRAY_BUFFER, buffer_size, nf_lines, GL_DYNAMIC_DRAW);

  /* Position attribute */
  glEnableVertexAttribArray(state->position_location);
  glVertexAttribPointer(
      state->position_location,
      3, GL_FLOAT, GL_FALSE,
      sizeof(color_point_t),
      (void*)0);

  /* Color attribute */
  glEnableVertexAttribArray(state->color_location);
  glVertexAttribPointer(
      state->color_location,
      4, GL_FLOAT, GL_FALSE,
      sizeof(color_point_t),
      (void*)(4 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  state->line_count = nf_line_count;

} /* opengl_rdpattern_update_nf_buffers() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_generate_triangles()
 *
 * Tessellate point_3d buffer into colored triangles
 */
  int
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

  pr_debug("Generating %d triangles (nth=%d, nph=%d)\n",
    rdpat_triangle_count, nth, nph);

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

/* opengl_rdpattern_update_buffers()
 *
 * Upload triangle data to GPU
 */
  void
opengl_rdpattern_update_buffers(rdpattern_gl_state_t *state)
{
  size_t buffer_size;

  if( !state || !state->gl || !state->gl->initialized || !rdpat_triangles || rdpat_triangle_count == 0 )
    return;

  pr_debug("Updating GL buffers with %d triangles\n", rdpat_triangle_count);

  glBindVertexArray(state->gl->vao);
  glBindBuffer(GL_ARRAY_BUFFER, state->gl->vbo);

  buffer_size = (size_t)rdpat_triangle_count * sizeof(color_triangle_t);
  glBufferData(GL_ARRAY_BUFFER, buffer_size, rdpat_triangles, GL_STATIC_DRAW);

  /* Position attribute */
  glEnableVertexAttribArray(state->position_location);
  glVertexAttribPointer(
      state->position_location,
      3, GL_FLOAT, GL_FALSE,
      sizeof(color_point_t),
      (void*)0);

  /* Color attribute */
  glEnableVertexAttribArray(state->color_location);
  glVertexAttribPointer(
      state->color_location,
      4, GL_FLOAT, GL_FALSE,
      sizeof(color_point_t),
      (void*)(4 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  state->triangle_count = rdpat_triangle_count;

} /* opengl_rdpattern_update_buffers() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_get_state()
 *
 * Get radiation pattern GL state from widget
 */
  rdpattern_gl_state_t*
opengl_rdpattern_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* opengl_rdpattern_get_state() */

/*-----------------------------------------------------------------------*/

/* update_viewport_state()
 *
 * Single point of truth for viewport-dependent state initialization.
 * Updates arcball and overlay dimensions.
 */
  static void
update_viewport_state(rdpattern_gl_state_t *state, int width, int height)
{
  float aspect = (float)width / (float)height;

  arcball_set_aspect(state->gl->arcball, aspect);
  arcball_set_viewport(state->gl->arcball, (float)height);
  gradient_overlay_set_viewport(state->overlay, width, height);

} /* update_viewport_state() */

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler - initializes OpenGL context
 */
  static void
on_realize(GtkGLArea *area)
{
  rdpattern_gl_state_t *state;
  GtkAllocation alloc;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error\n");
    return;
  }

  gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);

  state = opengl_rdpattern_state_new(1.0f);
  if( !state )
  {
    pr_err("Failed to create GL state\n");
    return;
  }

  update_viewport_state(state, alloc.width, alloc.height);

  g_object_set_data_full(G_OBJECT(area), "gl_state", state,
    (GDestroyNotify)opengl_rdpattern_state_free);

  pr_notice("OpenGL context initialized\n");

} /* on_realize() */

/*-----------------------------------------------------------------------*/

/* on_render()
 *
 * GtkGLArea render signal handler
 */
  static gboolean
on_render(GtkGLArea *area, GdkGLContext *context)
{
  rdpattern_gl_state_t *state;
  mat4 mvp;
  float r_max;

  state = g_object_get_data(G_OBJECT(area), "gl_state");
  if( !state || !state->gl || !state->gl->initialized )
    return( FALSE );

  /* Near-field rendering path */
  if( isFlagSet(DRAW_EHFIELD) && isFlagSet(ENABLE_NEAREH) && near_field.valid )
  {
    int line_count;

    line_count = opengl_rdpattern_generate_nf_lines();
    if( line_count <= 0 )
      return( FALSE );

    opengl_rdpattern_update_nf_buffers(state);

    if( state->line_count == 0 )
      return( FALSE );

    /* Set distance only on data change to preserve user zoom */
    static float last_nf_r_max = 0.0f;
    r_max = (float)near_field.r_max;
    if( r_max != last_nf_r_max )
    {
      rdpattern_proj_params.r_max = (double)r_max;
      arcball_set_zoom_factor(state->gl->arcball, r_max * 2.165f,
          (float)rdpattern_proj_params.xy_zoom);
      opengl_axes_set_scale(state->axes, r_max);
      last_nf_r_max = r_max;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    arcball_get_mvp(state->gl->arcball, mvp, 1.0f);

    glUseProgram(state->gl->shader.program);
    glUniformMatrix4fv(state->gl->mvp_location, 1, GL_FALSE, (float*)mvp);

    glBindVertexArray(state->gl->vao);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glDrawArrays(GL_LINES, 0, state->line_count * 2);

    glBindVertexArray(0);

    opengl_axes_render(state->axes, mvp);

    glUseProgram(0);

    Update_Rdpattern_UI();

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
    {
      pr_err("Failed to get radiation pattern data\n");
      return( FALSE );
    }

    r_max = (float)rd_data.r_max;

    if( current_gen != state->gl_last_gen )
    {
      int tri_count = opengl_rdpattern_generate_triangles(
          rd_data.points, rd_data.nth, rd_data.nph, r_min, r_range);

      if( tri_count > 0 )
      {
        opengl_rdpattern_update_buffers(state);

        float base_distance = r_max * 2.165f;
        rdpattern_proj_params.r_max = (double)r_max;
        arcball_set_zoom_factor(state->gl->arcball, base_distance,
            (float)rdpattern_proj_params.xy_zoom);

        gradient_overlay_mark_dirty(state->overlay);

        state->gl_last_gen = current_gen;
      }
      else
      {
        pr_err("Triangle generation failed\n");
        return( FALSE );
      }
    }

    if( state->triangle_count == 0 )
      return( FALSE );

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    arcball_get_mvp(state->gl->arcball, mvp, 1.0f);

    glUseProgram(state->gl->shader.program);
    glUniformMatrix4fv(state->gl->mvp_location, 1, GL_FALSE, (float*)mvp);

    glBindVertexArray(state->gl->vao);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glDrawArrays(GL_TRIANGLES, 0, state->triangle_count * 3);

    glBindVertexArray(0);

    opengl_axes_set_scale(state->axes, r_max);
    opengl_axes_render(state->axes, mvp);

    glUseProgram(0);

    if( rc_config.rdpattern_gradient_key )
      gradient_overlay_render(state->overlay);

    Update_Rdpattern_UI();

    return( TRUE );
  }

  return( FALSE );

} /* on_render() */

/*-----------------------------------------------------------------------*/

/* on_unrealize()
 *
 * GtkGLArea unrealize signal handler - cleanup resources
 */
  static void
on_unrealize(GtkGLArea *area)
{
  gl_area_cleanup_state(area, "gl_state",
      (void(*)(void*))opengl_rdpattern_state_free);

} /* on_unrealize() */

/*-----------------------------------------------------------------------*/

/* on_button_press()
 *
 * Mouse button press handler
 */
  static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  rdpattern_gl_state_t *state;

  state = g_object_get_data(G_OBJECT(widget), "gl_state");
  if( !state || !state->gl )
    return( FALSE );

  arcball_begin_drag(state->gl->arcball, event->button, event->x, event->y);
  return( TRUE );
}

/* on_button_release()
 *
 * Mouse button release handler
 */
  static gboolean
on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  rdpattern_gl_state_t *state;

  state = g_object_get_data(G_OBJECT(widget), "gl_state");
  if( !state || !state->gl )
    return( FALSE );

  arcball_end_drag(state->gl->arcball);
  return( TRUE );
}

/* on_motion()
 *
 * Mouse motion handler for rotation and pan
 */
  static gboolean
on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
  rdpattern_gl_state_t *state;

  state = g_object_get_data(G_OBJECT(widget), "gl_state");
  if( !state || !state->gl )
    return( FALSE );

  if( !(event->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK)) )
    return( FALSE );

  arcball_drag(state->gl->arcball, event->x, event->y);
  gtk_widget_queue_draw(widget);
  return( TRUE );
}

/* on_scroll()
 *
 * Mouse scroll handler for zoom
 */
  static gboolean
on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
  rdpattern_proj_params.xy_zoom =
    gtk_spin_button_get_value( rdpattern_zoom );
  if( event->direction == GDK_SCROLL_UP )
    rdpattern_proj_params.xy_zoom *= 1.1;
  else if( event->direction == GDK_SCROLL_DOWN )
    rdpattern_proj_params.xy_zoom /= 1.1;
  else
    return( FALSE );

  gtk_spin_button_set_value(
      rdpattern_zoom, rdpattern_proj_params.xy_zoom );
  return( FALSE );
}

/* on_resize()
 *
 * Window resize handler to update viewport dimensions
 */
  static void
on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
  rdpattern_gl_state_t *state;

  state = g_object_get_data(G_OBJECT(widget), "gl_state");
  if( !state || !state->gl )
    return;

  update_viewport_state(state, allocation->width, allocation->height);

  gtk_gl_area_queue_render(GTK_GL_AREA(widget));

} /* on_resize() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_create_widget()
 *
 * Creates and configures GtkGLArea widget
 */
  GtkWidget*
opengl_rdpattern_create_widget(void)
{
  GtkWidget *gl_area;

  gl_area = gtk_gl_area_new();

  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), TRUE);
  gtk_widget_set_hexpand(gl_area, TRUE);
  gtk_widget_set_vexpand(gl_area, TRUE);

  gtk_widget_add_events(gl_area,
      GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK |
      GDK_POINTER_MOTION_MASK |
      GDK_SCROLL_MASK);

  g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize), NULL);
  g_signal_connect(gl_area, "render", G_CALLBACK(on_render), NULL);
  g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_unrealize), NULL);
  g_signal_connect(gl_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
  g_signal_connect(gl_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
  g_signal_connect(gl_area, "motion-notify-event", G_CALLBACK(on_motion), NULL);
  g_signal_connect(gl_area, "scroll-event", G_CALLBACK(on_scroll), NULL);
  g_signal_connect(gl_area, "size-allocate", G_CALLBACK(on_resize), NULL);

  gtk_widget_show(gl_area);

  return( gl_area );

} /* opengl_rdpattern_create_widget() */

/*-----------------------------------------------------------------------*/

/* opengl_rdpattern_cleanup()
 *
 * Free resources
 */
  void
opengl_rdpattern_cleanup(void)
{
  free_ptr((void **)&rdpat_triangles);
  rdpat_triangle_count = 0;

  free_ptr((void **)&nf_lines);
  nf_line_count = 0;

  free_ptr((void **)&pov_x);
  free_ptr((void **)&pov_y);
  free_ptr((void **)&pov_z);
  free_ptr((void **)&pov_r);

} /* opengl_rdpattern_cleanup() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
