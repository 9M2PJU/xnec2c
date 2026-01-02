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

#ifdef HAVE_OPENGL

static rdpattern_gl_state_t state = {0};

/*-----------------------------------------------------------------------*/

/* on_realize()
 *
 * GtkGLArea realize signal handler - initializes OpenGL context
 */
  static void
on_realize(GtkGLArea *area)
{
  gboolean ok;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error\n");
    return;
  }

  ok = gl_shader_load(&state.shader,
    "/gl/color-vertex.glsl",
    "/gl/color-fragment.glsl");

  if( !ok )
  {
    pr_err("Shader load failed\n");
    return;
  }

  state.mvp_location = glGetUniformLocation(
    state.shader.program, "mvp");
  state.position_location = glGetAttribLocation(
    state.shader.program, "position");
  state.color_location = glGetAttribLocation(
    state.shader.program, "color");

  glGenVertexArrays(1, &state.vao);
  glGenBuffers(1, &state.vbo);

  arcball_init(&state.arcball, 3.0f);
  state.initialized = TRUE;

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
  if( !state.initialized )
  {
    /* GL not ready */
    return( FALSE );
  }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return( TRUE );

} /* on_render() */

/*-----------------------------------------------------------------------*/

/* on_unrealize()
 *
 * GtkGLArea unrealize signal handler - cleanup resources
 */
  static void
on_unrealize(GtkGLArea *area)
{
  gtk_gl_area_make_current(area);

  if( state.vbo )
    glDeleteBuffers(1, &state.vbo);
  if( state.vao )
    glDeleteVertexArrays(1, &state.vao);

  gl_shader_destroy(&state.shader);

  state.initialized = FALSE;

} /* on_unrealize() */

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

  g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize), NULL);
  g_signal_connect(gl_area, "render", G_CALLBACK(on_render), NULL);
  g_signal_connect(gl_area, "unrealize", G_CALLBACK(on_unrealize), NULL);

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
  state.triangle_count = 0;

} /* opengl_rdpattern_cleanup() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
