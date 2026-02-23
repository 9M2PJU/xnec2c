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

#include "opengl_view_input.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/* Forward declarations */
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);

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

    state->drag_active = TRUE;
    gtk_widget_queue_draw(widget);

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

  state->drag_active = FALSE;
  gtk_widget_queue_draw(widget);

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
  int drag_button;

  state = (gl_view_state_t *)user_data;

  if( !state || !state->arcball )
    return( FALSE );

  drag_button = arcball_get_drag_button(state->arcball);

  if( drag_button == 0 )
    return( FALSE );

  if( drag_button == 1 )
  {
    /* Rotation handled by arcball */
    arcball_drag(state->arcball, event->x, event->y, state->viewport_height);
  }
  else if( drag_button == 2 )
  {
    /* Pan: compute pixel delta from last recorded position */
    float last_x, last_y;

    arcball_get_last_pos(state->arcball, &last_x, &last_y);

    dx = event->x - last_x;
    dy = event->y - last_y;

    /* Convert pixel delta to world-space delta at model plane.
     * Formula: 2 * distance * tan(fov/2) / viewport_height
     * Pan translation is post-scale in MVP chain, so model_scale omitted.
     * Aspect correction handled by projection matrix, not world-space conversion. */
    scale = 2.0f * state->cached_camera_distance *
            tanf(state->fov_rad / 2.0f) /
            state->viewport_height;

    state->pan_offset[0] += dx * scale;
    state->pan_offset[1] -= dy * scale;

    arcball_update_last_pos(state->arcball, event->x, event->y);

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
 * Mouse scroll handler.
 * Shift+scroll invokes scene-specific handler if provided.
 * Normal scroll adjusts primary zoom via spinbutton.
 */
  static gboolean
on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
  gl_view_state_t *state;
  GtkSpinButton *spinbutton;
  double value, scale, zoom_percent;
  int viewport_width, viewport_height;

  state = (gl_view_state_t *)user_data;

  if( !state )
    return( FALSE );

  /* Ctrl+scroll: invoke segment radius scaling handler */
  if( (event->state & GDK_CONTROL_MASK) && state->scene->on_ctrl_scroll )
  {
    return( state->scene->on_ctrl_scroll(widget, event, state) );
  }

  /* Shift+scroll: invoke scene-specific handler */
  if( (event->state & GDK_SHIFT_MASK) && state->scene->on_shift_scroll )
  {
    return( state->scene->on_shift_scroll(widget, event, state) );
  }

  /* Normal scroll: adjust primary zoom */
  if( !state->zoom_spinbutton || !*state->zoom_spinbutton )
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

/* gl_view_input_connect()
 *
 * Wire input signal handlers to GL area widget
 */
  void
gl_view_input_connect(GtkWidget *gl_area, gl_view_state_t *state)
{
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

} /* gl_view_input_connect() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
