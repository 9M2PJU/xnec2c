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

/*
 * structure_ui: Structure-window UI helpers and initialization.
 *
 * Houses viewer-gain display, frequency-step current buffers,
 * structure redraw queuing, and Init_Struct_Drawing which wires
 * prerender to the 2D projection buffer.
 */
#include "structure_ui.h"
#include "shared.h"
#include "callbacks.h"
#include "cairo/cairo_draw.h"
#include "prerender/prerender_aggregate.h"
#include "prerender/prerender_color.h"
#include "view/view_core.h"

/*-----------------------------------------------------------------------*/

/**
 * Draw_Structure_UI() - Update structure-window UI readouts
 *
 * Writes the viewer-gain entry, frequency-step label, and color-code
 * max label for the main window.  Called under freq_data_lock by
 * render() after dispatching to the backend.
 */
  void
Draw_Structure_UI(void)
{
  Show_Viewer_Gain(
      main_window_builder,
      "main_gain_entry",
      structure_view );

  if( calc_data.freq_step >= 0 )
    Display_Fstep( structure_fstep_entry, calc_data.freq_step );

  /* Update structure-window color code max label for current/charge display */
  int fstep = calc_data.freq_step;
  if( CRNT_FSTEP_AVAILABLE(fstep) && struct_colors )
  {
    char label[16];
    gboolean do_update = FALSE;

    if( isFlagSet(DRAW_CURRENTS) )
    {
      snprintf( label, sizeof(label) - 1, "%8.2E",
          (double)struct_colors[fstep].wire_crnt_cmax * (double)data.wlam );
      do_update = TRUE;
    }
    else if( isFlagSet(DRAW_CHARGES) )
    {
      snprintf( label, sizeof(label) - 1, "%8.2E",
          (double)struct_colors[fstep].wire_chrg_cmax * 1.0E-6 / (double)calc_data.freq_mhz );
      do_update = TRUE;
    }
    /* else geometry mode: label retains previous value */

    if( do_update )
      gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
              main_window_builder, "main_colorcode_maxlabel")), label );
  }
  /* else no current data: label retains previous value */
}

/*-----------------------------------------------------------------------*/

/* Show_Viewer_Gain()
 *
 * Shows gain in direction of viewer
 */
  void
Show_Viewer_Gain(
    GtkBuilder *builder,
    gchar *widget,
    view_t *v )
{
  if( isFlagSet(DRAW_CURRENTS) ||
      isFlagSet(DRAW_CHARGES)  ||
      isFlagSet(DRAW_GAIN)     ||
      isFlagSet(FREQ_LOOP_RUNNING) )
  {
    char txt[16];
    if( isFlagSet(ENABLE_RDPAT) && (calc_data.freq_step >= 0) )
    {
      snprintf( txt, sizeof(txt)-1, "%.2f", Viewer_Gain(v, calc_data.freq_step) );
      gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(builder, widget)), txt );
    }
  }

} /* Show_Viewer_Gain() */

/*-----------------------------------------------------------------------*/

/**
 * free_crnt_step() - Release one fstep's crnt sub-buffers
 * @elem: pointer to one crnt_t element
 */
static void
free_crnt_step(void *elem)
{
  crnt_t *c = elem;
  mem_free(&c->air);
  mem_free(&c->aii);
  mem_free(&c->bir);
  mem_free(&c->bii);
  mem_free(&c->cir);
  mem_free(&c->cii);
  mem_free(&c->cur);
}

/*-----------------------------------------------------------------------*/

/**
 * Alloc_Crnt_Fstep_Buffers() - Allocate per-frequency-step crnt storage
 *
 * @nfrq: Number of frequency steps (steps_total + 1)
 *
 * Allocates crnt_fstep[] array so each frequency step can store
 * its computed current/charge data for later restoration.
 */
  void
Alloc_Crnt_Fstep_Buffers( int nfrq )
{
  /* Resize the outer array, freeing only the shrink tail; surviving
   * entries keep their sub-buffers for reuse by the inner alloc loop. */
  mem_array_resize(&crnt_fstep, nfrq, free_crnt_step);

  for( int i = 0; i < nfrq; i++ )
  {
    mem_array_realloc(&crnt_fstep[i].air, data.npm);
    mem_array_realloc(&crnt_fstep[i].aii, data.npm);
    mem_array_realloc(&crnt_fstep[i].bir, data.npm);
    mem_array_realloc(&crnt_fstep[i].bii, data.npm);
    mem_array_realloc(&crnt_fstep[i].cir, data.npm);
    mem_array_realloc(&crnt_fstep[i].cii, data.npm);
    mem_array_realloc(&crnt_fstep[i].cur, data.np3m);

  }

  alloc_struct_colors(nfrq);

} /* Alloc_Crnt_Fstep_Buffers() */

/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

/**
 * Save_Crnt_Data() - Save current crnt data for a frequency step
 *
 * @fstep: Frequency step index
 *
 * Copies the global crnt struct arrays into crnt_fstep[fstep].
 */
  void
Save_Crnt_Data( int fstep )
{
  if( crnt_fstep == NULL
      || fstep < 0 || fstep > calc_data.steps_total
      || crnt_fstep[fstep].cur == NULL )
    return;

  size_t npm = (size_t)data.npm;
  size_t np3m = (size_t)data.np3m;

  memcpy( crnt_fstep[fstep].air, crnt.air, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].aii, crnt.aii, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].bir, crnt.bir, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].bii, crnt.bii, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].cir, crnt.cir, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].cii, crnt.cii, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].cur, crnt.cur, np3m * sizeof(complex double) );

} /* Save_Crnt_Data() */

