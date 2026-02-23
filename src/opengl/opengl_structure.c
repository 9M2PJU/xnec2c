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
#include "../shared.h"
#include "../draw.h"

#ifdef HAVE_OPENGL

#include "../opengl-engine/opengl_view.h"
#include "../opengl-engine/opengl_cylinder.h"

/* Default scale factor for cylinder radius display */
#define CYLINDER_RADIUS_SCALE_DEFAULT 1.0

/* Scale below which segments render as lines instead of cylinders */
#define CYLINDER_SCALE_LINE_THRESHOLD 1.0

/* Minimum radius as fraction of model extent for visible cylinders */
#define CYLINDER_MIN_VISIBLE_FRACTION 0.0015

/* Maximum allowed radius scale */
#define CYLINDER_RADIUS_SCALE_MAX 100.0

/* Number of sides for cylinder rendering */
#define STRUCTURE_CYLINDER_SEGMENTS 24

/* Mutable cylinder radius scale factor (user-adjustable via Ctrl+scroll) */
static double cylinder_radius_scale = CYLINDER_RADIUS_SCALE_DEFAULT;

/* Shared geometry buffer accessible by both structure window and overlay */
static lit_color_point_t *structure_vertices = NULL;
static int structure_vertex_count = 0;
static unsigned int structure_geometry_generation = 1;
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;
static float structure_view_scale = 1.0f;
static GLenum structure_draw_mode = GL_TRIANGLES;

/* Track previous radius scale to detect changes requiring regeneration */
static double structure_last_radius_scale = CYLINDER_RADIUS_SCALE_DEFAULT;

/* Tooltip shown once per session */
static gboolean ctrl_scroll_tooltip_shown = FALSE;

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

/* opengl_structure_get_radius_scale()
 *
 * Return current cylinder radius display scale factor
 */
  double
opengl_structure_get_radius_scale(void)
{
  return( cylinder_radius_scale );

} /* opengl_structure_get_radius_scale() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_set_radius_scale()
 *
 * Set cylinder radius display scale factor and sync to rc_config.
 * Bumps geometry generation to force regeneration on next render.
 */
  void
opengl_structure_set_radius_scale(double scale)
{
  if( scale < 0.0 )
  {
    scale = 0.0;
  }

  if( scale > CYLINDER_RADIUS_SCALE_MAX )
  {
    scale = CYLINDER_RADIUS_SCALE_MAX;
  }

  cylinder_radius_scale = scale;
  rc_config.opengl_cylinder_radius_scale = scale;

} /* opengl_structure_set_radius_scale() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_on_ctrl_scroll()
 *
 * Ctrl+scroll handler for adjusting cylinder radius scale.
 * Shared by structure and rdpattern scene providers.
 */
  gboolean
opengl_structure_on_ctrl_scroll(
    GtkWidget *widget, GdkEventScroll *event, gpointer view_state)
{
  gl_view_state_t *state;
  double scale, new_scale;

  state = (gl_view_state_t *)view_state;

  if( !state )
    return( FALSE );

  /* Compute increment matching zoom scroll feel */
  scale = compute_zoom_scale(
      (int)(state->viewport_height * state->aspect),
      (int)state->viewport_height,
      (cylinder_radius_scale > 0.1 ? cylinder_radius_scale : 0.1) * 100.0);

  new_scale = cylinder_radius_scale;

  if( event->direction == GDK_SCROLL_UP )
  {
    /* Scroll up: thicker. If at zero, jump to threshold. */
    if( new_scale < CYLINDER_SCALE_LINE_THRESHOLD )
    {
      new_scale = CYLINDER_SCALE_LINE_THRESHOLD;
    }
    else
    {
      new_scale *= (1.0 + 0.1 * scale);
    }
  }
  else if( event->direction == GDK_SCROLL_DOWN )
  {
    /* Scroll down: thinner. Below threshold snaps to zero (line mode). */
    new_scale /= (1.0 + 0.1 * scale);

    if( new_scale < CYLINDER_SCALE_LINE_THRESHOLD )
    {
      new_scale = 0.0;
    }
  }
  else
  {
    return( FALSE );
  }

  opengl_structure_set_radius_scale(new_scale);

  /* Show tooltip on first use */
  if( !ctrl_scroll_tooltip_shown )
  {
    gl_view_show_tooltip(widget, "Ctrl+Scroll: Wire Radius", 2500);
    ctrl_scroll_tooltip_shown = TRUE;
  }

  /* Queue redraw on the event source widget */
  gtk_widget_queue_draw(widget);

  /* Queue redraw on cross-window GL areas */
  if( structure_gl_widget && widget != structure_gl_widget )
    gtk_widget_queue_draw(structure_gl_widget);

  if( rdpattern_gl_area && widget != rdpattern_gl_area )
    gtk_widget_queue_draw(rdpattern_gl_area);

  return( TRUE );

} /* opengl_structure_on_ctrl_scroll() */

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

/* calculate_excitation_center()
 *
 * Calculate the 3D center point of all excitation segments
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

/* opengl_structure_generate_geometry()
 *
 * Generate cylinder geometry for antenna wire segments
 */
  static int
