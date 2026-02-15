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

#include "opengl_structure.h"
#include "shared.h"
#include "draw.h"

#ifdef HAVE_OPENGL

#include "opengl_view.h"
#include "opengl_cylinder.h"

/* Scale factor for cylinder radius display */
#define CYLINDER_RADIUS_SCALE 5.0

/* Number of sides for cylinder rendering */
#define STRUCTURE_CYLINDER_SEGMENTS 24

/* Shared geometry buffer accessible by both structure window and overlay */
static lit_color_point_t *structure_vertices = NULL;
static int structure_vertex_count = 0;
static unsigned int structure_geometry_generation = 1;
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;
static float structure_view_scale = 1.0f;

/* Public accessor for shared geometry */
static structure_overlay_data_t shared_overlay_data = { 0 };

/* Widget pointer for queue_redraw */
static GtkWidget *structure_gl_widget = NULL;

/* Arcball for structure view */
static arcball_state_t *structure_arcball = NULL;

/* Vertex attribute layout for lit-color shader (shared with overlay consumers) */
const gl_vertex_attrib_t opengl_structure_attribs[3] = {
  { "position", 3, 0 },
  { "normal",   3, 4 * (int)sizeof(float) },
  { "color",    4, 8 * (int)sizeof(float) }
};

/*-----------------------------------------------------------------------*/

/* opengl_structure_get_draw_mode()
 *
 * Derive draw mode from global flags
 */
  static structure_draw_mode_t
opengl_structure_get_draw_mode(void)
{
  if( isFlagSet(DRAW_CURRENTS) )
    return( STRUCTURE_DRAW_CURRENTS );

  if( isFlagSet(DRAW_CHARGES) )
    return( STRUCTURE_DRAW_CHARGES );

  return( STRUCTURE_DRAW_GEOMETRY );
}

/*-----------------------------------------------------------------------*/

/* get_segment_magnitude()
 *
 * Compute current or charge magnitude for a segment
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

/* get_segment_color()
 *
 * Determine color for a segment based on type and draw mode.
 * GEOMETRY mode uses shared get_segment_color_type() from draw.c
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

/* opengl_structure_generate_geometry()
 *
 * Generate cylinder geometry for antenna wire segments
 */
  static int
