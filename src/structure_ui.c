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
  size_t mreq;

  /* Outer array — zero so realloc of sub-fields receives NULL, not garbage */
  mreq = (size_t)nfrq * sizeof(crnt_t);
  mem_realloc( (void **)&crnt_fstep, mreq, "in structure_ui.c" );
  memset( crnt_fstep, 0, mreq );

  for( int i = 0; i < nfrq; i++ )
  {
    /* Basis function coefficients: npm elements each */
    mreq = (size_t)data.npm * sizeof(double);
    mem_realloc( (void **)&crnt_fstep[i].air, mreq, "in structure_ui.c" );
    mem_realloc( (void **)&crnt_fstep[i].aii, mreq, "in structure_ui.c" );
    mem_realloc( (void **)&crnt_fstep[i].bir, mreq, "in structure_ui.c" );
    mem_realloc( (void **)&crnt_fstep[i].bii, mreq, "in structure_ui.c" );
    mem_realloc( (void **)&crnt_fstep[i].cir, mreq, "in structure_ui.c" );
    mem_realloc( (void **)&crnt_fstep[i].cii, mreq, "in structure_ui.c" );

    /* Solved current amplitudes: np3m elements */
    mreq = (size_t)data.np3m * sizeof(complex double);
    mem_realloc( (void **)&crnt_fstep[i].cur, mreq, "in structure_ui.c" );

  }

  alloc_struct_colors(nfrq);

} /* Alloc_Crnt_Fstep_Buffers() */

/*-----------------------------------------------------------------------*/

/**
 * Free_Crnt_Fstep_Buffers() - Free per-frequency-step crnt storage
 */
  void
Free_Crnt_Fstep_Buffers( void )
{
  if( crnt_fstep == NULL )
    return;

  int nfrq = calc_data.steps_total + 1;
  for( int i = 0; i < nfrq; i++ )
  {
    free_ptr( (void **)&crnt_fstep[i].air );
    free_ptr( (void **)&crnt_fstep[i].aii );
    free_ptr( (void **)&crnt_fstep[i].bir );
    free_ptr( (void **)&crnt_fstep[i].bii );
    free_ptr( (void **)&crnt_fstep[i].cir );
    free_ptr( (void **)&crnt_fstep[i].cii );
    free_ptr( (void **)&crnt_fstep[i].cur );
  }

  free_ptr( (void **)&crnt_fstep );
  free_struct_colors();

} /* Free_Crnt_Fstep_Buffers() */

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
      isFlagSet(PLOT_GVIEWER) &&
      isFlagClear(SUPPRESS_INTERMEDIATE_REDRAWS) )
  {
    xnec2_widget_queue_draw( freqplots_drawingarea, TRUE );
  }

} /* Queue_Structure_Redraw() */

/*-----------------------------------------------------------------------*/

/*  Init_Struct_Drawing()
 *
 *  Initializes drawing parameters after geometry input
 */
  void
Init_Struct_Drawing( void )
{
  /* We need n segs for wires + 4 edges per patch */
  size_t mreq = (size_t)(data.n + 4*data.m) * sizeof(Segment_t);
  mem_realloc( (void **)&structure_segs, mreq, "in structure_ui.c" );
  Prerender_Aggregate();

  /* Viewport only exists in the UI process; children skip this harmlessly.
   * Guard: skip if no geometry loaded (segments or patches required). */
  if( (data.n > 0 || data.m > 0) && structure_view != NULL )
    view_set_viewport( structure_view, structure_width, structure_height );
}

/*-----------------------------------------------------------------------*/
