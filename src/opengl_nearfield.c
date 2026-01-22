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

#include "opengl_nearfield.h"
#include "shared.h"
#include "draw.h"
#include "draw_radiation.h"
#include "opengl_axes.h"

#ifdef HAVE_OPENGL

/* Line buffer for near field vectors */
static color_point_t *nearfield_lines = NULL;
static int nearfield_line_count = 0;

/* Poynting vector buffers */
static double *pov_x = NULL;
static double *pov_y = NULL;
static double *pov_z = NULL;
static double *pov_r = NULL;

/* Zoom spin button for near field window */
static GtkSpinButton *nearfield_zoom = NULL;


/*-----------------------------------------------------------------------*/

/* nearfield_gl_state_new()
 *
 * Allocate and initialize near field GL state
 */
  static nearfield_gl_state_t*
nearfield_gl_state_new(float aspect)
{
  nearfield_gl_state_t *state;

  state = g_new0(nearfield_gl_state_t, 1);

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
    pr_err("Failed to create axes renderer for near field\n");
    gl_instance_free(state->gl);
    g_free(state);
    return( NULL );
  }

  state->initialized = TRUE;

  return( state );

} /* nearfield_gl_state_new() */

/*-----------------------------------------------------------------------*/

/* nearfield_gl_state_free()
 *
 * Free near field GL state
 */
  static void
nearfield_gl_state_free(nearfield_gl_state_t *state)
{
  if( !state )
    return;

  opengl_axes_free(state->axes);
  gl_instance_free(state->gl);
  g_free(state);

} /* nearfield_gl_state_free() */

/*-----------------------------------------------------------------------*/

/* opengl_nearfield_generate_lines()
 *
 * Generate line geometry for near field vectors
 * Matches Cairo implementation: renders all enabled field types simultaneously
 */
  static int
opengl_nearfield_generate_lines(nearfield_gl_state_t *state)
{
  int idx, npts, line_idx;
  int total_lines;
  double fscale;
  double dr;
  double red, grn, blu;
  double pov_max;
  size_t mreq;

  if( !state || isFlagClear(ENABLE_NEAREH) || !near_field.valid )
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

  /* Normalization scale factor matches Cairo implementation
   * (from draw_radiation.c Draw_Near_Field) */
  if( fpat.near )
    dr = (double)fpat.dxnr;
  else
    dr = sqrt(
        (double)fpat.dxnr * (double)fpat.dxnr +
        (double)fpat.dynr * (double)fpat.dynr +
        (double)fpat.dznr * (double)fpat.dznr ) / 1.75;

  /* Allocate line buffer for all enabled fields */
  mreq = (size_t)total_lines * 2 * sizeof(color_point_t);
  mem_realloc((void **)&nearfield_lines, mreq, __LOCATION__);

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
      /* Poynting vector: E × H */
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
    /*** Draw Near E Field ***/
    if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) )
    {
      fscale = dr / near_field.er[idx];

      nearfield_lines[line_idx].point.x = (float)near_field.px[idx];
      nearfield_lines[line_idx].point.y = (float)near_field.py[idx];
      nearfield_lines[line_idx].point.z = (float)near_field.pz[idx];

      nearfield_lines[line_idx + 1].point.x = (float)(near_field.px[idx] + near_field.erx[idx] * fscale);
      nearfield_lines[line_idx + 1].point.y = (float)(near_field.py[idx] + near_field.ery[idx] * fscale);
      nearfield_lines[line_idx + 1].point.z = (float)(near_field.pz[idx] + near_field.erz[idx] * fscale);

      Value_to_Color(&red, &grn, &blu, near_field.er[idx], near_field.max_er);

      nearfield_lines[line_idx].color.r = (float)red;
      nearfield_lines[line_idx].color.g = (float)grn;
      nearfield_lines[line_idx].color.b = (float)blu;
      nearfield_lines[line_idx].color.a = 1.0f;

      nearfield_lines[line_idx + 1].color = nearfield_lines[line_idx].color;

      line_idx += 2;
    }

    /*** Draw Near H Field ***/
    if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    {
      fscale = dr / near_field.hr[idx];

      nearfield_lines[line_idx].point.x = (float)near_field.px[idx];
      nearfield_lines[line_idx].point.y = (float)near_field.py[idx];
      nearfield_lines[line_idx].point.z = (float)near_field.pz[idx];

      nearfield_lines[line_idx + 1].point.x = (float)(near_field.px[idx] + near_field.hrx[idx] * fscale);
      nearfield_lines[line_idx + 1].point.y = (float)(near_field.py[idx] + near_field.hry[idx] * fscale);
      nearfield_lines[line_idx + 1].point.z = (float)(near_field.pz[idx] + near_field.hrz[idx] * fscale);

      Value_to_Color(&red, &grn, &blu, near_field.hr[idx], near_field.max_hr);

      nearfield_lines[line_idx].color.r = (float)red;
      nearfield_lines[line_idx].color.g = (float)grn;
      nearfield_lines[line_idx].color.b = (float)blu;
      nearfield_lines[line_idx].color.a = 1.0f;

      nearfield_lines[line_idx + 1].color = nearfield_lines[line_idx].color;

      line_idx += 2;
    }

    /*** Draw Poynting Vector ***/
    if( isFlagSet(DRAW_POYNTING) && (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    {
      fscale = dr / pov_r[idx];

      nearfield_lines[line_idx].point.x = (float)near_field.px[idx];
      nearfield_lines[line_idx].point.y = (float)near_field.py[idx];
      nearfield_lines[line_idx].point.z = (float)near_field.pz[idx];

      nearfield_lines[line_idx + 1].point.x = (float)(near_field.px[idx] + pov_x[idx] * fscale);
      nearfield_lines[line_idx + 1].point.y = (float)(near_field.py[idx] + pov_y[idx] * fscale);
      nearfield_lines[line_idx + 1].point.z = (float)(near_field.pz[idx] + pov_z[idx] * fscale);

      Value_to_Color(&red, &grn, &blu, pov_r[idx], pov_max);

      nearfield_lines[line_idx].color.r = (float)red;
      nearfield_lines[line_idx].color.g = (float)grn;
      nearfield_lines[line_idx].color.b = (float)blu;
      nearfield_lines[line_idx].color.a = 1.0f;

      nearfield_lines[line_idx + 1].color = nearfield_lines[line_idx].color;

      line_idx += 2;
    }
  }

  nearfield_line_count = total_lines;

  return( nearfield_line_count );

} /* opengl_nearfield_generate_lines() */