opengl_structure_generate_geometry(structure_draw_mode_t mode)
{
  int idx, vidx;
  int total_vertices;
  int verts_per_seg;
  size_t mreq;
  double cmax;
  double r_max;
  float r, g, b;

  if( data.n <= 0 )
  {
    structure_vertex_count = 0;
    structure_view_scale = 1.0f;
    return( 0 );
  }

  /* Calculate vertices needed */
  verts_per_seg = opengl_cylinder_vertex_count(STRUCTURE_CYLINDER_SEGMENTS);
  total_vertices = data.n * verts_per_seg;

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

  /* Generate cylinder for each wire segment */
  vidx = 0;
  for( idx = 0; idx < data.n; idx++ )
  {
    lit_cylinder_mesh_t mesh;

    mesh.vertices = structure_vertices;
    mesh.vertex_count = total_vertices;

    get_segment_color(idx, mode, cmax, &r, &g, &b);

    vidx = opengl_lit_cylinder_append(&mesh, vidx,
        data.x1[idx], data.y1[idx], data.z1[idx],
        data.x2[idx], data.y2[idx], data.z2[idx],
        data.bi[idx] * CYLINDER_RADIUS_SCALE,
        STRUCTURE_CYLINDER_SEGMENTS,
        r, g, b, 1.0f);
  }

  structure_vertex_count = vidx;
  structure_geometry_generation++;

  return( structure_vertex_count );
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_update_shared_geometry()
 *
 * Check staleness of shared geometry buffer and regenerate if needed.
 * Called by both structure window and rdpattern overlay before rendering.
 */
  void
opengl_structure_update_shared_geometry(void)
{
  structure_draw_mode_t current_mode;

  current_mode = opengl_structure_get_draw_mode();

  /* Check if current mode requires current/charge data */
  gboolean is_current_mode =
    (current_mode == STRUCTURE_DRAW_CURRENTS ||
     current_mode == STRUCTURE_DRAW_CHARGES);

  /* Regenerate on mode change, empty buffer, or new current data */
  if( current_mode != structure_last_mode ||
      structure_vertex_count == 0 ||
      (is_current_mode && crnt.newer) )
  {
    structure_last_mode = current_mode;
    opengl_structure_generate_geometry(current_mode);

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
    shared_overlay_data.generation = structure_geometry_generation;
  }
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_get_shared_geometry()
 *
 * Return read-only pointer to shared structure geometry data
 */
  const structure_overlay_data_t*
opengl_structure_get_shared_geometry(void)
{
  return( &shared_overlay_data );
}

/*-----------------------------------------------------------------------*/

/* structure_scene_generate()
 *
 * Scene provider generate callback.
 * Delegates to shared geometry updater, then fills view content.
 */
  static gboolean
structure_scene_generate(gl_view_content_t *out)
{
  float zoom;

  opengl_structure_update_shared_geometry();

  /* Zoom from structure spinbutton, normalized to multiplier */
  zoom = 1.0f;
  if( structure_zoom )
    zoom = (float)gtk_spin_button_get_value(structure_zoom) / 100.0f;

  if( zoom < 0.01f )
    zoom = 0.01f;

  out->vertices = structure_vertices;
  out->vertex_count = structure_vertex_count;
  out->vertex_stride = (int)sizeof(lit_color_point_t);
  out->draw_mode = GL_TRIANGLES;
  out->r_max = (structure_vertex_count > 0) ? structure_view_scale : 1.5f;
  out->zoom = zoom;
  out->model_scale = 1.0f;
  out->show_gradient = FALSE;
  out->generation = structure_geometry_generation;

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* structure_scene_cleanup()
 *
 * Scene provider cleanup callback
 */
  static void
structure_scene_cleanup(void)
{
  free_ptr((void **)&structure_vertices);
  structure_vertex_count = 0;
}

/*-----------------------------------------------------------------------*/

/* Static scene configuration and provider */
static gl_view_config_t structure_view_config = {
  .vertex_shader_path = "/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "/gl/lit-color-fragment.glsl",
  .attribs = opengl_structure_attribs,
  .attrib_count = 3,
  .vertex_stride = (int)sizeof(lit_color_point_t),
  .has_gradient = FALSE,
  .gradient_draw = NULL
};

static gl_scene_provider_t structure_scene_provider = {
  .generate = structure_scene_generate,
  .post_render = NULL,
  .cleanup = structure_scene_cleanup
};

/*-----------------------------------------------------------------------*/

/* opengl_structure_create_widget_impl()
 *
 * Create the OpenGL structure widget using the generic view engine
 */
  static GtkWidget*
opengl_structure_create_widget_impl(void)
{
  if( !structure_arcball )
    structure_arcball = arcball_new();

  /* Initialize arcball from current projection angles */
  arcball_set_view(structure_arcball,
      (float)structure_proj_params.Wr,
      (float)structure_proj_params.Wi);

  structure_gl_widget = gl_view_create_widget(
      &structure_view_config,
      &structure_scene_provider,
      structure_arcball,
      &structure_zoom);

  return( structure_gl_widget );
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_get_arcball()
 *
 * Return reference to structure arcball
 */
  arcball_state_t*
opengl_structure_get_arcball(void)
{
  return( structure_arcball );
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_cleanup_impl()
 *
 * Cleanup structure GL resources
 */
  static void
opengl_structure_cleanup_impl(void)
{
  structure_scene_cleanup();
  structure_gl_widget = NULL;
}

#endif /* HAVE_OPENGL */

/*-----------------------------------------------------------------------*/

/* opengl_structure_create_widget()
 *
 * Public API - create structure GL widget
 */
  GtkWidget*
opengl_structure_create_widget(void)
{
#ifdef HAVE_OPENGL
  return( opengl_structure_create_widget_impl() );
#else
  return( NULL );
#endif
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_cleanup()
 *
 * Public API - cleanup resources
 */
  void
opengl_structure_cleanup(void)
{
#ifdef HAVE_OPENGL
  opengl_structure_cleanup_impl();
#endif
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_queue_draw()
 *
 * Public API - queue redraw of OpenGL structure widget
 */
  void
opengl_structure_queue_draw(void)
{
#ifdef HAVE_OPENGL
  if( structure_gl_widget != NULL )
    xnec2_widget_queue_draw( structure_gl_widget );
#endif
}
