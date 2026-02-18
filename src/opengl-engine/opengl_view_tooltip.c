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
#include "opengl_cairo_overlay.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/* gl_view_render_tooltip()
 *
 * Render tooltip overlay with fade animation
 */
  void
gl_view_render_tooltip(gl_view_state_t *state, int surf_width, int surf_height)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_text_extents_t extents;
  double x, y, padding;

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surf_width, surf_height);
  cr = cairo_create(surface);

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 16.0);

  cairo_text_extents(cr, state->tooltip_text, &extents);

  padding = 12.0;
  x = (surf_width - extents.width) / 2.0 - extents.x_bearing;
  y = (surf_height - extents.height) / 2.0 - extents.y_bearing;

  /* Background box with fade */
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7 * state->tooltip_alpha);
  cairo_rectangle(cr,
      x + extents.x_bearing - padding,
      y + extents.y_bearing - padding,
      extents.width + 2.0 * padding,
      extents.height + 2.0 * padding);
  cairo_fill(cr);

  /* Text with fade */
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, state->tooltip_alpha);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, state->tooltip_text);

  cairo_destroy(cr);

  /* Lazily allocate cached tooltip overlay */
  if( !state->tooltip_overlay )
    state->tooltip_overlay = cairo_gl_overlay_new();

  if( state->tooltip_overlay )
  {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    cairo_gl_overlay_set_size(state->tooltip_overlay, surf_width, surf_height);
    cairo_gl_overlay_upload(state->tooltip_overlay, surface);
    cairo_gl_overlay_render(state->tooltip_overlay);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
  }

  cairo_surface_destroy(surface);

} /* gl_view_render_tooltip() */

/*-----------------------------------------------------------------------*/

/* tooltip_update_callback()
 *
 * Timer callback for tooltip fade animation.
 * Tooltip stays visible for tooltip_hold_ms, then fades over 500ms.
 */
  static gboolean
tooltip_update_callback(gpointer user_data)
{
  GtkWidget *widget;
  gl_view_state_t *state;
  gint64 current_time, elapsed_ms;
  double progress;

  widget = GTK_WIDGET(user_data);
  state = gl_view_get_state(widget);

  if( !state || !state->tooltip_active )
    return( FALSE );

  current_time = g_get_monotonic_time();
  elapsed_ms = (current_time - state->tooltip_start_time) / 1000;

  /* Stay visible for configured hold duration */
  if( elapsed_ms < state->tooltip_hold_ms )
  {
    state->tooltip_alpha = 1.0;
    gtk_widget_queue_draw(widget);
    return( TRUE );
  }

  /* Fade over 500ms after hold period */
  progress = (double)(elapsed_ms - state->tooltip_hold_ms) / 500.0;

  if( progress >= 1.0 )
  {
    state->tooltip_active = FALSE;
    state->tooltip_alpha = 0.0;
    state->tooltip_timeout_id = 0;
    gtk_widget_queue_draw(widget);
    return( FALSE );
  }

  /* Linear fade from 1.0 to 0.0 */
  state->tooltip_alpha = 1.0 - progress;

  gtk_widget_queue_draw(widget);

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/* gl_view_show_tooltip()
 *
 * Display a tooltip message that holds for duration_ms then fades over 500ms
 */
  void
gl_view_show_tooltip(GtkWidget *widget, const char *text, int duration_ms)
{
  gl_view_state_t *state;

  state = gl_view_get_state(widget);

  if( !state || !text )
    return;

  /* Remove existing tooltip timer */
  if( state->tooltip_timeout_id )
  {
    g_source_remove(state->tooltip_timeout_id);
    state->tooltip_timeout_id = 0;
  }

  g_free(state->tooltip_text);
  state->tooltip_text = g_strdup(text);
  state->tooltip_active = TRUE;
  state->tooltip_alpha = 1.0;
  state->tooltip_hold_ms = duration_ms;
  state->tooltip_start_time = g_get_monotonic_time();

  /* Timer fires every 16ms (60fps) for smooth fade */
  state->tooltip_timeout_id = g_timeout_add(16, tooltip_update_callback, widget);

  gtk_widget_queue_draw(widget);

} /* gl_view_show_tooltip() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
