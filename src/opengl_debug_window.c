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

#include "opengl_debug_window.h"
#include "shared.h"
#include "opengl_axes.h"

#ifdef HAVE_OPENGL

/* Line buffer for debug geometry */
static color_point_t *debug_lines = NULL;
static int debug_line_count = 0;
static gboolean debug_geometry_initialized = FALSE;

/* Zoom spin button for debug window */
static GtkSpinButton *debug_zoom = NULL;

/* Cube size for debug rendering */
#define DEBUG_CUBE_SIZE 1.0f

/*-----------------------------------------------------------------------*/

/* debug_gl_state_new()
 *
 * Allocate and initialize debug GL state
 */
  static debug_gl_state_t*
debug_gl_state_new(float aspect)
{
  debug_gl_state_t *state;

  state = g_new0(debug_gl_state_t, 1);

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
  state->line_count = 0;

  state->axes = opengl_axes_new(&state->gl->shader);
  if( !state->axes )
  {
    pr_err("Failed to create axes renderer for debug window\n");
    gl_instance_free(state->gl);
    g_free(state);
    return( NULL );
  }

  state->initialized = TRUE;

  return( state );

} /* debug_gl_state_new() */

/*-----------------------------------------------------------------------*/

/* debug_gl_state_free()
 *
 * Free debug GL state
 */
  static void
debug_gl_state_free(debug_gl_state_t *state)
{
  if( !state )
    return;

  opengl_axes_free(state->axes);
  gl_instance_free(state->gl);
  g_free(state);

} /* debug_gl_state_free() */

/*-----------------------------------------------------------------------*/

/* opengl_debug_add_line()
 *
 * Helper to add a colored line to the buffer
 */
  static void
opengl_debug_add_line(
    int *idx,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float r, float g, float b)
{
  int i = *idx;

  debug_lines[i].point.x = x1;
  debug_lines[i].point.y = y1;
  debug_lines[i].point.z = z1;
  debug_lines[i].color.r = r;
  debug_lines[i].color.g = g;
  debug_lines[i].color.b = b;
  debug_lines[i].color.a = 1.0f;

  debug_lines[i + 1].point.x = x2;
  debug_lines[i + 1].point.y = y2;
  debug_lines[i + 1].point.z = z2;
  debug_lines[i + 1].color.r = r;
  debug_lines[i + 1].color.g = g;
  debug_lines[i + 1].color.b = b;
  debug_lines[i + 1].color.a = 1.0f;

  *idx = i + 2;

} /* opengl_debug_add_line() */

/*-----------------------------------------------------------------------*/

/* opengl_debug_generate_cube()
 *
 * Generate wireframe cube for debug visualization
 * 12 edges, color-coded by axis: X=red, Y=green, Z=blue
 */
  static int
