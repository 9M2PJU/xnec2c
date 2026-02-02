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

#include "opengl_structure_view.h"
#include "shared.h"
#include "opengl_cylinder.h"
#include "draw.h"

#ifdef HAVE_OPENGL

/* Scale factor for cylinder radius display (5x for visibility) */
#define CYLINDER_RADIUS_SCALE 5.0

/* Number of sides for cylinder rendering (higher = smoother) */
#define STRUCTURE_CYLINDER_SEGMENTS 24

/* Vertex buffer for structure geometry (with normals for lighting) */
static lit_color_point_t *structure_vertices = NULL;
static int structure_vertex_count = 0;
static int structure_geometry_generation = 1;  /* Start at 1 to trigger initial generation */
static structure_draw_mode_t structure_last_mode = STRUCTURE_DRAW_GEOMETRY;

/* Structure scale factor for zoom */
static float structure_view_scale = 1.0f;

/* Forward declarations for internal implementations */
static GtkWidget* opengl_structure_view_create_impl(void);
static void opengl_structure_view_destroyed_impl(void);
static void opengl_structure_view_queue_redraw_impl(void);

/*-----------------------------------------------------------------------*/

/* opengl_structure_get_draw_mode()
 *
 * Derive draw mode from global flags (single point of truth)
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

  /* GEOMETRY mode or NULL data pointers: magnitude not applicable */
  return( 0.0 );

} /* get_segment_magnitude() */

/*-----------------------------------------------------------------------*/

/* get_segment_color()
 *
 * Determine color for a segment based on type and draw mode
 */
  static void
get_segment_color(
    int idx,
    structure_draw_mode_t mode,
    double cmax,
    float *r, float *g, float *b)
{
  int i;
  double red, grn, blu;
  double mag;

  /* Current/charge mode: color by magnitude (only if data is valid) */
  if( (mode == STRUCTURE_DRAW_CURRENTS || mode == STRUCTURE_DRAW_CHARGES) &&
      crnt.valid && cmax > 0.0 )
  {
    mag = get_segment_magnitude(idx, mode);
    Value_to_Color(&red, &grn, &blu, mag, cmax);
    *r = (float)red;
    *g = (float)grn;
    *b = (float)blu;
    return;
  }

  /* Check if segment is an excitation source */
  for( i = 0; i < vsorc.nsant; i++ )
  {
    if( vsorc.isant[i] - 1 == idx )
    {
      *r = 1.0f; *g = 0.0f; *b = 0.0f;
      return;
    }
  }

  for( i = 0; i < vsorc.nvqd; i++ )
  {
    if( vsorc.ivqd[i] - 1 == idx )
    {
      *r = 1.0f; *g = 0.0f; *b = 0.0f;
      return;
    }
  }

  /* Check if segment is loaded */
  for( i = 0; i < zload.nldseg; i++ )
  {
    if( zload.ldsegn[i] - 1 == idx )
    {
      *r = 1.0f; *g = 1.0f; *b = 0.0f;
      return;
    }
  }

  /* Default: blue for normal segments */
  *r = 0.0f; *g = 0.0f; *b = 1.0f;

} /* get_segment_color() */

/*-----------------------------------------------------------------------*/

/* structure_gl_state_new()
 *
 * Allocate and initialize structure view GL state
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
    pr_err("Failed to create axes renderer for structure view\n");
    gl_instance_free(state->gl);
    g_free(state);
    return( NULL );
  }

  state->initialized = TRUE;

  return( state );

} /* structure_gl_state_new() */

/*-----------------------------------------------------------------------*/

/* structure_gl_state_free()
 *
 * Free structure view GL state
 */
  static void
structure_gl_state_free(structure_gl_state_t *state)
{
  if( !state )
    return;

  opengl_axes_free(state->axes);
  gl_instance_free(state->gl);
  g_free(state);

} /* structure_gl_state_free() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_generate_geometry()
 *
 * Generate cylinder geometry for antenna wire segments
 * Colors based on segment type and current draw mode
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

  /* Check for geometry data */
  if( data.n <= 0 )
  {
    structure_vertex_count = 0;
    structure_view_scale = 1.0f;
    return( 0 );
  }

  /* Calculate vertices needed: each segment becomes a cylinder */
  verts_per_seg = opengl_cylinder_vertex_count(STRUCTURE_CYLINDER_SEGMENTS);
  total_vertices = data.n * verts_per_seg;

  mreq = (size_t)total_vertices * sizeof(lit_color_point_t);
  mem_realloc((void **)&structure_vertices, mreq, __LOCATION__);

  /* Find max current/charge magnitude for color scaling */
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

  /* Generate lit cylinder for each wire segment */
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

} /* opengl_structure_generate_geometry() */

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

} /* opengl_structure_update_buffers() */

