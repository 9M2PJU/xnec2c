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
 * freqplots_input: Plots-window lifecycle and pointer interaction.
 *
 * Owns the deferred click event used to set the display frequency, the
 * window teardown path, and the mouse handling that moves the frequency
 * marker and resizes FR-card panels.
 */

#include "freqplots_internal.h"
#include "../shared.h"

#include <string.h>

static GdkEvent *prev_click_event = NULL;

/* Plots_Window_Killed()
 *
 * Cleans up after the plots window is closed
 */
  void
Plots_Window_Killed( void )
{
  // Reset this for next time otherwise width_available rescales in Plot_Graph will not work.
  fr_prev_width_available = 0;
  fr_prev_ngraphs = 0;

  if( isFlagSet(PLOT_ENABLED) )
  {
    ClearFlag( PLOT_ENABLED );
    freqplots_drawingarea = NULL;
    g_object_unref( freqplots_window_builder );
    freqplots_window_builder = NULL;

    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(
          Builder_Get_Object(main_window_builder, "main_freqplots")), FALSE );
  }
  freqplots_window = NULL;
  kill_window = NULL;

  if (fr_plots != NULL)
	  mem_free((void **)&fr_plots);

  if (prev_click_event != NULL)
	  mem_free((void **)&prev_click_event);

  fp_render_destroy();

} /* Plots_Window_Killed() */

/*-----------------------------------------------------------------------*/

void save_click_event(GdkEvent *e)
{
	if (prev_click_event == NULL)
		mem_alloc((void **)&prev_click_event, sizeof(GdkEvent));

	memcpy(prev_click_event, e, sizeof(GdkEvent));
}

GdkEvent *freqplots_pending_click(void)
{
	return prev_click_event;
}


/* Set_Frequecy_On_Click()
 *
 * Sets the current freq after click by user on plots drawingarea
 */
  void
