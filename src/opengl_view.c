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

/* gl_view_recreate_msaa()
 *
 * Recreate MSAA resources without destroying GL widget
 */
  static void
gl_view_recreate_msaa(gl_view_state_t *state, int requested_samples)
{
  GLint max_samples;
  int width, height;

  if( !state || !state->initialized )
    return;

  /* Delete existing MSAA resources */
  if( state->msaa_fbo )
  {
    glDeleteFramebuffers(1, &state->msaa_fbo);
    state->msaa_fbo = 0;
  }

  if( state->msaa_color_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    state->msaa_color_rbo = 0;
  }

  if( state->msaa_depth_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_depth_rbo = 0;
  }

  state->msaa_samples = 0;

  if( requested_samples == 0 )
    return;

  /* Query hardware limit and clamp */
  glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
  state->msaa_samples = (requested_samples > max_samples) ? max_samples : requested_samples;

  if( state->msaa_samples < 2 )
  {
    state->msaa_samples = 0;
    return;
  }

  /* Get current viewport dimensions */
  width = state->msaa_width;
  height = state->msaa_height;

  if( width == 0 || height == 0 )
    return;

  /* Create multisampled color renderbuffer */
  glGenRenderbuffers(1, &state->msaa_color_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, state->msaa_color_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, state->msaa_samples,
      GL_RGBA8, width, height);

  /* Create multisampled depth renderbuffer */
  glGenRenderbuffers(1, &state->msaa_depth_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, state->msaa_depth_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, state->msaa_samples,
      GL_DEPTH_COMPONENT24, width, height);

  /* Create FBO and attach renderbuffers */
  glGenFramebuffers(1, &state->msaa_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, state->msaa_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER, state->msaa_color_rbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
      GL_RENDERBUFFER, state->msaa_depth_rbo);

  if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
  {
    pr_err("MSAA framebuffer incomplete\n");
    glDeleteFramebuffers(1, &state->msaa_fbo);
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_fbo = 0;
    state->msaa_color_rbo = 0;
    state->msaa_depth_rbo = 0;
    state->msaa_samples = 0;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

} /* gl_view_recreate_msaa() */

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

  if( state->scene && state->scene->overlay_cleanup )
    state->scene->overlay_cleanup();

  /* Free overlay rendering resources */
  if( state->ovl_initialized )
  {
    if( state->ovl_vbo )
      glDeleteBuffers(1, &state->ovl_vbo);

    if( state->ovl_vao )
      glDeleteVertexArrays(1, &state->ovl_vao);

    gl_shader_destroy(&state->ovl_shader);

    if( state->ovl_attrib_locations )
      g_free(state->ovl_attrib_locations);

    state->ovl_initialized = FALSE;
  }

  if( state->overlay )
    gradient_overlay_free(state->overlay);

  if( state->axes )
    opengl_axes_free(state->axes);

  /* Free MSAA resources */
  if( state->msaa_fbo )
  {
    glDeleteFramebuffers(1, &state->msaa_fbo);
    state->msaa_fbo = 0;
  }

  if( state->msaa_color_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    state->msaa_color_rbo = 0;
  }

  if( state->msaa_depth_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_depth_rbo = 0;
  }

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

  /* Initialize overlay rendering resources if configured */
  if( state->scene->overlay_config )
  {
    const gl_overlay_config_t *ocfg = state->scene->overlay_config;

    ok = gl_shader_load(&state->ovl_shader,
        ocfg->vertex_shader_path,
        ocfg->fragment_shader_path);

    if( ok )
    {
      state->ovl_mvp_location =
        glGetUniformLocation(state->ovl_shader.program, "mvp");

      glGenVertexArrays(1, &state->ovl_vao);
      glGenBuffers(1, &state->ovl_vbo);

      state->ovl_attrib_locations = g_new(GLint, ocfg->attrib_count);
      for( i = 0; i < ocfg->attrib_count; i++ )
      {
        state->ovl_attrib_locations[i] = glGetAttribLocation(
            state->ovl_shader.program,
            ocfg->attribs[i].name);
      }

      state->ovl_last_generation = (unsigned int)-1;
      state->ovl_initialized = TRUE;
    }
    else
    {
      pr_err("Failed to load overlay shaders\n");
    }
  }

  state->initialized = TRUE;

  /* Initialize MSAA resources after GL context is ready */
  gl_view_recreate_msaa(state, rc_config.opengl_msaa_samples);

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

/* gl_view_draw_pass()
 *
 * Execute a rendering pass with given shader, VAO, VBO, and vertex attributes
 */
  static void
gl_view_draw_pass(
    GLuint shader_program,
    GLint mvp_location,
    mat4 mvp,
    GLuint vao,
    GLuint vbo,
    const gl_vertex_attrib_t *attribs,
    const GLint *attrib_locations,
    int attrib_count,
    const gl_view_content_t *content)
{
  int i;

  glUseProgram(shader_program);
  glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (float *)mvp);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  for( i = 0; i < attrib_count; i++ )
  {
    glEnableVertexAttribArray(attrib_locations[i]);
    glVertexAttribPointer(
        attrib_locations[i],
        attribs[i].components,
        GL_FLOAT,
        GL_FALSE,
        content->vertex_stride,
        (void *)(long)attribs[i].offset);
  }

  glDrawArrays(content->draw_mode, 0, content->vertex_count);

  for( i = 0; i < attrib_count; i++ )
    glDisableVertexAttribArray(attrib_locations[i]);

  glBindVertexArray(0);

} /* gl_view_draw_pass() */

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
  GLint default_fbo = 0;

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

  /* Scene provides normalized zoom via content.zoom */
  camera_distance = content.r_max * ARCBALL_BASE_DISTANCE_FACTOR / content.zoom;

  /* Cache distance for pan calculations during drag */
  state->cached_camera_distance = camera_distance;

  /* Compute clip planes dynamically based on scene geometry and camera position.
   * This prevents Z-depth clipping when zooming out while maintaining depth
   * precision by keeping near plane as far from camera as geometry allows. */
  {
    float nearest_point, farthest_point, near_plane, far_plane;

    nearest_point = camera_distance - content.r_max;
    farthest_point = camera_distance + content.r_max;

    far_plane = farthest_point * 1.2f;

    if( nearest_point > 0.0f )
    {
      near_plane = nearest_point * 0.8f;
    }
    else
    {
      near_plane = 0.001f;
    }

    arcball_get_mvp(state->arcball, mvp, state->pan_offset,
        camera_distance, content.model_scale, state->aspect, state->fov_rad,
        near_plane, far_plane);
  }

  /* Save default FBO and bind MSAA FBO if active */
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &default_fbo);

  if( state->msaa_fbo )
    glBindFramebuffer(GL_FRAMEBUFFER, state->msaa_fbo);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

  if( content.vertex_count > 0 )
  {
    gl_view_draw_pass(
        state->shader.program,
        state->mvp_location,
        mvp,
        state->vao,
        state->vbo,
        state->config->attribs,
        state->attrib_locations,
        state->config->attrib_count,
        &content);
  }

  /* Render structure overlay if provider is active */
  if( state->ovl_initialized && state->scene->overlay_generate )
  {
    gl_view_content_t ovl_content;

    if( state->scene->overlay_generate(&ovl_content) &&
        ovl_content.vertex_count > 0 )
    {
      const gl_overlay_config_t *ocfg = state->scene->overlay_config;

      /* Upload overlay vertices on generation change */
      if( ovl_content.generation != state->ovl_last_generation )
      {
        glBindBuffer(GL_ARRAY_BUFFER, state->ovl_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            ovl_content.vertex_count * ovl_content.vertex_stride,
            ovl_content.vertices,
            GL_STATIC_DRAW);
        state->ovl_last_generation = ovl_content.generation;
      }

      /* Draw overlay with its own shader, same MVP */
      gl_view_draw_pass(
          state->ovl_shader.program,
          state->ovl_mvp_location,
          mvp,
          state->ovl_vao,
          state->ovl_vbo,
          ocfg->attribs,
          state->ovl_attrib_locations,
          ocfg->attrib_count,
          &ovl_content);
    }
  }

  opengl_axes_set_scale(state->axes, content.r_max * 1.1f);
  opengl_axes_render(state->axes, mvp);

  if( state->overlay && content.show_gradient )
    gradient_overlay_render(state->overlay);

  if( state->scene->post_render )
    state->scene->post_render();

  /* Blit MSAA to default FBO */
  if( state->msaa_fbo )
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, state->msaa_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, default_fbo);
    glBlitFramebuffer(0, 0, state->msaa_width, state->msaa_height,
                      0, 0, state->msaa_width, state->msaa_height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, default_fbo);
  }

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

  state->aspect = aspect;
  state->viewport_height = (float)height;

  /* Store dimensions for MSAA recreation */
  state->msaa_width = width;
  state->msaa_height = height;

  /* Recreate MSAA FBO at new dimensions */
  if( state->msaa_samples > 0 )
    gl_view_recreate_msaa(state, state->msaa_samples);

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
 * Mouse motion handler. Handles rotation (button 1) via arcball,
 * and pan (button 2) with proper world-space scaling.
 */
  static gboolean