/*-----------------------------------------------------------------------*/

/* opengl_nearfield_update_buffers()
 *
 * Upload line data to GPU
 */
  static void
opengl_nearfield_update_buffers(nearfield_gl_state_t *state)
{
  size_t buffer_size;

  if( !state || !state->gl || !state->gl->initialized || !nearfield_lines || nearfield_line_count == 0 )
    return;

  glBindVertexArray(state->gl->vao);
  glBindBuffer(GL_ARRAY_BUFFER, state->gl->vbo);

  buffer_size = (size_t)nearfield_line_count * 2 * sizeof(color_point_t);
  glBufferData(GL_ARRAY_BUFFER, buffer_size, nearfield_lines, GL_DYNAMIC_DRAW);

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

  state->line_count = nearfield_line_count;

} /* opengl_nearfield_update_buffers() */

/*-----------------------------------------------------------------------*/

/* nearfield_gl_get_state()
 *
 * Get near field GL state from widget
 */
  static nearfield_gl_state_t*
nearfield_gl_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* nearfield_gl_get_state() */

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler
 */
  static void
on_realize(GtkGLArea *area)
{
  nearfield_gl_state_t *state;
  GtkAllocation alloc;
  float aspect;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error in near field window\n");
    return;
  }

  gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
  aspect = (float)alloc.width / (float)alloc.height;

  state = nearfield_gl_state_new(aspect);
  if( !state )
  {
    pr_err("Failed to create near field GL state\n");
    return;
  }

  arcball_set_viewport(state->gl->arcball, (float)alloc.height);

  /* Initialize view to match Cairo rdpattern projection */
  arcball_set_view(state->gl->arcball,
      (float)rdpattern_proj_params.Wr,
      (float)rdpattern_proj_params.Wi);

  g_object_set_data_full(G_OBJECT(area), "gl_state", state,
    (GDestroyNotify)nearfield_gl_state_free);

  pr_notice("Near field OpenGL context initialized (Wr=%.1f, Wi=%.1f)\n",
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
  nearfield_gl_state_t *state;
  mat4 mvp;
  int line_count;
  float r_max;
  float zoom;

  state = g_object_get_data(G_OBJECT(area), "gl_state");
  if( !state || !state->gl || !state->gl->initialized )
    return( FALSE );

  if( isFlagClear(ENABLE_NEAREH) || !near_field.valid )
    return( FALSE );

  line_count = opengl_nearfield_generate_lines(state);
  if( line_count <= 0 )
    return( FALSE );

  opengl_nearfield_update_buffers(state);

  if( state->line_count == 0 )
    return( FALSE );

  /* Set distance and axes scale */
  r_max = (float)near_field.r_max;
  arcball_set_distance(state->gl->arcball, r_max * 2.165f);
  opengl_axes_set_scale(state->axes, r_max);

  /* Get zoom value from spinbutton */
  zoom = 1.0f;
  if( nearfield_zoom )
    zoom = (float)gtk_spin_button_get_value(nearfield_zoom);

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
  nearfield_gl_state_t *state;

  state = nearfield_gl_get_state(widget);
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
  nearfield_gl_state_t *state;

  state = nearfield_gl_get_state(widget);
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
  nearfield_gl_state_t *state;

  state = nearfield_gl_get_state(widget);
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

  if( !nearfield_zoom )
    return( FALSE );

  zoom = gtk_spin_button_get_value(nearfield_zoom);
  if( event->direction == GDK_SCROLL_UP )
    zoom *= 1.1;
  else if( event->direction == GDK_SCROLL_DOWN )
    zoom /= 1.1;
  else
    return( FALSE );

  gtk_spin_button_set_value(nearfield_zoom, zoom);
  return( FALSE );
}