opengl_debug_generate_cube(void)
{
  float s = DEBUG_CUBE_SIZE;
  int idx = 0;
  size_t mreq;

  if( debug_geometry_initialized )
    return( debug_line_count );

  /* 12 edges = 24 vertices */
  mreq = 24 * sizeof(color_point_t);
  mem_realloc((void **)&debug_lines, mreq, __LOCATION__);

  /* Bottom face edges (z = -s) */
  opengl_debug_add_line(&idx, -s, -s, -s,  s, -s, -s, 1.0f, 0.0f, 0.0f);  /* X red */
  opengl_debug_add_line(&idx,  s, -s, -s,  s,  s, -s, 0.0f, 1.0f, 0.0f);  /* Y green */
  opengl_debug_add_line(&idx,  s,  s, -s, -s,  s, -s, 1.0f, 0.0f, 0.0f);  /* X red */
  opengl_debug_add_line(&idx, -s,  s, -s, -s, -s, -s, 0.0f, 1.0f, 0.0f);  /* Y green */

  /* Top face edges (z = +s) */
  opengl_debug_add_line(&idx, -s, -s,  s,  s, -s,  s, 1.0f, 0.0f, 0.0f);  /* X red */
  opengl_debug_add_line(&idx,  s, -s,  s,  s,  s,  s, 0.0f, 1.0f, 0.0f);  /* Y green */
  opengl_debug_add_line(&idx,  s,  s,  s, -s,  s,  s, 1.0f, 0.0f, 0.0f);  /* X red */
  opengl_debug_add_line(&idx, -s,  s,  s, -s, -s,  s, 0.0f, 1.0f, 0.0f);  /* Y green */

  /* Vertical edges (Z blue) */
  opengl_debug_add_line(&idx, -s, -s, -s, -s, -s,  s, 0.0f, 0.0f, 1.0f);
  opengl_debug_add_line(&idx,  s, -s, -s,  s, -s,  s, 0.0f, 0.0f, 1.0f);
  opengl_debug_add_line(&idx,  s,  s, -s,  s,  s,  s, 0.0f, 0.0f, 1.0f);
  opengl_debug_add_line(&idx, -s,  s, -s, -s,  s,  s, 0.0f, 0.0f, 1.0f);

  debug_line_count = 12;
  debug_geometry_initialized = TRUE;

  return( debug_line_count );

} /* opengl_debug_generate_cube() */

/*-----------------------------------------------------------------------*/

/* opengl_debug_update_buffers()
 *
 * Upload line data to GPU
 */
  static void
opengl_debug_update_buffers(debug_gl_state_t *state)
{
  size_t buffer_size;

  if( !state || !state->gl || !state->gl->initialized || !debug_lines || debug_line_count == 0 )
    return;

  glBindVertexArray(state->gl->vao);
  glBindBuffer(GL_ARRAY_BUFFER, state->gl->vbo);

  buffer_size = (size_t)debug_line_count * 2 * sizeof(color_point_t);
  glBufferData(GL_ARRAY_BUFFER, buffer_size, debug_lines, GL_DYNAMIC_DRAW);

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

  state->line_count = debug_line_count;

} /* opengl_debug_update_buffers() */

/*-----------------------------------------------------------------------*/

/* debug_gl_get_state()
 *
 * Get debug GL state from widget
 */
  static debug_gl_state_t*
debug_gl_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* debug_gl_get_state() */

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler
 */
  static void
on_realize(GtkGLArea *area)
{
  debug_gl_state_t *state;
  GtkAllocation alloc;
  float aspect;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error in debug window\n");
    return;
  }

  gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
  aspect = (float)alloc.width / (float)alloc.height;

  state = debug_gl_state_new(aspect);
  if( !state )
  {
    pr_err("Failed to create debug GL state\n");
    return;
  }

  arcball_set_viewport(state->gl->arcball, (float)alloc.height);

  /* Initialize view to match rdpattern projection */
  arcball_set_view(state->gl->arcball,
      (float)rdpattern_proj_params.Wr,
      (float)rdpattern_proj_params.Wi);

  g_object_set_data_full(G_OBJECT(area), "gl_state", state,
    (GDestroyNotify)debug_gl_state_free);

  pr_notice("Debug OpenGL context initialized (Wr=%.1f, Wi=%.1f)\n",
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
  debug_gl_state_t *state;
  mat4 mvp;
  int line_count;
  float zoom;

  state = g_object_get_data(G_OBJECT(area), "gl_state");
  if( !state || !state->gl || !state->gl->initialized )
    return( FALSE );

  line_count = opengl_debug_generate_cube();
  if( line_count <= 0 )
    return( FALSE );

  opengl_debug_update_buffers(state);

  if( state->line_count == 0 )
    return( FALSE );

  /* Get zoom value from spinbutton */
  zoom = 1.0f;
  if( debug_zoom )
    zoom = (float)gtk_spin_button_get_value(debug_zoom);

  /* Set distance and axes scale for cube */
  arcball_set_zoom_factor(state->gl->arcball, DEBUG_CUBE_SIZE * 4.0f, zoom);
  opengl_axes_set_scale(state->axes, DEBUG_CUBE_SIZE * 1.5f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  arcball_get_mvp(state->gl->arcball, mvp, zoom);

  glUseProgram(state->gl->shader.program);
  glUniformMatrix4fv(state->gl->mvp_location, 1, GL_FALSE, (float*)mvp);

  glBindVertexArray(state->gl->vao);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glDrawArrays(GL_LINES, 0, state->line_count * 2);

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
  gtk_gl_area_make_current(area);

} /* on_unrealize() */

/*-----------------------------------------------------------------------*/

/* on_button_press()
 *
 * Mouse button press handler
 */
  static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  debug_gl_state_t *state;

  state = debug_gl_get_state(widget);
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
  debug_gl_state_t *state;

  state = debug_gl_get_state(widget);
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
  debug_gl_state_t *state;

  state = debug_gl_get_state(widget);
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
  double zoom;

  if( !debug_zoom )
    return( FALSE );

  zoom = gtk_spin_button_get_value(debug_zoom);
  if( event->direction == GDK_SCROLL_UP )
    zoom *= 1.1;
  else if( event->direction == GDK_SCROLL_DOWN )
    zoom /= 1.1;
  else
    return( FALSE );

  gtk_spin_button_set_value(debug_zoom, zoom);
  return( FALSE );
}