/*-----------------------------------------------------------------------*/

/* structure_gl_get_state()
 *
 * Get structure view GL state from widget
 */
  static structure_gl_state_t*
structure_gl_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* structure_gl_get_state() */

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
    pr_err("GL context error in structure view\n");
    return;
  }

  gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
  aspect = (float)alloc.width / (float)alloc.height;

  state = structure_gl_state_new(aspect);
  if( !state )
  {
    pr_err("Failed to create structure view GL state\n");
    return;
  }

  arcball_set_viewport(state->gl->arcball, (float)alloc.height);

  /* Initialize view to match rdpattern projection */
  arcball_set_view(state->gl->arcball,
      (float)rdpattern_proj_params.Wr,
      (float)rdpattern_proj_params.Wi);

  g_object_set_data_full(G_OBJECT(area), "gl_state", state,
    (GDestroyNotify)structure_gl_state_free);

  pr_notice("Structure view OpenGL context initialized (Wr=%.1f, Wi=%.1f)\n",
      rdpattern_proj_params.Wr, rdpattern_proj_params.Wi);

} /* on_realize() */

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

  /* Derive mode from global flags (single point of truth) */
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

  if( vertex_count <= 0 )
  {
    /* No geometry - just clear and draw axes */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    zoom = 1.0f;
    if( structure_zoom )
      zoom = (float)gtk_spin_button_get_value(structure_zoom);

    arcball_set_zoom_factor(state->gl->arcball, 4.0f, zoom);
    opengl_axes_set_scale(state->axes, 1.5f);
    arcball_get_mvp(state->gl->arcball, mvp, zoom);
    opengl_axes_render(state->axes, mvp);

    return( TRUE );
  }

  opengl_structure_update_buffers(state);

  if( state->vertex_count == 0 )
    return( FALSE );

  /* Get zoom value from main window's structure zoom spinbutton */
  zoom = 1.0f;
  if( structure_zoom )
    zoom = (float)gtk_spin_button_get_value(structure_zoom) / 100.0f;

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

  /* Draw triangles instead of lines */
  glDrawArrays(GL_TRIANGLES, 0, state->vertex_count);

  glBindVertexArray(0);

  opengl_axes_render(state->axes, mvp);

  glUseProgram(0);

  return( TRUE );

} /* on_render() */

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

} /* on_unrealize() */

/*-----------------------------------------------------------------------*/

/* on_button_press()
 *
 * Mouse button press handler
 */
  static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  structure_gl_state_t *state;

  state = structure_gl_get_state(widget);
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
  structure_gl_state_t *state;

  state = structure_gl_get_state(widget);
  if( !state || !state->gl )
    return( FALSE );

  arcball_end_drag(state->gl->arcball);
  return( TRUE );
}

/* on_motion()
 *
 * Mouse motion handler
 */
  static gboolean
on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
  structure_gl_state_t *state;

  state = structure_gl_get_state(widget);
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
 * Mouse scroll handler for zoom - updates main window's structure_zoom
 */
  static gboolean
on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
  double zoom;

  if( !structure_zoom )
    return( FALSE );

  zoom = gtk_spin_button_get_value(structure_zoom);
  if( event->direction == GDK_SCROLL_UP )
    zoom *= 1.1;
  else if( event->direction == GDK_SCROLL_DOWN )
    zoom /= 1.1;
  else
    return( FALSE );

  gtk_spin_button_set_value(structure_zoom, zoom);
  return( FALSE );
}

/* on_resize()
 *
 * Window resize handler
 */
  static void
on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
  structure_gl_state_t *state;
  float aspect;

  state = structure_gl_get_state(widget);
  if( !state || !state->gl )
    return;

  aspect = (float)allocation->width / (float)allocation->height;
  arcball_set_aspect(state->gl->arcball, aspect);
  arcball_set_viewport(state->gl->arcball, (float)allocation->height);

} /* on_resize() */

/*-----------------------------------------------------------------------*/

/* set_view_preset()
 *
 * Set view to preset angles
 */
  static void
set_view_preset(float wr, float wi)
{
  structure_gl_state_t *state;

  state = structure_gl_get_state(structure_gl_area);
  if( !state || !state->gl )
    return;

  arcball_set_view(state->gl->arcball, wr, wi);
  xnec2_widget_queue_draw(structure_gl_area);

} /* set_view_preset() */