on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  gl_view_state_t *state;
  float dx, dy;
  float scale;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->arcball )
    return( FALSE );

  if( state->arcball->drag_button == 0 )
    return( FALSE );

  if( state->arcball->drag_button == 1 )
  {
    /* Rotation handled by arcball */
    arcball_drag(state->arcball, event->x, event->y, state->viewport_height);
  }
  else if( state->arcball->drag_button == 2 )
  {
    /* Pan: compute pixel delta */
    dx = event->x - state->arcball->last_x;
    dy = event->y - state->arcball->last_y;

    /* Convert pixel delta to world-space delta at model plane.
     * Formula: 2 * distance * tan(fov/2) / viewport_height
     * Pan translation is post-scale in MVP chain, so model_scale omitted.
     * Aspect correction handled by projection matrix, not world-space conversion. */
    scale = 2.0f * state->cached_camera_distance *
            tanf(state->fov_rad / 2.0f) /
            state->viewport_height;

    state->pan_offset[0] += dx * scale;
    state->pan_offset[1] -= dy * scale;

    /* Update arcball last position for next delta */
    state->arcball->last_x = event->x;
    state->arcball->last_y = event->y;

    /* Notify arcball callbacks to trigger cross-view redraw */
    arcball_notify_changed(state->arcball);
  }
  else
  {
    return( FALSE );
  }

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
  double value, scale, zoom_percent;
  int viewport_width, viewport_height;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->zoom_spinbutton || !*state->zoom_spinbutton )
    return( FALSE );

  spinbutton = *state->zoom_spinbutton;
  value = gtk_spin_button_get_value(spinbutton);

  viewport_width = (int)(state->viewport_height * state->aspect);
  viewport_height = (int)state->viewport_height;
  zoom_percent = value;

  scale = compute_zoom_scale(viewport_width, viewport_height, zoom_percent);

  if( event->direction == GDK_SCROLL_UP )
    value *= (1.0 + 0.1 * scale);
  else if( event->direction == GDK_SCROLL_DOWN )
    value /= (1.0 + 0.1 * scale);
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
  state->last_generation = (unsigned int)-1;
  state->fov_rad = glm_rad(60.0f);
  state->aspect = 1.0f;
  state->viewport_height = 1.0f;

  /* Initialize per-view pan state */
  glm_vec2_zero(state->pan_offset);
  state->cached_camera_distance = 1.0f;

  gl_area = gtk_gl_area_new();

  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), TRUE);
  gtk_gl_area_set_auto_render(GTK_GL_AREA(gl_area), TRUE);

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

