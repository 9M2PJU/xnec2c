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
#include "opengl_cylinder.h"
#include "draw.h"

#ifdef HAVE_OPENGL

/* Scale factor for cylinder radius display */
#define CYLINDER_RADIUS_SCALE 5.0

/* Number of sides for cylinder rendering */
#define STRUCTURE_CYLINDER_SEGMENTS 24

/* Vertex buffer for structure geometry */
static lit_color_point_t *structure_vertices = NULL;
static int structure_vertex_count = 0;
static int structure_geometry_generation = 1;
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;

/* Structure scale factor for view */
static float structure_view_scale = 1.0f;

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

/* structure_gl_state_new()
 *
 * Allocate and initialize structure GL state
 */
  static structure_gl_state_t*
structure_gl_state_new(float aspect)
{
  structure_gl_state_t *state;

  state = g_new0(structure_gl_state_t, 1);

  state->gl = gl_instance_new("/gl/lit-color-vertex.glsl", "/gl/lit-color-fragment.glsl",
    6.0f, aspect);

  if( !state->gl )
  {
    g_free(state);
    return( NULL );
  }

  state->position_location = glGetAttribLocation(
    state->gl->shader.program, "position");
  state->normal_location = glGetAttribLocation(
    state->gl->shader.program, "normal");
  state->color_location = glGetAttribLocation(
    state->gl->shader.program, "color");
  state->vertex_count = 0;
  state->geometry_generation = 0;

  state->axes = opengl_axes_new(&state->gl->shader);
  if( !state->axes )
  {
    pr_err("Failed to create axes renderer for structure\n");
    gl_instance_free(state->gl);
    g_free(state);
    return( NULL );
  }

  state->initialized = TRUE;

  return( state );
}

/*-----------------------------------------------------------------------*/

/* structure_gl_state_free()
 *
 * Free structure GL state
 */
  static void