/* on_resize()
 *
 * Window resize handler
 */
  static void
on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
  nearfield_gl_state_t *state;
  float aspect;

  state = nearfield_gl_get_state(widget);
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
  nearfield_gl_state_t *state;

  state = nearfield_gl_get_state(nearfield_gl_area);
  if( !state || !state->gl )
    return;

  arcball_set_view(state->gl->arcball, wr, wi);
  xnec2_widget_queue_draw(nearfield_gl_area);

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

/* on_animate_activate()
 *
 * Animation menu handler
 */
  static void
on_animate_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  /* Open existing animation dialog which controls both Cairo and OpenGL */
  if( !Validate_Nearfield_Animation() )
    return;

  if( animate_dialog == NULL )
  {
    animate_dialog = create_animate_dialog(&animate_dialog_builder);
    gtk_widget_show(animate_dialog);
  }
  else
  {
    gtk_window_present(GTK_WINDOW(animate_dialog));
  }
}

/*-----------------------------------------------------------------------*/

/* on_zoom_value_changed()
 *
 * Zoom spin button value changed handler
 */
  static void
on_zoom_value_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
  xnec2_widget_queue_draw(nearfield_gl_area);
}

/*-----------------------------------------------------------------------*/

/* on_window_destroy()
 *
 * Window destroy signal handler
 */
  static void
on_window_destroy(GObject *object, gpointer user_data)
{
  opengl_nearfield_window_killed();
}

/*-----------------------------------------------------------------------*/

/* on_delete_event()
 *
 * Window delete event handler
 */
  static gboolean
on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  opengl_nearfield_window_killed();
  return( FALSE );
}

/*-----------------------------------------------------------------------*/

/* opengl_nearfield_create_window()
 *
 * Creates the OpenGL near field test window
 */
  GtkWidget*
opengl_nearfield_create_window(void)
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

  if( nearfield_gl_window != NULL )
  {
    gtk_window_present(GTK_WINDOW(nearfield_gl_window));
    return( nearfield_gl_window );
  }

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "OpenGL Near Field Test");
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

  /* Animation menu */
  menu_item = gtk_menu_item_new_with_label("Animation");
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);
  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

  menu_item = gtk_menu_item_new_with_label("Start...");
  g_signal_connect(menu_item, "activate", G_CALLBACK(on_animate_activate), NULL);
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

  nearfield_zoom = GTK_SPIN_BUTTON(zoom_spin);

  g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
  g_signal_connect(window, "delete-event", G_CALLBACK(on_delete_event), NULL);

  nearfield_gl_window = window;
  nearfield_gl_area = gl_area;

  gtk_widget_show_all(window);

  return( window );

} /* opengl_nearfield_create_window() */

/*-----------------------------------------------------------------------*/

/* opengl_nearfield_window_killed()
 *
 * Cleanup when window is closed
 */
  void
opengl_nearfield_window_killed(void)
{
  nearfield_gl_window = NULL;
  nearfield_gl_area = NULL;
  nearfield_zoom = NULL;

} /* opengl_nearfield_window_killed() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
