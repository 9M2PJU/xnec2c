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

#include "opengl_view.h"
#include "shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/* gl_view_state_free()
 *
 * Free view state resources
 */
  static void
gl_view_state_free(gl_view_state_t *state)
{
  if( !state )
    return;

  if( state->scene && state->scene->cleanup )
    state->scene->cleanup();

  if( state->overlay )
    gradient_overlay_free(state->overlay);

  if( state->axes )
    opengl_axes_free(state->axes);

  if( state->vbo )
    glDeleteBuffers(1, &state->vbo);

  if( state->vao )
    glDeleteVertexArrays(1, &state->vao);

  gl_shader_destroy(&state->shader);

  if( state->attrib_locations )
    g_free(state->attrib_locations);

  g_free(state);

} /* gl_view_state_free() */

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler
 */
  static void
on_realize(GtkGLArea *area, gpointer user_data)
{
  gl_view_state_t *state;
  GtkAllocation alloc;
  gboolean ok;
  int i;

  state = (gl_view_state_t *)user_data;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error\n");
    return;
  }

  ok = gl_shader_load(&state->shader,
    state->config->vertex_shader_path,
    state->config->fragment_shader_path);

  if( !ok )
  {
    pr_err("Failed to load shaders\n");
    return;
  }

  state->mvp_location = glGetUniformLocation(state->shader.program, "mvp");

  glGenVertexArrays(1, &state->vao);
  glGenBuffers(1, &state->vbo);

  state->attrib_locations = g_new(GLint, state->config->attrib_count);

  for( i = 0; i < state->config->attrib_count; i++ )
  {
    state->attrib_locations[i] = glGetAttribLocation(
      state->shader.program,
      state->config->attribs[i].name);
  }

  state->axes = opengl_axes_new();
  if( !state->axes )
  {
    pr_err("Failed to create axes renderer\n");
    return;
  }

  if( state->config->has_gradient )
  {
    state->overlay = gradient_overlay_new(state->config->gradient_draw);
    if( !state->overlay )
    {
      pr_err("Failed to create gradient overlay\n");
      return;
    }

    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    gradient_overlay_set_viewport(state->overlay, alloc.width, alloc.height);
  }

  state->initialized = TRUE;

} /* on_realize() */

/*-----------------------------------------------------------------------*/

/* on_unrealize()
 *
 * GtkGLArea unrealize signal handler
 */
  static void
on_unrealize(GtkGLArea *area, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  gtk_gl_area_make_current(area);

  gl_view_state_free(state);

} /* on_unrealize() */

/*-----------------------------------------------------------------------*/

/* on_render()
 *
 * GtkGLArea render signal handler
 */
  static gboolean
on_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
  gl_view_state_t *state;
  gl_view_content_t content;
  mat4 mvp;
  float camera_distance;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->initialized )
    return( FALSE );

  if( !state->scene->generate(&content) )
    return( FALSE );

  if( content.generation != state->last_generation )
  {
    if( content.vertex_count > 0 )
    {
      glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
      glBufferData(GL_ARRAY_BUFFER,
        content.vertex_count * content.vertex_stride,
        content.vertices,
        GL_STATIC_DRAW);
    }

    if( state->overlay )
      gradient_overlay_mark_dirty(state->overlay);

    state->last_generation = content.generation;
  }

  state->content = content;

  state->current_zoom = (state->zoom_spinbutton && *state->zoom_spinbutton) ?
    (float)gtk_spin_button_get_value(*state->zoom_spinbutton) : content.zoom;

  camera_distance = content.r_max * ARCBALL_BASE_DISTANCE_FACTOR / state->current_zoom;

  arcball_get_mvp(state->arcball, mvp, camera_distance, content.model_scale);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);

  if( content.vertex_count > 0 )
  {
    int i;

    glUseProgram(state->shader.program);
    glUniformMatrix4fv(state->mvp_location, 1, GL_FALSE, (float *)mvp);

    glBindVertexArray(state->vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);

    for( i = 0; i < state->config->attrib_count; i++ )
    {
      const gl_vertex_attrib_t *attr = &state->config->attribs[i];

      glEnableVertexAttribArray(state->attrib_locations[i]);
      glVertexAttribPointer(
        state->attrib_locations[i],
        attr->components,
        GL_FLOAT,
        GL_FALSE,
        content.vertex_stride,
        (void *)(long)attr->offset);
    }

    glDrawArrays(content.draw_mode, 0, content.vertex_count);

    for( i = 0; i < state->config->attrib_count; i++ )
      glDisableVertexAttribArray(state->attrib_locations[i]);

    glBindVertexArray(0);
  }

  opengl_axes_set_scale(state->axes, content.r_max * 1.1f);
  opengl_axes_render(state->axes, mvp);

  if( state->overlay && content.show_gradient )
    gradient_overlay_render(state->overlay);

  if( state->scene->post_render )
    state->scene->post_render();

  return( TRUE );

} /* on_render() */

/*-----------------------------------------------------------------------*/

/* on_resize()
 *
 * GtkGLArea resize signal handler
 */
  static void