structure_gl_state_free(structure_gl_state_t *state)
{
  if( !state )
    return;

  opengl_axes_free(state->axes);
  gl_instance_free(state->gl);
  g_free(state);
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

/* opengl_structure_update_buffers()
 *
 * Upload vertex data to GPU
 */
  static void
opengl_structure_update_buffers(structure_gl_state_t *state)
{
  size_t buffer_size;

  if( !state || !state->gl || !state->gl->initialized ||
      !structure_vertices || structure_vertex_count == 0 )
    return;

  glBindVertexArray(state->gl->vao);
  glBindBuffer(GL_ARRAY_BUFFER, state->gl->vbo);

  buffer_size = (size_t)structure_vertex_count * sizeof(lit_color_point_t);
  glBufferData(GL_ARRAY_BUFFER, buffer_size, structure_vertices, GL_DYNAMIC_DRAW);

  /* Position attribute: offset 0 */
  glEnableVertexAttribArray(state->position_location);
  glVertexAttribPointer(
      state->position_location,
      3, GL_FLOAT, GL_FALSE,
      sizeof(lit_color_point_t),
      (void*)0);

  /* Normal attribute: offset after point (4 floats due to padding) */
  glEnableVertexAttribArray(state->normal_location);
  glVertexAttribPointer(
      state->normal_location,
      3, GL_FLOAT, GL_FALSE,
      sizeof(lit_color_point_t),
      (void*)(4 * sizeof(float)));

  /* Color attribute: offset after point and normal (8 floats) */
  glEnableVertexAttribArray(state->color_location);
  glVertexAttribPointer(
      state->color_location,
      4, GL_FLOAT, GL_FALSE,
      sizeof(lit_color_point_t),
      (void*)(8 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  state->vertex_count = structure_vertex_count;
  state->geometry_generation = structure_geometry_generation;
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_get_state()
 *
 * Get GL state from widget
 */
  structure_gl_state_t*
opengl_structure_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );
}

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler
 */
  static void
on_realize(GtkGLArea *area)
{
  structure_gl_state_t *state;
  GtkAllocation alloc;
  float aspect;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error in structure\n");
    return;
  }

  gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
  aspect = (float)alloc.width / (float)alloc.height;

  state = structure_gl_state_new(aspect);
  if( !state )
  {
    pr_err("Failed to create structure GL state\n");
    return;
  }

  arcball_set_viewport(state->gl->arcball, (float)alloc.height);

  /* Initialize view from structure projection params */
  arcball_set_view(state->gl->arcball,
      (float)structure_proj_params.Wr,
      (float)structure_proj_params.Wi);

  g_object_set_data_full(G_OBJECT(area), "gl_state", state,
    (GDestroyNotify)structure_gl_state_free);

  pr_notice("Structure OpenGL initialized (Wr=%.1f, Wi=%.1f)\n",
      structure_proj_params.Wr, structure_proj_params.Wi);
}

/*-----------------------------------------------------------------------*/

/* on_render()
 *
 * GtkGLArea render signal handler
 */
  static gboolean
on_render(GtkGLArea *area, GdkGLContext *context)
{
  structure_gl_state_t *state;
  mat4 mvp;
  int vertex_count;
  float zoom;
  gboolean need_regenerate;
  structure_draw_mode_t current_mode;

  state = g_object_get_data(G_OBJECT(area), "gl_state");
  if( !state || !state->gl || !state->gl->initialized )
    return( FALSE );

  current_mode = opengl_structure_get_draw_mode();

  /* Check if geometry needs regeneration */
  need_regenerate = (state->geometry_generation != structure_geometry_generation) ||
                    (structure_last_mode != current_mode);

  if( need_regenerate )
  {
    structure_last_mode = current_mode;
    vertex_count = opengl_structure_generate_geometry(current_mode);
  }
  else
  {
    vertex_count = structure_vertex_count;
  }

  /* Get zoom from main window spinbutton */
  zoom = 1.0f;
  if( structure_zoom )
    zoom = (float)gtk_spin_button_get_value(structure_zoom) / 100.0f;

  if( vertex_count <= 0 )
  {
    /* No geometry - clear and draw axes */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    arcball_set_zoom_factor(state->gl->arcball, 4.0f, zoom);
    opengl_axes_set_scale(state->axes, 1.5f);
    arcball_get_mvp(state->gl->arcball, mvp, zoom);
    opengl_axes_render(state->axes, mvp);

    return( TRUE );
  }

  opengl_structure_update_buffers(state);

  if( state->vertex_count == 0 )
    return( FALSE );

  /* Set distance and axes scale based on structure size */
  arcball_set_zoom_factor(state->gl->arcball, structure_view_scale * 4.0f, zoom);
  opengl_axes_set_scale(state->axes, structure_view_scale * 1.1f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  arcball_get_mvp(state->gl->arcball, mvp, zoom);

  glUseProgram(state->gl->shader.program);
  glUniformMatrix4fv(state->gl->mvp_location, 1, GL_FALSE, (float*)mvp);

  glBindVertexArray(state->gl->vao);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glDrawArrays(GL_TRIANGLES, 0, state->vertex_count);

  glBindVertexArray(0);

  opengl_axes_render(state->axes, mvp);

  glUseProgram(0);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* on_unrealize()
 *
 * GtkGLArea unrealize signal handler
 */
  static void
on_unrealize(GtkGLArea *area)
{
  gl_area_cleanup_state(area, "gl_state",
      (void(*)(void*))structure_gl_state_free);
}

/*-----------------------------------------------------------------------*/

/* on_button_press()
 *
 * Mouse button press handler
 */
  static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  structure_gl_state_t *state;

  state = opengl_structure_get_state(widget);
  if( !state || !state->gl )
    return( FALSE );

  arcball_begin_drag(state->gl->arcball, event->button, event->x, event->y);
  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* on_button_release()
 *
 * Mouse button release handler
 */
  static gboolean
on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  structure_gl_state_t *state;

  state = opengl_structure_get_state(widget);
  if( !state || !state->gl )
    return( FALSE );

  arcball_end_drag(state->gl->arcball);
  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* on_motion()
 *
 * Mouse motion handler - uses arcball for rotation (matches rdpattern)
 */
  static gboolean
on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  structure_gl_state_t *state;

  state = opengl_structure_get_state(widget);
  if( !state || !state->gl )
    return( FALSE );

  if( !(event->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK)) )
    return( FALSE );

  arcball_drag(state->gl->arcball, event->x, event->y);
  gtk_widget_queue_draw(widget);
  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* on_scroll()
 *
 * Mouse scroll handler - updates zoom (matches rdpattern pattern)
 */
  static gboolean
on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
  if( !structure_zoom )
    return( FALSE );

  structure_proj_params.xy_zoom = gtk_spin_button_get_value(structure_zoom);

  if( event->direction == GDK_SCROLL_UP )
    structure_proj_params.xy_zoom *= 1.1;
  else if( event->direction == GDK_SCROLL_DOWN )
    structure_proj_params.xy_zoom /= 1.1;
  else
    return( FALSE );

  gtk_spin_button_set_value(structure_zoom, structure_proj_params.xy_zoom);
  return( FALSE );
}

/*-----------------------------------------------------------------------*/

/* on_resize()
 *
 * Widget resize handler
 */
  static void
on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
  structure_gl_state_t *state;
  float aspect;

  state = opengl_structure_get_state(widget);
  if( !state || !state->gl )
    return;

  aspect = (float)allocation->width / (float)allocation->height;
  arcball_set_aspect(state->gl->arcball, aspect);
  arcball_set_viewport(state->gl->arcball, (float)allocation->height);
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_create_widget_impl()
 *
 * Create the OpenGL structure widget for main window
 */
  static GtkWidget*
opengl_structure_create_widget_impl(void)
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

  return( gl_area );
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_queue_redraw_impl()
 *
 * Request redraw of structure GL widget
 */
  static void
opengl_structure_queue_redraw_impl(void)
{
  if( structure_gl_area != NULL )
  {
    structure_geometry_generation++;
    xnec2_widget_queue_draw(structure_gl_area);
  }
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_cleanup_impl()
 *
 * Cleanup structure GL resources
 */
  static void
opengl_structure_cleanup_impl(void)
{
  free_ptr((void **)&structure_vertices);
  structure_vertex_count = 0;
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

/* opengl_structure_queue_redraw()
 *
 * Public API - request redraw
 */
  void
opengl_structure_queue_redraw(void)
{
#ifdef HAVE_OPENGL
  opengl_structure_queue_redraw_impl();
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