Set_Frequency_On_Click( GdkEvent *e)
{
  double fmhz = 0.0;
  int set_fmhz = 0, draw_freqplot = 0;
  double x, w;
  int button, i;

  GdkEventButton *button_event = (GdkEventButton *)e;
  GdkEventScroll *scroll_event = (GdkEventScroll *)e;
  GdkEventMotion *motion_event = (GdkEventMotion *)e;

  // fr_plot: the plot where the mouse hovered:
  fr_plot_t *fr_plot = NULL;

  // fr_adj: the plot to adjust when using the mouse wheel:
  fr_plot_t *fr_adj = NULL;

  if (isFlagSet(SY_OPTIMIZER_ACTIVE))
	  return;

  if (fr_plots == NULL)
  {
	save_click_event(e);
	return;
  }

  // find the fr_plot structure that the mouse is within:
  for (i = 0; i < calc_data.ngraph * calc_data.FR_cards; i++)
  {
	if (   button_event->x >= fr_plots[i].plot_rect.x
		&& button_event->x <= fr_plots[i].plot_rect.x + fr_plots[i].plot_rect.width
		&& button_event->y >= fr_plots[i].plot_rect.y
		&& button_event->y <= fr_plots[i].plot_rect.y + fr_plots[i].plot_rect.height)
	{
		fr_plot = &fr_plots[i];
		break;
	}

  }

  // Decode the button, scroll direction, or button-drag state.
  button = 0;

  if (e->type == GDK_BUTTON_PRESS)
	  button = button_event->button;
  else if (e->type == GDK_SCROLL && scroll_event->direction == GDK_SCROLL_UP)
	  button = 4;
  else if (e->type == GDK_SCROLL && scroll_event->direction == GDK_SCROLL_DOWN)
	  button = 5;
  else if (e->type == GDK_MOTION_NOTIFY)
  {
  // Support holding the button down to drag the green line:
  if (motion_event->state & GDK_BUTTON1_MASK) button = 1;
	  else if (motion_event->state & GDK_BUTTON2_MASK) button = 2;
	  else if (motion_event->state & GDK_BUTTON3_MASK) button = 3;
  }

  if (fr_plot != NULL && FR_PLOT_T_IS_VALID(fr_plot))
  {
  // local shorthand variables
  double min_fscale = fr_plot->min_fscale;
  double max_fscale = fr_plot->max_fscale;

  /* Width of plot bounding rectangle */
  w = fr_plot->plot_rect.width;

  /* 'x' posn of click referred to plot bounding rectangle's 'x' */
  x = button_event->x - fr_plot->plot_rect.x;
  if( x < 0.0 ) x = 0.0;
  else if( x > w ) x = w;

  /* Calculate frequency corresponding to x position of click,
   * used for button 1 and 3: */
  fmhz = max_fscale - min_fscale;
  fmhz = min_fscale + fmhz * x / w;

  // event types: https://gitlab.gnome.org/GNOME/gtk/-/blob/gtk-3-24/gdk/gdkevents.h#L309
  // Enable this for mouse debugging:
  /*
  pr_debug("mouse click[type=%d pos=%4.1f,%4.1f button=%d(%d)]\n",
	  e->type,
	  button_event->x, button_event->y,
	  button, button_event->button);
  print_fr_plot(fr_plot);
  */

  /* Set freq corresponding to click 'x', to freq spinbuttons FIXME */
  switch( button )
  {
    case 1: /* Enable drawing of frequency line */

	  draw_freqplot = 1;
	  set_fmhz = 1;
      xnec2_widget_queue_draw( freqplots_drawingarea, TRUE );
      break;

    case 2: /* Disable drawing of freq line */
      calc_data.fmhz_save = 0.0;

	  draw_freqplot = 1;

      break;

    case 3: /* Calculate nearest frequency corresponding to mouse position in plot */
      i = (fmhz - fr_plot->freq_loop_data->min_freq) / fr_plot->freq_loop_data->delta_freq + 0.5;

      fmhz = fr_plot->freq_loop_data->min_freq + i * fr_plot->freq_loop_data->delta_freq;

      /* Enable drawing of frequency line */

	  draw_freqplot = 1;
	  set_fmhz = 1;
      break;


    case 4:  // scroll up
    case 5:  // scroll down
		// If its the last plot than shrink/grow the previous:
		if (fr_plot->fr == calc_data.FR_cards-1)
			fr_adj = get_fr_plot(fr_plot->posn, fr_plot->fr-1);

		// Otherwise shink/grow the next:
		else
			fr_adj = get_fr_plot(fr_plot->posn, fr_plot->fr+1);

		// Abort if there is only one plot:
		if (fr_adj == NULL)
			return;

		// the amount to adjust on scale:
		int px_adjust = 20;

		// scroll up
		if (button == 4)
		{
			if (fr_adj->plot_rect.width < 100)
			return;

			fr_adj->plot_rect.width -= px_adjust;
			fr_plot->plot_rect.width += px_adjust;
		}
		// scroll down
		else if (button == 5)
		{
			if (fr_plot->plot_rect.width < 100)
				return;

			fr_adj->plot_rect.width += px_adjust;
			fr_plot->plot_rect.width -= px_adjust;
		}

		// Sync widths for all positions based on fr_plot:
		fr_plot_sync_widths(fr_plot);

		draw_freqplot = 1;

		break;

		default:
			pr_debug("mouse button: unknown button %d\n", button);
			return;

  } /* switch( button_event->button ) */
  }
  else if (isFlagSet(PLOT_SMITH)
      && (button == 1 || button == 2 || button == 3)
      && fp_smith_freq_at_pixel(button_event->x, button_event->y,
          button == 3, &fmhz))
  {
    /* Smith chart is not an x-axis panel; the click maps onto the locus.
     * Primary interpolates the locus position; secondary snaps to a step. */
    if (button == 2)
    {
      calc_data.fmhz_save = 0.0;
      draw_freqplot = 1;
    }
    else
    {
      draw_freqplot = 1;
      set_fmhz = 1;
      if (button == 1)
        xnec2_widget_queue_draw( freqplots_drawingarea, TRUE );
    }
  }
  else
  {
	  // no plot_rect selected for frequency line
	save_click_event(e);
	  return;
  }

  if (set_fmhz)
  {
	  /* Round frequency to nearest 1 Hz */
	  uint64_t ifmhz = ( fmhz * 1e6 + 0.5 );
	  fmhz = ifmhz / 1e6;

	  user_set_frequency(fmhz);
  }

  // If prev_click_event is set then we are being called from Plot_Frequency_Data()
  // to update fmhz so don't redraw because Plot_Frequency_Data() is going to draw
  // after we return.
  if (draw_freqplot && prev_click_event == NULL)
    xnec2_widget_queue_draw( freqplots_drawingarea, TRUE );

  // Free the prev_click_event since it was serviced.
  if (prev_click_event != NULL)
	  mem_free((void **)&prev_click_event);

} /* Set_Freq_On_Click() */
