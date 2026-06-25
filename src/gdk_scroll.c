/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  The official website and doumentation for xnec2c is available here:
 *    https://www.xnec2c.org/
 */

/*
 * gdk_scroll: one scroll resolver and one capture-phase spin consumer.
 *
 * A wheel notch and a trackpad frame both reduce to a scroll_step_t carrying a
 * direction and a magnitude.  GtkSpinButton swallows bubble-phase smooth scroll
 * through its own internal controller, so each spin receives a capture-phase
 * GtkEventControllerScroll that resolves the frame before the widget consumes
 * it.  A continuous-capable spin (fractional digits) applies the magnitude
 * directly; a quantized spin (digits == 0) floors at one quantum and stores no
 * residual.
 */

#include "gdk_scroll.h"

#include <math.h>

/* Reduce smooth-axis deltas to a single dominant-axis step.  GDK normalises a
 * wheel notch to a summed delta near 1.0, so a discrete notch and a smooth
 * frame share one magnitude scale. */
static scroll_step_t
scroll_step_from_xy(double dx, double dy)
{
  scroll_step_t s = { FALSE, GDK_SCROLL_SMOOTH, 0.0 };

  if (fabs(dy) >= fabs(dx))
  {
    if (dy == 0.0)
      return s;
    s.direction = (dy < 0.0) ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
    s.step      = fabs(dy);
  }
  else
  {
    s.direction = (dx < 0.0) ? GDK_SCROLL_LEFT : GDK_SCROLL_RIGHT;
    s.step      = fabs(dx);
  }

  s.active = TRUE;
  return s;
}

/* Resolve a GdkEventScroll into a step.  A smooth event yields its deltas; a
 * discrete event yields a unit step in its declared direction. */
scroll_step_t
scroll_step_from_deltas(GdkEvent *event)
{
  scroll_step_t s = { FALSE, GDK_SCROLL_SMOOTH, 0.0 };
  GdkScrollDirection direction;
  double dx, dy;

  if (gdk_event_get_scroll_deltas(event, &dx, &dy))
    return scroll_step_from_xy(dx, dy);

  if (!gdk_event_get_scroll_direction(event, &direction))
    return s;

  s.active    = TRUE;
  s.direction = direction;
  s.step      = 1.0;
  return s;
}

/* Capture-phase scroll consumer for one spin.  Reads the spin's own precision:
 * digits == 0 quantizes (whole units of the wheel/override grain, floored at
 * one), digits > 0 applies the magnitude continuously against the adjustment
 * step.  Stores no residual. */
static gboolean
spin_notch_scroll_cb(GtkEventControllerScroll *controller,
    double dx, double dy, gpointer user_data)
{
  spin_scroll_ctx_t *ctx = user_data;
  GtkWidget *widget =
      gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  GtkSpinButton *spin = GTK_SPIN_BUTTON(widget);
  GtkAdjustment *adj = gtk_spin_button_get_adjustment(spin);
  scroll_step_t s = scroll_step_from_xy(dx, dy);
  double value, dir, grain;
  guint digits;

  if (!s.active ||
      (s.direction != GDK_SCROLL_UP && s.direction != GDK_SCROLL_DOWN))
    return FALSE;

  value  = gtk_spin_button_get_value(spin);
  digits = gtk_spin_button_get_digits(spin);
  dir    = (s.direction == GDK_SCROLL_UP) ? 1.0 : -1.0;

  if (digits == 0)
  {
    /* Wheel notch (s.step >= 1.0): apply override grain or adjustment step.
     * Smooth trackpad frame (s.step < 1.0): move one quantum (10^-digits). */
    if (s.step >= 1.0)
      grain = (ctx->wheel_grain != 0.0)
                  ? ctx->wheel_grain
                  : gtk_adjustment_get_step_increment(adj);
    else
      grain = gtk_adjustment_get_step_increment(adj);
    value += dir * grain;
  }
  else if (digits > 0)
  {
    grain = gtk_adjustment_get_step_increment(adj);
    value += dir * s.step * grain;
  }

  gtk_spin_button_set_value(spin, value);
  return TRUE;
}

/* Attach a capture-phase scroll controller to one spin, replacing any prior
 * controller installed by an earlier call.  The closure lifetime is tied to
 * the spin through the "scroll_controller" object-data slot, whose destroy
 * notify both detaches the old controller and frees its closure. */
void
scroll_install_spin_notches(GtkSpinButton *spin, double wheel_grain)
{
  spin_scroll_ctx_t *ctx = g_new0(spin_scroll_ctx_t, 1);
  GtkEventController *controller;

  ctx->wheel_grain = wheel_grain;

  controller = gtk_event_controller_scroll_new(GTK_WIDGET(spin),
      GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
  g_signal_connect_data(controller, "scroll",
      G_CALLBACK(spin_notch_scroll_cb), ctx,
      (GClosureNotify)g_free, 0);

  /* Reinstalling drops the prior controller: unref destroys its closure,
   * which frees the prior ctx via the GClosureNotify above. */
  g_object_set_data_full(G_OBJECT(spin), "scroll_controller",
      controller, g_object_unref);
}

/* Sweep one capture-phase controller onto every spin built through a builder.
 * A 0.0 grain leaves each spin reading its own adjustment step-increment. */
void
scroll_install_all_spins(GtkBuilder *builder)
{
  GSList *objects = gtk_builder_get_objects(builder);
  GSList *node;

  for (node = objects; node != NULL; node = node->next)
  {
    if (GTK_IS_SPIN_BUTTON(node->data))
      scroll_install_spin_notches(GTK_SPIN_BUTTON(node->data), 0.0);
  }

  g_slist_free(objects);
}