/*-----------------------------------------------------------------------*/

/* on_view_x_activate()
 *
 * View X axis button handler
 */
  static void
on_view_x_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  set_view_preset(0.0f, 0.0f);
}

/* on_view_y_activate()
 *
 * View Y axis button handler
 */
  static void
on_view_y_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  set_view_preset(90.0f, 0.0f);
}

/* on_view_z_activate()
 *
 * View Z axis button handler
 */
  static void
on_view_z_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  set_view_preset(0.0f, 90.0f);
}

/* on_view_default_activate()
 *
 * Default view button handler
 */
  static void
on_view_default_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  set_view_preset(45.0f, 45.0f);
}

/*-----------------------------------------------------------------------*/

/* on_window_destroy()
 *
 * Window destroy signal handler
 */
  static void
on_window_destroy(GObject *object, gpointer user_data)
{
  opengl_structure_view_destroyed_impl();
}

/*-----------------------------------------------------------------------*/

/* on_delete_event()
 *
 * Window delete event handler
 */
  static gboolean
on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  opengl_structure_view_destroyed_impl();
  return( FALSE );
}

/*-----------------------------------------------------------------------*/

/* opengl_structure_view_create_impl()
 *
 * Creates the OpenGL structure view window (internal implementation)
 */
  static GtkWidget*
opengl_structure_view_create_impl(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *menu_item;
  GtkWidget *menu;
  GtkWidget *gl_area;

  if( structure_gl_window != NULL )
  {
    gtk_window_present(GTK_WINDOW(structure_gl_window));
    return( structure_gl_window );
  }

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "OpenGL Structure View");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* Menu bar */
  menubar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  /* View menu */
  menu_item = gtk_menu_item_new_with_label("View");
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

  menu_item = gtk_menu_item_new_with_label("X Axis");
  g_signal_connect(menu_item, "activate", G_CALLBACK(on_view_x_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Y Axis");
  g_signal_connect(menu_item, "activate", G_CALLBACK(on_view_y_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Z Axis");
  g_signal_connect(menu_item, "activate", G_CALLBACK(on_view_z_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Default");
  g_signal_connect(menu_item, "activate", G_CALLBACK(on_view_default_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

  /* GL area */
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

  gtk_box_pack_start(GTK_BOX(vbox), gl_area, TRUE, TRUE, 0);

  g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
  g_signal_connect(window, "delete-event", G_CALLBACK(on_delete_event), NULL);

  structure_gl_window = window;
  structure_gl_area = gl_area;

  gtk_widget_show_all(window);

  return( window );

} /* opengl_structure_view_create_impl() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_view_destroyed_impl()
 *
 * Cleanup when window is closed (internal implementation)
 */
  static void
opengl_structure_view_destroyed_impl(void)
{
  structure_gl_window = NULL;
  structure_gl_area = NULL;

  free_ptr((void **)&structure_vertices);
  structure_vertex_count = 0;

} /* opengl_structure_view_destroyed_impl() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_view_queue_redraw()
 *
 * Request redraw of structure view if it exists
 */
  static void
opengl_structure_view_queue_redraw_impl(void)
{
  if( structure_gl_area != NULL )
  {
    /* Force geometry regeneration */
    structure_geometry_generation++;
    xnec2_widget_queue_draw(structure_gl_area);
  }

} /* opengl_structure_view_queue_redraw_impl() */

#endif /* HAVE_OPENGL */

/*-----------------------------------------------------------------------*/

/* opengl_structure_view_queue_redraw()
 *
 * Public API - always available, no-op when OpenGL not compiled in
 */
  void
opengl_structure_view_queue_redraw(void)
{
#ifdef HAVE_OPENGL
  opengl_structure_view_queue_redraw_impl();
#endif

} /* opengl_structure_view_queue_redraw() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_view_create()
 *
 * Public API - always available, returns NULL when OpenGL not compiled in
 */
  GtkWidget*
opengl_structure_view_create(void)
{
#ifdef HAVE_OPENGL
  return( opengl_structure_view_create_impl() );
#else
  return( NULL );
#endif

} /* opengl_structure_view_create() */

/*-----------------------------------------------------------------------*/

/* opengl_structure_view_destroyed()
 *
 * Public API - always available, no-op when OpenGL not compiled in
 */
  void
opengl_structure_view_destroyed(void)
{
#ifdef HAVE_OPENGL
  opengl_structure_view_destroyed_impl();
#endif

} /* opengl_structure_view_destroyed() */
