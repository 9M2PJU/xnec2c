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
 * Net gain added by Mark Whitis http://www.freelabs.com/~whitis/
 * References:
 *   http://www.digitalhome.ca/forum/showpost.php?p=744018&postcount=47
 *      NetGain = RawGain+10*log(Feed-pointGain)
 *      where Feed-point Gain = 4*Zr*Zo/((Zr+Zo)^2+Zi^2)
 *   http://www.avsforum.com/avs-vb/showthread.php?p=14086104#post14086104
 *      NetGain = RawGain+10*log(4*Zr*Zo/((Zr+Zo)^2+Zi^2)
 *   Where log is log10.
 */

/*
 * freqplots_core: Frequency-plots data orchestration.
 *
 * Owns the fr_plot panel table, gathers per-frequency measurements, and
 * drives the per-type plot renderers.  Each frame resets the depth-buffered
 * render state, deposits all geometry and deferred text through the plot
 * types, then flushes so labels overlay every segment layer.
 */

#include "freqplots_internal.h"
#include "../shared.h"
#include "../opt_ui.h"

#include <string.h>

fr_plot_t *fr_plots = NULL;

static void Display_Frequency_Data( void );

/* One plot-type dispatch row: a guard predicate gating an accumulating
 * renderer.  fp_run_dispatch() invokes render only when enabled is true;
 * render deposits segments and defers text, returning FALSE to abort the
 * frame (e.g. on allocation failure). */
typedef struct
{
  int      (*enabled)(void);
  gboolean (*render)(fp_plot_ctx_t *ctx);
} fp_plot_dispatch_t;

/* Dispatch order fixes the top-to-bottom panel layout of the plots window. */
static const fp_plot_dispatch_t fp_plot_dispatch[] = {
  { fp_gain_enabled,      fp_gain_render      },
  { fp_viewer_enabled,    fp_viewer_render    },
  { fp_vswr_enabled,      fp_vswr_render      },
  { fp_impedance_enabled, fp_impedance_render },
  { fp_ant_temp_enabled,  fp_ant_temp_render  },
};

/* Copy each bound column out of the shared per-frame measurement rows so all
 * plot types draw their per-frequency values from the one measurement source.
 * Each column reads meas_rows[i].a[meas_idx]. */
  void
fp_fill_meas_columns(fp_plot_ctx_t *ctx,
    const fp_meas_column_t *cols, int ncols)
{
  int i, c;

  for( i = 0; i < ctx->num_fsteps; i++ )
    for( c = 0; c < ncols; c++ )
      cols[c].dst[i] = ctx->meas_rows[i].a[cols[c].meas_idx];
}

void fr_plots_init(void)
{
  int idx;

  /* 2d array of plot rectangles popluated by the Plot_Graph function */
  if (calc_data.ngraph > 0 && calc_data.FR_cards > 0)
	  mem_realloc((void **)&fr_plots,
                      sizeof(fr_plot_t) * calc_data.ngraph * calc_data.FR_cards);
  else
  {
	  mem_free((void **)&fr_plots);
	  return;
  }

  for (idx = 0; idx < calc_data.ngraph * calc_data.FR_cards; idx++)
  {
	  if (FR_PLOT_T_IS_VALID(&fr_plots[idx]))
		  continue;

	  // Set the plot position
	  fr_plots[idx].posn = idx / calc_data.FR_cards;

	  // Point to the freq loop data
	  fr_plots[idx].fr = idx % calc_data.FR_cards;
	  fr_plots[idx].freq_loop_data = &calc_data.freq_loop_data[fr_plots[idx].fr];

	  // zero the plot_rect, Plot_Graph() will fill it in.
	  memset(&fr_plots[idx].plot_rect, 0, sizeof(GdkRectangle));

	  // Set it as valid:
	  fr_plots[idx].valid = FR_PLOT_T_MAGIC;
  }
}

fr_plot_t *get_fr_plot(int posn, int fr)
{
	if (posn < 0 || posn >= calc_data.ngraph ||	fr < 0 || fr >= calc_data.FR_cards)
		return NULL;

	return &fr_plots[posn*calc_data.FR_cards + fr];
}

GdkRectangle *get_plot_rect(int posn, int fr)
{
	fr_plot_t *p = get_fr_plot(posn, fr);

	if (p == NULL)
		return NULL;

	return &p->plot_rect;
}

void fr_plot_sync_widths(fr_plot_t *fr_plot)
{
	GdkRectangle *current, *r;
	int posn, fr;

	if (fr_plot == NULL || fr_plots == NULL)
		return;

	// Update all plot widths so they re the same as fr_plot's width.
	for (posn = 0; posn < calc_data.ngraph; posn++)
	{
		// Skip the current one the mouse adjusted:
		if (posn == fr_plot->posn)
			continue;

		for (fr = 0; fr < calc_data.FR_cards; fr++)
		{
			current = get_plot_rect(fr_plot->posn, fr);
			if (current == NULL)
			{
				BUG("fr_plot_sync_widths, current == NULL: calc_data.FR_cards=%d calc_data.ngraph=%d fr=%d posn=%d valid=%d\n",
					calc_data.FR_cards, calc_data.ngraph,
					fr, fr_plot->posn,
					FR_PLOT_T_IS_VALID(fr_plot));
				return;
			}
			r = get_plot_rect(posn, fr);

			r->width = current->width;
		}
	}

}

void print_fr_plot(fr_plot_t *p)
{
	pr_debug("fr_plot[posn=%d fr=%d] rect[x=%d y=%d, w=%d h=%d] freq[min=%4.2f max=%4.2f n=%d]\n",
		p->posn, p->fr, p->plot_rect.x, p->plot_rect.y,
		p->plot_rect.width, p->plot_rect.height,
		p->freq_loop_data->min_freq, p->freq_loop_data->max_freq,
		p->freq_loop_data->freq_steps);
}

/**
 * fmhz_within_display_range - test whether a frequency lies within any visible plot panel
 * @fmhz: frequency in MHz to test
 *
 * Iterates the fr_plots array and returns TRUE when @fmhz falls within the
 * display scale range (min_fscale..max_fscale) of any valid panel.  The display
 * range can be wider than the underlying FR card data range (e.g. when a single-
 * frequency card is expanded by Fit_to_Scale for visualization).
 *
 * Returns FALSE when fr_plots is not yet initialized or no panel covers @fmhz.
 */
gboolean
fmhz_within_display_range( double fmhz )
{
  int i, n;

  if (fr_plots == NULL || calc_data.ngraph < 1 || calc_data.FR_cards < 1)
    return FALSE;

  n = calc_data.ngraph * calc_data.FR_cards;
  for (i = 0; i < n; i++)
  {
    if (!FR_PLOT_T_IS_VALID(&fr_plots[i]))
      continue;

    if (fmhz >= fr_plots[i].min_fscale - 1e-6
        && fmhz <= fr_plots[i].max_fscale + 1e-6)
      return TRUE;
  }

  return FALSE;
}

/*-----------------------------------------------------------------------*/

/* Display_Frequency_Data()
 *
 * Displays freq dependent data (gain, impedance etc)
 * in the entry widgets in the freq plots window
 */
  static void
Display_Frequency_Data( void )
{
  int fstep;
  double vswr, zreal, zimag;
  char txt[16];

  measurement_t meas;

  if( isFlagClear(PLOT_ENABLED) ) return;

  /* Limit freq stepping to freq_steps FIXME */
  fstep = calc_data.freq_step;

  if (fstep < 0)
	  return;

  if( fstep >= calc_data.steps_total )
    fstep = calc_data.steps_total;

  meas_calc(&meas, fstep);

  /* Display max gain */
  snprintf( txt, sizeof(txt)-1, "%.2f", meas.gain_max );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_maxgain_entry")), txt );

  /* Display frequency */
  snprintf( txt, sizeof(txt)-1, "%.3f", (double)calc_data.freq_mhz );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_fmhz_entry")), txt );

  // Prevent UI overflows that cause beeps:
  vswr = meas.vswr;
  if( vswr > 999.0 )
    vswr = 999.0;

  zreal = meas.zreal;
  if (zreal > 1e6)
	  zreal = 999999;

  zimag = meas.zimag;
  if (zimag > 1e6)
	  zimag = 999999;

  /* Display VSWR */
  snprintf( txt, sizeof(txt)-1, "%.2f", vswr );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_vswr_entry")), txt );

  /* Display Z real */
  snprintf( txt, sizeof(txt)-1, "%.1f", zreal);
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_zreal_entry")), txt );

  /* Display Z imaginary */
  snprintf( txt, sizeof(txt)-1, "%.1f", zimag);
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_zimag_entry")), txt );

  /* Display antenna temperature values */
  if (meas.ant_temp_tot >= 0.0)
    snprintf( txt, sizeof(txt)-1, "%.0f K", meas.ant_temp_tot );
  else
    snprintf( txt, sizeof(txt)-1, "— K" );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_ant_temp_tot_entry")), txt );

  if (meas.ant_temp >= 0.0)
    snprintf( txt, sizeof(txt)-1, "%.0f K", meas.ant_temp );
  else
    snprintf( txt, sizeof(txt)-1, "— K" );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_ant_temp_entry")), txt );

  if (meas.gt > -999.0)
    snprintf( txt, sizeof(txt)-1, "%.1f", meas.gt );
  else
    snprintf( txt, sizeof(txt)-1, "— dB" );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_gt_entry")), txt );

} /* Display_Frequency_Data() */

/*-----------------------------------------------------------------------*/

/* Emit one dual-axis XY panel through the shared plot renderer, advancing
 * the panel position.  Centralizes the Plot_Graph invocation common to every
 * XY plot type so individual renderers carry only their data preparation.
 * The orchestrator guarantees num_fsteps > 0 before any renderer runs. */
  void
fp_plot_panel(fp_plot_ctx_t *ctx, double *left, double *right, char *titles[3])
{
  Plot_Graph(ctx->fp, left, right, ctx->fplot, ctx->num_fsteps, titles,
      ctx->posn++);
}

/*-----------------------------------------------------------------------*/

/* Walk the dispatch table in panel order, running each enabled plot type's
 * accumulating renderer.  Returns FALSE when a renderer aborts the frame. */
  static gboolean
fp_run_dispatch(fp_plot_ctx_t *ctx)
{
  size_t i;

  for( i = 0; i < G_N_ELEMENTS(fp_plot_dispatch); i++ )
  {
    if( !fp_plot_dispatch[i].enabled() )
      continue;

    if( !fp_plot_dispatch[i].render(ctx) )
      return FALSE;
  }

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/* Plot_Frequency_Data()
 *
 * Plots a graph of frequency-dependent parameters
 * (gain, vswr, imoedance etc) against frequency
 */
  void
_Plot_Frequency_Data( cairo_t *cr )
{
  /* Abort plotting if main window is to be closed
   * or when plots drawing area not available */
  if( isFlagClear(PLOT_ENABLED) ||
      isFlagClear(ENABLE_EXCITN) )
    return;

  int idx, num_fsteps; /* Loop index and valid freq-step count */

  /* Per-frame render handle for the depth-buffered segment pipeline */
  fp_render_t fp;

  /* Shared per-frame data handed to the dispatched plot renderers */
  fp_plot_ctx_t ctx;

  fr_plots_init();

  if (fr_plots == NULL)
	  return; // nothing to do here...

  /* Clear drawingarea */
  cairo_set_source_rgb( cr, (double)COLOR_BLACK.r,
      (double)COLOR_BLACK.g, (double)COLOR_BLACK.b );
  cairo_rectangle(
      cr, 0.0, 0.0,
      (double)freqplots_width,
      (double)freqplots_height );
  cairo_fill( cr );

  /* Begin a new depth-buffered frame; plot types deposit segments and
   * deferred text, flushed together after all panels are drawn. */
  fp_render_reset( &fp, cr );

  /* Build compact index list; out-of-order arrivals appear immediately */
  static int    *valid_steps_map = NULL;
  static double *fplot       = NULL;
  size_t vstep_mreq = (size_t)calc_data.steps_total * sizeof(int);
  size_t fplot_mreq = (size_t)calc_data.steps_total * sizeof(double);
  mem_realloc((void **)&valid_steps_map, vstep_mreq);
  mem_realloc((void **)&fplot, fplot_mreq);

  num_fsteps = 0;
  for( idx = 0; idx < calc_data.steps_total; idx++ )
  {
    if( !save.fstep[idx] )
      continue;
    valid_steps_map[num_fsteps] = idx;
    fplot[num_fsteps]       = save.freq[idx];
    num_fsteps++;
  }

  /* Abort if plotting is not possible FIXME */
  if( (num_fsteps <= 0) || isFlagClear(FREQ_LOOP_READY) ||
      (isFlagClear(FREQ_LOOP_RUNNING) && isFlagClear(FREQ_LOOP_DONE)) ||

      (isFlagClear(PLOT_GMAX)         &&
       isFlagClear(PLOT_GVIEWER)      &&
       isFlagClear(PLOT_VSWR)         &&
       isFlagClear(PLOT_ZREAL_ZIMAG)  &&
       isFlagClear(PLOT_ZMAG_ZPHASE)  &&
       isFlagClear(PLOT_SMITH)        &&
       isFlagClear(PLOT_ANT_TEMP)) )
  {
    return;
  }

  // Call the underscore version because freq_data_lock is already held.
  // This makes the plot choppy during optimize, so skip that then.
  GdkEvent *pending_click = freqplots_pending_click();
  if (pending_click != NULL)
	  Set_Frequency_On_Click(pending_click);

  /* Compute every per-frequency measurement once per frame; meas_calc is
   * costly (antenna-temperature spherical integration) and was previously
   * repeated by each plot type.  All plot types now read these rows. */
  static measurement_t *meas_rows = NULL;
  mem_realloc((void **)&meas_rows,
      (size_t)num_fsteps * sizeof(measurement_t));
  for( idx = 0; idx < num_fsteps; idx++ )
    meas_calc( &meas_rows[idx], valid_steps_map[idx] );

  /* Resolve every enabled plot panel through the dispatch table; each
   * renderer deposits segments and defers its text.  posn advances across
   * renderers so panels lay out top to bottom. */
  ctx.fp              = &fp;
  ctx.valid_steps_map = valid_steps_map;
  ctx.fplot           = fplot;
  ctx.meas_rows       = meas_rows;
  ctx.num_fsteps      = num_fsteps;
  ctx.posn            = 0;

  if( !fp_run_dispatch( &ctx ) )
    return;

  /* Flush all deposited segments depth-sorted, then paint deferred text */
  fp_render_flush( &fp );

  /* Display freq data in entry widgets */
  Display_Frequency_Data();


} /* Plot_Frequency_Data() */

void Plot_Frequency_Data( cairo_t *cr )
{
	if (isFlagSet(ERROR_CONDX))
		return;
	g_rec_mutex_lock(&freq_data_lock);
	_Plot_Frequency_Data( cr );
	g_rec_mutex_unlock(&freq_data_lock);
}

/*-----------------------------------------------------------------------*/