/*-----------------------------------------------------------------------*/

/*  Queue_Structure_Redraw()
 *
 *  Queues a redraw of the structure drawingarea and, when a
 *  "viewer gain" plot is active, the frequency-plot area.
 *
 *  Called by view_t observers when rotation, pan or zoom change.
 *  No projection-cache state is maintained here: view_R(), the
 *  pan_offset and the zoom are read directly at draw time.
 */
  void
Queue_Structure_Redraw(void)
{
  /* Trigger a redraw of structure drawingarea */
  if( structure_drawingarea )
    xnec2_widget_queue_draw( structure_drawingarea, TRUE );

  /* Trigger a redraw of plots drawingarea */
  if( isFlagSet(PLOT_ENABLED) &&
      (isFlagSet(PLOT_GVIEWER) || freqplots_popup_open(FP_PANEL_VIEWER)) &&
      isFlagClear(SUPPRESS_INTERMEDIATE_REDRAWS) )
  {
    freqplots_redraw_all(TRUE);
  }

} /* Queue_Structure_Redraw() */

/*-----------------------------------------------------------------------*/

/** structure_view_changed_cb() - view_t change callback for structure view
 * @v:           view that changed
 * @_user_data:  unused
 *
 * Invoked by view_notify_change() whenever rotation, pan, zoom, or extent
 * changes.  Refreshes the WR/WI spin display and queues a structure redraw.
 * Propagation to the rdpattern view under common-projection sharing is
 * handled internally by view_t via the rotation_follower link established
 * by view_share_master().  Bound as changed_cb at view_new() in main.c.
 */
  void
structure_view_changed_cb(view_t *v, gpointer _user_data)
{
  (void)_user_data;

  view_update_spin_display( v );
  Queue_Structure_Redraw();

} /* structure_view_changed_cb() */

/*-----------------------------------------------------------------------*/

/** Animate_Phase() - Unified animation tick callback
 * @_udata: unused
 *
 * Returns G_SOURCE_REMOVE immediately when ANIMATE is cleared.
 * Otherwise advances flow_phase by one anim_step, dispatches to all
 * active animation consumers, then queues redraws.  Animate_Phase owns
 * all queue decisions; compute_near_field_frame() is pure computation.
 */
  gboolean
Animate_Phase(gpointer _udata)
{
  if( isFlagClear(ANIMATE) )
  {
    anim_tag = 0;
    return( G_SOURCE_REMOVE );
  }

  flow_phase += (float)near_field.anim_step;
  if( flow_phase >= (float)M_2PI )
    flow_phase -= (float)M_2PI;

  apply_animation_phase();

  return( G_SOURCE_CONTINUE );

} /* Animate_Phase() */

/*-----------------------------------------------------------------------*/

/** apply_animation_phase() - Render structure and pattern at the current phase
 *
 * Shared by the timer tick and the manual phase slider.  Reads flow_phase
 * without modifying it: computes the near-field frame when EH field
 * visualization is active, queues the structure drawing area, and queues the
 * radiation-pattern drawing area for near-field or patch-arrow overlay.
 */
  void
apply_animation_phase(void)
{
  /* Near-field computation only runs when EH field visualization is active */
  if( isFlagSet(DRAW_EHFIELD) )
    compute_near_field_frame((double)flow_phase);

  xnec2_widget_queue_draw( structure_drawingarea, TRUE );

  /* Queue rdpattern for near-field visualization or patch arrow overlay */
  if( isFlagSet(DRAW_EHFIELD) ||
      (isFlagSet(OVERLAY_STRUCT) && data.m > 0) )
    xnec2_widget_queue_draw( rdpattern_drawingarea, TRUE );

  /* Update the phase readout from flow_phase without moving the slider, whose
   * position stays static while the timer animation runs.  The readout is
   * decoupled from the slider thumb.  Slider reads degrees; flow_phase stores
   * radians. */
  if( animate_dialog != NULL )
  {
    GtkLabel *readout = GTK_LABEL( Builder_Get_Object(
          animate_dialog_builder, "animate_phase_value") );
    gchar *text = g_strdup_printf( "φ %.0f°", (gdouble)flow_phase * TODEG );
    gtk_label_set_text( readout, text );
    g_free( text );
  }

} /* apply_animation_phase() */

/*-----------------------------------------------------------------------*/

/** reset_animation_phase() - Zero the shared animation phase
 *
 * Called when animation stops to return arrows to reference direction.
 */
  void
reset_animation_phase(void)
{
  flow_phase = 0.0f;
}

/*-----------------------------------------------------------------------*/

/*  Init_Struct_Drawing()
 *
 *  Initializes drawing parameters after geometry input
 */
  void
Init_Struct_Drawing( void )
{
  mem_array_realloc(&structure_segs, (data.n + 4 * data.m));
  Prerender_Aggregate();

  /* Viewport only exists in the UI process; children skip this harmlessly.
   * Guard: skip if no geometry loaded (segments or patches required). */
  if( (data.n > 0 || data.m > 0) && structure_view != NULL )
    view_set_viewport( structure_view, structure_width, structure_height );
}

/*-----------------------------------------------------------------------*/
