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

#ifdef HAVE_OPENGL

/* Triangle buffer for radiation pattern mesh */
static color_triangle_t *rdpat_triangles = NULL;
static int rdpat_triangle_count = 0;

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

  gl_instance_free(state->gl);
  g_free(state);

} /* opengl_rdpattern_state_free() */

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

/* on_realize()
 *
 * GtkGLArea realize signal handler - initializes OpenGL context
 */
  static void
on_realize(GtkGLArea *area)
{
  rdpattern_gl_state_t *state;
  GtkAllocation alloc;
  float aspect;

  gtk_gl_area_make_current(area);

  if( gtk_gl_area_get_error(area) != NULL )
  {
    pr_err("GL context error\n");
    return;
  }

  gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
  aspect = (float)alloc.width / (float)alloc.height;

  state = opengl_rdpattern_state_new(aspect);
  if( !state )
  {
    pr_err("Failed to create GL state\n");
    return;
  }

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
  unsigned int current_gen;
  double r_min, r_range;

  state = g_object_get_data(G_OBJECT(area), "gl_state");
  if( !state || !state->gl || !state->gl->initialized )
  {
    /* GL not ready */
    return( FALSE );
  }

  current_gen = Generate_Rdpattern_Data(&r_min, &r_range);
  if( current_gen == 0 )
    return( FALSE );

  if( current_gen != state->gl_last_gen )
  {
    rdpattern_data_t rd_data;
    if( Get_Radiation_Pattern_Data(&rd_data) && rd_data.valid )
    {
      int tri_count = opengl_rdpattern_generate_triangles(
          rd_data.points, rd_data.nth, rd_data.nph, r_min, r_range);
      if( tri_count > 0 )
      {
        opengl_rdpattern_update_buffers(state);

        float r_max = (float)rd_data.r_max;
        float distance = r_max * 2.165f;
        arcball_set_distance(state->gl->arcball, distance);

        state->gl_last_gen = current_gen;
      }
      else
      {
        pr_err("Triangle generation failed\n");
        return( FALSE );
      }
    }
    else
    {
      pr_err("Failed to get radiation pattern data\n");
      return( FALSE );
    }
  }

  if( state->triangle_count == 0 )
    return( FALSE );

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  arcball_get_mvp(state->gl->arcball, mvp, 1.0f, 60.0f);

  glUseProgram(state->gl->shader.program);
  glUniformMatrix4fv(state->gl->mvp_location, 1, GL_FALSE, (float*)mvp);

  glBindVertexArray(state->gl->vao);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glDrawArrays(GL_TRIANGLES, 0, state->triangle_count * 3);

  glBindVertexArray(0);
  glUseProgram(0);

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

  /* State freed automatically by GDestroyNotify */

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
  rdpattern_gl_state_t *state;
  float delta;

  state = g_object_get_data(G_OBJECT(widget), "gl_state");
  if( !state || !state->gl )
    return( FALSE );

  if( event->direction == GDK_SCROLL_UP )
    delta = -0.3f;
  else if( event->direction == GDK_SCROLL_DOWN )
    delta = 0.3f;
  else
    return( FALSE );

  arcball_zoom(state->gl->arcball, delta);
  gtk_widget_queue_draw(widget);
  return( TRUE );
}

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

} /* opengl_rdpattern_cleanup() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