/* on_resize()
 *
 * Window resize handler
 */
  static void
on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
  debug_gl_state_t *state;
  float aspect;

  state = debug_gl_get_state(widget);
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
  debug_gl_state_t *state;

  state = debug_gl_get_state(debug_gl_area);
  if( !state || !state->gl )
    return;

  arcball_set_view(state->gl->arcball, wr, wi);
  xnec2_widget_queue_draw(debug_gl_area);

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

/* on_zoom_value_changed()
 *
 * Zoom spin button value changed handler
 */
  static void
on_zoom_value_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
  xnec2_widget_queue_draw(debug_gl_area);
}

/*-----------------------------------------------------------------------*/

/* on_window_destroy()
 *
 * Window destroy signal handler
 */
  static void
on_window_destroy(GObject *object, gpointer user_data)
{
  opengl_debug_window_killed();
}

/*-----------------------------------------------------------------------*/

/* on_delete_event()
 *
 * Window delete event handler
 */
  static gboolean
on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  opengl_debug_window_killed();
  return( FALSE );
}

/*-----------------------------------------------------------------------*/

/* opengl_debug_create_window()
 *
 * Creates the OpenGL debug window
 */
  GtkWidget*
opengl_debug_create_window(void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *menu_item;
  GtkWidget *menu;
  GtkWidget *gl_area;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *zoom_spin;

  if( debug_gl_window != NULL )
  {
    gtk_window_present(GTK_WINDOW(debug_gl_window));
    return( debug_gl_window );
  }

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "OpenGL Debug Window");
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

  /* Bottom bar with zoom control */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

  label = gtk_label_new("Zoom:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  zoom_spin = gtk_spin_button_new_with_range(0.1, 10.0, 0.1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_spin), 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(zoom_spin), 1);
  g_signal_connect(zoom_spin, "value-changed", G_CALLBACK(on_zoom_value_changed), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), zoom_spin, FALSE, FALSE, 0);

  debug_zoom = GTK_SPIN_BUTTON(zoom_spin);

  g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
  g_signal_connect(window, "delete-event", G_CALLBACK(on_delete_event), NULL);

  debug_gl_window = window;
  debug_gl_area = gl_area;

  gtk_widget_show_all(window);

  return( window );

} /* opengl_debug_create_window() */

/*-----------------------------------------------------------------------*/

/* opengl_debug_window_killed()
 *
 * Cleanup when window is closed
 */
  void
opengl_debug_window_killed(void)
{
  debug_gl_window = NULL;
  debug_gl_area = NULL;
  debug_zoom = NULL;

} /* opengl_debug_window_killed() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