on_resize(GtkGLArea *area, int width, int height, gpointer user_data)
{
  gl_view_state_t *state;
  float aspect;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return;

  aspect = (float)width / (float)height;

  arcball_set_aspect(state->arcball, aspect);
  arcball_set_viewport(state->arcball, (float)height);

  if( state->overlay )
    gradient_overlay_set_viewport(state->overlay, width, height);

  glViewport(0, 0, width, height);

} /* on_resize() */

/*-----------------------------------------------------------------------*/

/* on_button_press()
 *
 * Mouse button press handler
 */
  static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  if( event->button == 1 || event->button == 2 )
  {
    arcball_begin_drag(state->arcball, event->button, event->x, event->y);
    return( TRUE );
  }

  return( FALSE );

} /* on_button_press() */

/*-----------------------------------------------------------------------*/

/* on_button_release()
 *
 * Mouse button release handler
 */
  static gboolean
on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  arcball_end_drag(state->arcball);

  return( TRUE );

} /* on_button_release() */

/*-----------------------------------------------------------------------*/

/* on_motion()
 *
 * Mouse motion handler
 */
  static gboolean
on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  gl_view_state_t *state;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  arcball_drag(state->arcball, event->x, event->y);
  gtk_widget_queue_draw(widget);

  return( TRUE );

} /* on_motion() */

/*-----------------------------------------------------------------------*/

/* on_scroll()
 *
 * Mouse scroll handler
 */
  static gboolean
on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
  gl_view_state_t *state;
  GtkSpinButton *spinbutton;
  double value, step;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->zoom_spinbutton || !*state->zoom_spinbutton )
    return( FALSE );

  spinbutton = *state->zoom_spinbutton;
  value = gtk_spin_button_get_value(spinbutton);
  gtk_spin_button_get_increments(spinbutton, &step, NULL);

  if( event->direction == GDK_SCROLL_UP )
    value += step;
  else if( event->direction == GDK_SCROLL_DOWN )
    value -= step;
  else
    return( FALSE );

  gtk_spin_button_set_value(spinbutton, value);

  return( TRUE );

} /* on_scroll() */

/*-----------------------------------------------------------------------*/

/* gl_view_create_widget()
 *
 * Create GL area widget with engine wired
 */
  GtkWidget*
gl_view_create_widget(
    gl_view_config_t *config,
    gl_scene_provider_t *scene,
    arcball_state_t *arcball,
    GtkSpinButton **zoom_spinbutton)
{
  GtkWidget *gl_area;
  gl_view_state_t *state;

  if( !config || !scene || !arcball )
    return( NULL );

  state = g_new0(gl_view_state_t, 1);

  state->config = config;
  state->scene = scene;
  state->arcball = arcball;
  state->zoom_spinbutton = zoom_spinbutton;
  state->current_zoom = 1.0f;
  state->last_generation = (unsigned int)-1;

  gl_area = gtk_gl_area_new();

  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), TRUE);
  gtk_gl_area_set_auto_render(GTK_GL_AREA(gl_area), FALSE);

  gtk_widget_set_size_request(gl_area, 400, 400);
  gtk_widget_set_hexpand(gl_area, TRUE);
  gtk_widget_set_vexpand(gl_area, TRUE);

  g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize), state);
  g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_unrealize), state);
  g_signal_connect(gl_area, "render", G_CALLBACK(on_render), state);
  g_signal_connect(gl_area, "resize", G_CALLBACK(on_resize), state);

  gtk_widget_add_events(gl_area,
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
    GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);

  g_signal_connect(gl_area, "button-press-event",
    G_CALLBACK(on_button_press), state);
  g_signal_connect(gl_area, "button-release-event",
    G_CALLBACK(on_button_release), state);
  g_signal_connect(gl_area, "motion-notify-event",
    G_CALLBACK(on_motion), state);
  g_signal_connect(gl_area, "scroll-event",
    G_CALLBACK(on_scroll), state);

  g_object_set_data(G_OBJECT(gl_area), "gl_state", state);

  return( gl_area );

} /* gl_view_create_widget() */

/*-----------------------------------------------------------------------*/

/* gl_view_get_state()
 *
 * Get view state from widget
 */
  gl_view_state_t*
gl_view_get_state(GtkWidget *widget)
{
  if( !widget )
    return( NULL );

  return( g_object_get_data(G_OBJECT(widget), "gl_state") );

} /* gl_view_get_state() */

/*-----------------------------------------------------------------------*/

/* gl_view_invalidate()
 *
 * Mark scene dirty and queue redraw
 */
  void
gl_view_invalidate(GtkWidget *widget)
{
  if( !widget )
    return;

  gtk_gl_area_queue_render(GTK_GL_AREA(widget));

} /* gl_view_invalidate() */

/*-----------------------------------------------------------------------*/

/* gl_view_sync_arcball()
 *
 * Sync arcball rotation from angles
 */
  void
gl_view_sync_arcball(GtkWidget *widget, double wr, double wi)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state || !state->arcball )
    return;

  arcball_set_view(state->arcball, (float)wr, (float)wi);
  arcball_notify_changed(state->arcball);

} /* gl_view_sync_arcball() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