opengl_structure_generate_geometry(structure_draw_mode_t mode)
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
      structure_vertices[vidx].point.x = (float)data.x1[idx];
      structure_vertices[vidx].point.y = (float)data.y1[idx];
      structure_vertices[vidx].point.z = (float)data.z1[idx];
      structure_vertices[vidx].normal.x = nx;
      structure_vertices[vidx].normal.y = ny;
      structure_vertices[vidx].normal.z = nz;
      structure_vertices[vidx].color.r = r;
      structure_vertices[vidx].color.g = g;
      structure_vertices[vidx].color.b = b;
      structure_vertices[vidx].color.a = 1.0f;
      vidx++;

      /* Endpoint 2 */
      structure_vertices[vidx].point.x = (float)data.x2[idx];
      structure_vertices[vidx].point.y = (float)data.y2[idx];
      structure_vertices[vidx].point.z = (float)data.z2[idx];
      structure_vertices[vidx].normal.x = nx;
      structure_vertices[vidx].normal.y = ny;
      structure_vertices[vidx].normal.z = nz;
      structure_vertices[vidx].color.r = r;
      structure_vertices[vidx].color.g = g;
      structure_vertices[vidx].color.b = b;
      structure_vertices[vidx].color.a = 1.0f;
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

      vidx = opengl_lit_cylinder_append(&mesh, vidx,
          data.x1[idx], data.y1[idx], data.z1[idx],
          data.x2[idx], data.y2[idx], data.z2[idx],
          radius,
          STRUCTURE_CYLINDER_SEGMENTS,
          r, g, b, 1.0f);
    }

    structure_draw_mode = GL_TRIANGLES;
  }

  structure_vertex_count = vidx;
  structure_last_radius_scale = cylinder_radius_scale;
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
      cylinder_radius_scale != structure_last_radius_scale ||
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
  out->draw_mode = structure_draw_mode;
  out->r_max = (structure_vertex_count > 0) ? structure_view_scale : 1.5f;
  out->clip_extent = out->r_max;
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
  .cleanup = structure_scene_cleanup,
  .on_ctrl_scroll = opengl_structure_on_ctrl_scroll
};

/*-----------------------------------------------------------------------*/

/* opengl_update_spin_display()
 *
 * Update spin button display text without emitting value_changed signal
 */
  void
opengl_update_spin_display(GtkSpinButton *spin, double angle)
{
  gchar value[6];

  snprintf( value, sizeof(value), "%d", (int)angle );
  gtk_entry_set_text( GTK_ENTRY(spin), value );
}

/*-----------------------------------------------------------------------*/

/* structure_arcball_changed_cb()
 *
 * Arcball change callback for constrained rotation mode.
 * Syncs arcball WR/WI angles back to structure_proj_params and
 * spin button display text, matching Cairo's Motion_Event behavior.
 */
  static void
structure_arcball_changed_cb(arcball_state_t *ab, gpointer _user_data)
{
  float wr, wi;

  (void)_user_data;

  if( arcball_get_drag_mode(ab) != ARCBALL_DRAG_CONSTRAINED )
    return;

  arcball_get_angles(ab, &wr, &wi);

  structure_proj_params.Wr = (double)wr;
  structure_proj_params.Wi = (double)wi;

  opengl_update_spin_display( rotate_structure, structure_proj_params.Wr );
  opengl_update_spin_display( incline_structure, structure_proj_params.Wi );

  /* Sync rdpattern spin buttons when common projection is active */
  if( isFlagSet(DRAW_ENABLED) && isFlagSet(COMMON_PROJECTION) )
  {
    rdpattern_proj_params.Wr = structure_proj_params.Wr;
    rdpattern_proj_params.Wi = structure_proj_params.Wi;

    opengl_update_spin_display( rotate_rdpattern, rdpattern_proj_params.Wr );
    opengl_update_spin_display( incline_rdpattern, rdpattern_proj_params.Wi );
  }
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_create_widget_impl()
 *
 * Create the OpenGL structure widget using the generic view engine
 */
  static GtkWidget*
opengl_structure_create_widget_impl(void)
{
  /* Return existing widget if already created */
  if( structure_gl_widget )
    return( structure_gl_widget );

  /* Load persisted radius scale from config.
   * Treat sub-threshold values as unset (upgrade from old config). */
  cylinder_radius_scale = rc_config.opengl_cylinder_radius_scale;
  if( cylinder_radius_scale < CYLINDER_SCALE_LINE_THRESHOLD )
  {
    cylinder_radius_scale = CYLINDER_RADIUS_SCALE_DEFAULT;
  }
  structure_last_radius_scale = cylinder_radius_scale;

  if( !structure_arcball )
  {
    structure_arcball = arcball_new();

    /* Initialize mode from rc_config */
    arcball_set_drag_mode(structure_arcball,
        rc_config.arcball_constrained_rotation ?
        ARCBALL_DRAG_CONSTRAINED : ARCBALL_DRAG_FREE);

    /* Sync constrained rotation back to WR/WI and spin buttons */
    arcball_add_callback(structure_arcball,
        structure_arcball_changed_cb, NULL);
  }

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