/* gl_view_set_arcball()
 *
 * Set the arcball reference for a view
 */
  void
gl_view_set_arcball(GtkWidget *widget, arcball_state_t *arcball)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state || !arcball )
    return;

  state->arcball = arcball;

} /* gl_view_set_arcball() */

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

/* gl_view_reset_pan()
 *
 * Reset pan offset to center the view
 */
  void
gl_view_reset_pan(GtkWidget *widget)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state )
    return;

  glm_vec2_zero(state->pan_offset);

} /* gl_view_reset_pan() */

/*-----------------------------------------------------------------------*/

/* Set_MSAA_Samples()
 *
 * Change MSAA sample count for all GL views
 */
  void
Set_MSAA_Samples(int samples)
{
  static char *msaa_widget_names[] = {
    "main_opengl_msaa_off",
    NULL,
    "main_opengl_msaa_2x",
    NULL,
    "main_opengl_msaa_4x",
    NULL, NULL, NULL,
    "main_opengl_msaa_8x",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "main_opengl_msaa_16x"
  };

  GtkWidget *widget;
  gl_view_state_t *state;

  rc_config.opengl_msaa_samples = samples;

  /* Update menu selection */
  if( samples <= 16 && msaa_widget_names[samples] )
  {
    widget = Builder_Get_Object(main_window_builder, msaa_widget_names[samples]);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
  }

  /* Recreate MSAA resources for structure view */
  if( structure_gl_area )
  {
    state = gl_view_get_state(structure_gl_area);
    if( state )
    {
      gtk_gl_area_make_current(GTK_GL_AREA(structure_gl_area));
      gl_view_recreate_msaa(state, samples);
      gtk_widget_queue_draw(structure_gl_area);
    }
  }

  /* Recreate MSAA resources for radiation pattern view */
  if( rdpattern_gl_area )
  {
    state = gl_view_get_state(rdpattern_gl_area);
    if( state )
    {
      gtk_gl_area_make_current(GTK_GL_AREA(rdpattern_gl_area));
      gl_view_recreate_msaa(state, samples);
      gtk_widget_queue_draw(rdpattern_gl_area);
    }
  }

} /* Set_MSAA_Samples() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
