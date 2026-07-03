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
#include "freqplots_locus.h"
#include "../shared.h"
#include "../opt_ui.h"

#include <string.h>

/* The primary frequency-plots window is the sole persistent view; popups are
 * heap views registered one per graph type and cleared to NULL when closed. */
static freqplots_view_t  fpv_main;
static freqplots_view_t *fpv_popups[FP_PANEL_COUNT];

/* Window-title text per popup graph type, indexed by fp_panel_t. */
static const char *fp_panel_names[FP_PANEL_COUNT] = {
  [FP_PANEL_GAIN]     = N_("Gain"),
  [FP_PANEL_GAIN_DIR] = N_("Gain Direction"),
  [FP_PANEL_VIEWER]   = N_("Viewer Gain"),
  [FP_PANEL_VSWR]     = N_("VSWR"),
  [FP_PANEL_ZRLZIM]   = N_("Impedance (real/imag)"),
  [FP_PANEL_ZMGZPH]   = N_("Impedance (mag/phase)"),
  [FP_PANEL_SMITH]    = N_("Smith Chart"),
  [FP_PANEL_ANT_TEMP] = N_("Antenna Temperature"),
};

/* Per-panel selection and data-availability descriptor.  select_flag is the
 * main-window PLOT_* toggle that chooses the panel; require_flag is a data
 * precondition that must hold for the panel to carry meaningful values, or 0
 * when the panel has none. */
typedef struct {
  unsigned long long select_flag;
  unsigned long long require_flag;
} fp_panel_desc_t;

static const fp_panel_desc_t fp_panel_desc[FP_PANEL_COUNT] = {
  [FP_PANEL_GAIN]     = { PLOT_GMAX,        ENABLE_RDPAT },
  [FP_PANEL_GAIN_DIR] = { PLOT_GAIN_DIR,    ENABLE_RDPAT },
  [FP_PANEL_VIEWER]   = { PLOT_GVIEWER,     ENABLE_RDPAT },
  [FP_PANEL_VSWR]     = { PLOT_VSWR,        0 },
  [FP_PANEL_ZRLZIM]   = { PLOT_ZREAL_ZIMAG, 0 },
  [FP_PANEL_ZMGZPH]   = { PLOT_ZMAG_ZPHASE, 0 },
  [FP_PANEL_SMITH]    = { PLOT_SMITH,       0 },
  [FP_PANEL_ANT_TEMP] = { PLOT_ANT_TEMP,    ENABLE_RDPAT },
};

/* Setting that gates one selected-frequency readout column.  Each value names
 * the same condition the matching graph renderer evaluates, so the popup bar
 * shows exactly the traces the graph draws. */
typedef enum {
  FP_FIELD_ALWAYS,
  FP_FIELD_IF_NETGAIN,
  FP_FIELD_IF_NO_NETGAIN,
  FP_FIELD_IF_S11,
  FP_FIELD_IF_ANT_TEMP_VIEW,
  FP_FIELD_IF_NO_ANT_TEMP_VIEW,
} fp_field_cond_t;

/* One readout column: a measurement field, its display gate, its numeric
 * format, and a trailing unit.  A value below invalid_min renders as an em
 * dash so sentinel results (-1, -999) read as unavailable, not as numbers. */
typedef struct {
  int             meas_idx;
  fp_field_cond_t cond;
  const char     *fmt;
  const char     *unit;
  double          invalid_min;
} fp_readout_field_t;

/* Leading column shared by every popup bar: the selected frequency. */
static const fp_readout_field_t fp_readout_freq =
  { MEAS_MHZ, FP_FIELD_ALWAYS, "%.3f", "", -1e30 };

static const fp_readout_field_t fp_rf_gain[] = {
  { MEAS_GAIN_MAX, FP_FIELD_ALWAYS,        "%.2f", " dBi", -1e30 },
  { MEAS_GAIN_NET, FP_FIELD_IF_NETGAIN,    "%.2f", " dBi", -1e30 },
  { MEAS_FB_RATIO, FP_FIELD_IF_NO_NETGAIN, "%.2f", " dB",  0.0   },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_gain_dir[] = {
  { MEAS_GAIN_THETA, FP_FIELD_ALWAYS, "%.1f", "°", -1e30 },
  { MEAS_GAIN_PHI,   FP_FIELD_ALWAYS, "%.1f", "°", -1e30 },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_viewer[] = {
  { MEAS_GAIN_VIEWER,     FP_FIELD_ALWAYS,     "%.2f", " dBi", -1e30 },
  { MEAS_GAIN_VIEWER_NET, FP_FIELD_IF_NETGAIN, "%.2f", " dBi", -1e30 },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_vswr[] = {
  { MEAS_VSWR, FP_FIELD_ALWAYS, "%.2f", "",    -1e30 },
  { MEAS_S11,  FP_FIELD_IF_S11, "%.2f", " dB", -1e30 },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_zrlzim[] = {
  { MEAS_ZREAL, FP_FIELD_ALWAYS, "%.1f", " Ω", -1e30 },
  { MEAS_ZIMAG, FP_FIELD_ALWAYS, "%.1f", " Ω", -1e30 },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_zmgzph[] = {
  { MEAS_ZMAG,   FP_FIELD_ALWAYS, "%.1f", " Ω", -1e30 },
  { MEAS_ZPHASE, FP_FIELD_ALWAYS, "%.1f", "°",  -1e30 },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_smith[] = {
  { MEAS_ZREAL, FP_FIELD_ALWAYS, "%.1f", " Ω", -1e30 },
  { MEAS_ZIMAG, FP_FIELD_ALWAYS, "%.1f", " Ω", -1e30 },
  { MEAS_VSWR,  FP_FIELD_ALWAYS, "%.2f", "",    -1e30 },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

static const fp_readout_field_t fp_rf_ant_temp[] = {
  { MEAS_GT,           FP_FIELD_ALWAYS,               "%.1f", " dB", -998.0 },
  { MEAS_ANT_TEMP,     FP_FIELD_IF_ANT_TEMP_VIEW,     "%.0f", " K",  0.0    },
  { MEAS_ANT_TEMP_TOT, FP_FIELD_IF_NO_ANT_TEMP_VIEW,  "%.0f", " K",  0.0    },
  { -1, FP_FIELD_ALWAYS, NULL, NULL, 0.0 },
};

/* Per-graph readout field set, indexed by fp_panel_t.  This is the one place
 * naming which measurement values each popup graph displays. */
static const fp_readout_field_t *const fp_panel_readout[FP_PANEL_COUNT] = {
  [FP_PANEL_GAIN]     = fp_rf_gain,
  [FP_PANEL_GAIN_DIR] = fp_rf_gain_dir,
  [FP_PANEL_VIEWER]   = fp_rf_viewer,
  [FP_PANEL_VSWR]     = fp_rf_vswr,
  [FP_PANEL_ZRLZIM]   = fp_rf_zrlzim,
  [FP_PANEL_ZMGZPH]   = fp_rf_zmgzph,
  [FP_PANEL_SMITH]    = fp_rf_smith,
  [FP_PANEL_ANT_TEMP] = fp_rf_ant_temp,
};

/* True when the setting gating @cond currently selects its column.  Each arm
 * mirrors the condition its graph renderer reads so bar and graph agree. */
  static gboolean
fp_field_cond_active(fp_field_cond_t cond)
{
  switch( cond )
  {
    case FP_FIELD_ALWAYS:              return TRUE;
    case FP_FIELD_IF_NETGAIN:          return isFlagSet(PLOT_NETGAIN);
    case FP_FIELD_IF_NO_NETGAIN:       return isFlagClear(PLOT_NETGAIN);
    case FP_FIELD_IF_S11:              return rc_config.freqplots_s11 != 0;
    case FP_FIELD_IF_ANT_TEMP_VIEW:    return rc_config.freqplots_show_ant_temp != 0;
    case FP_FIELD_IF_NO_ANT_TEMP_VIEW: return rc_config.freqplots_show_ant_temp == 0;
  }

  BUG("fp_field_cond_active: unhandled cond %d\n", cond);
  return FALSE;
}

/* Number of readout columns a popup of @filter shows: a leading frequency
 * column plus its per-graph field set, clamped to the widget array bound. */
  static int
fp_readout_field_count(fp_panel_t filter)
{
  const fp_readout_field_t *f = fp_panel_readout[filter];
  int n = 1;

  while( f != NULL && f->meas_idx >= 0 ) { n++; f++; }

  if( n > FP_READOUT_MAX )
  {
    BUG("fp_readout_field_count: field set exceeds FP_READOUT_MAX\n");
    n = FP_READOUT_MAX;
  }

  return n;
}

/* Resolve combined readout column @i for a popup of @filter: column 0 is the
 * frequency, the rest follow the per-graph field set. */
  static const fp_readout_field_t *
fp_readout_field_at(fp_panel_t filter, int i)
{
  if( i == 0 )
    return &fp_readout_freq;

  return &fp_panel_readout[filter][i - 1];
}

/* True when @panel should emit for @v: its data precondition holds and either
 * the view pins this single graph (popup) or the main-window toggle selects it
 * (primary).  A popup thus renders its graph even when the matching toggle is
 * off in the main window, while data preconditions still gate both views. */
  static gboolean
fp_panel_active(freqplots_view_t *v, fp_panel_t panel)
{
  const fp_panel_desc_t *d = &fp_panel_desc[panel];

  if( d->require_flag && isFlagClear(d->require_flag) )
    return FALSE;

  if( v->filter != FP_PANEL_ALL )
    return v->filter == panel;

  return isFlagSet(d->select_flag) ? TRUE : FALSE;
}

/* True when @v has at least one panel its user wants drawn: any panel for a
 * popup, or any selected toggle for the primary all-panels view. */
  static gboolean
fp_view_any_selected(freqplots_view_t *v)
{
  int p;

  if( v->filter != FP_PANEL_ALL )
    return TRUE;

  for( p = 0; p < FP_PANEL_COUNT; p++ )
    if( isFlagSet(fp_panel_desc[p].select_flag) )
      return TRUE;

  return FALSE;
}

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

void fr_plots_init(freqplots_view_t *v)
{
  int idx;

  /* 2d array of plot rectangles popluated by the Plot_Graph function */
  if (v->ngraph > 0 && calc_data.FR_cards > 0)
	  mem_array_realloc(&v->fr_plots, v->ngraph * calc_data.FR_cards);
  else
  {
	  mem_array_free(&v->fr_plots);
	  return;
  }

  for (idx = 0; idx < v->ngraph * calc_data.FR_cards; idx++)
  {
	  if (FR_PLOT_T_IS_VALID(&v->fr_plots[idx]))
		  continue;

	  // Set the plot position
	  v->fr_plots[idx].posn = idx / calc_data.FR_cards;

	  // Point to the freq loop data
	  v->fr_plots[idx].fr = idx % calc_data.FR_cards;
	  v->fr_plots[idx].freq_loop_data = &calc_data.freq_loop_data[v->fr_plots[idx].fr];

	  // zero the plot_rect, Plot_Graph() will fill it in.
	  memset(&v->fr_plots[idx].plot_rect, 0, sizeof(GdkRectangle));

	  // Set it as valid:
	  v->fr_plots[idx].valid = FR_PLOT_T_MAGIC;
  }
}

fr_plot_t *get_fr_plot(freqplots_view_t *v, int posn, int fr)
{
	if (posn < 0 || posn >= v->ngraph || fr < 0 || fr >= calc_data.FR_cards)
		return NULL;

	return &v->fr_plots[posn*calc_data.FR_cards + fr];
}

GdkRectangle *get_plot_rect(freqplots_view_t *v, int posn, int fr)
{
	fr_plot_t *p = get_fr_plot(v, posn, fr);

	if (p == NULL)
		return NULL;

	return &p->plot_rect;
}

/* Hit-test: return the valid plot row whose rectangle contains (px,py), or
 * NULL when the pixel falls in no panel.  Shared by the click-to-frequency
 * and double-click-to-popout input paths. */
fr_plot_t *fr_plot_at(freqplots_view_t *v, double px, double py)
{
	int i, n;

	if (v->fr_plots == NULL)
		return NULL;

	n = v->ngraph * calc_data.FR_cards;
	for (i = 0; i < n; i++)
	{
		fr_plot_t *p = &v->fr_plots[i];

		if (!FR_PLOT_T_IS_VALID(p))
			continue;

		if (   px >= p->plot_rect.x
			&& px <= p->plot_rect.x + p->plot_rect.width
			&& py >= p->plot_rect.y
			&& py <= p->plot_rect.y + p->plot_rect.height)
			return p;
	}

	return NULL;
}

void fr_plot_sync_widths(freqplots_view_t *v, fr_plot_t *fr_plot)
{
	GdkRectangle *current, *r;
	int posn, fr;

	if (fr_plot == NULL || v->fr_plots == NULL)
		return;

	// Update all plot widths so they re the same as fr_plot's width.
	for (posn = 0; posn < v->ngraph; posn++)
	{
		// Skip the current one the mouse adjusted:
		if (posn == fr_plot->posn)
			continue;

		for (fr = 0; fr < calc_data.FR_cards; fr++)
		{
			current = get_plot_rect(v, fr_plot->posn, fr);
			if (current == NULL)
			{
				BUG("fr_plot_sync_widths, current == NULL: calc_data.FR_cards=%d v->ngraph=%d fr=%d posn=%d valid=%d\n",
					calc_data.FR_cards, v->ngraph,
					fr, fr_plot->posn,
					FR_PLOT_T_IS_VALID(fr_plot));
				return;
			}
			r = get_plot_rect(v, posn, fr);

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
  fr_plot_t *fr_plots = freqplots_main_view()->fr_plots;

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

/*
 * freqplots_update_fscale_extents - populate main-view panel display extents
 *
 * Stores each valid panel's display extent into fr_plot->min_fscale/max_fscale
 * via the single extent derivation.  Geometry-free: passes nval = 0 so
 * Fit_to_Scale applies only the degenerate single-point expansion, yielding a
 * baseline independent of rendered plot width; a later paint refines each
 * extent with real geometry.  Runs on the GTK thread before a sweep starts so
 * green-line classification reads a populated extent while the sweep worker
 * only reads these fields.
 */
void
freqplots_update_fscale_extents( void )
{
  freqplots_view_t *v = freqplots_main_view();
  int i, n;

  if (v->fr_plots == NULL || v->ngraph < 1 || calc_data.FR_cards < 1)
    return;

  n = v->ngraph * calc_data.FR_cards;
  for (i = 0; i < n; i++)
  {
    int nval = 0;

    if (!FR_PLOT_T_IS_VALID(&v->fr_plots[i]))
      continue;

    fscale_extent_fit( v->fr_plots[i].freq_loop_data,
        rc_config.freqplots_round_x_axis,
        &v->fr_plots[i].min_fscale, &v->fr_plots[i].max_fscale, &nval );
  }
}

/*-----------------------------------------------------------------------*/

/* Step index the green frequency line marks: calc_data.freq_step, clamped to
 * the extra custom-frequency slot at steps_total (xnec2.c user_set_frequency).
 * Returns -1 when no step is selected, so callers skip their readout. */
  static int
fp_selected_fstep( void )
{
  int fstep = calc_data.freq_step;

  if( fstep < 0 )
    return -1;

  if( fstep >= calc_data.steps_total )
    fstep = calc_data.steps_total;

  return fstep;
}

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

  fstep = fp_selected_fstep();

  if( fstep < 0 )
    return;

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

  /* Feedpoint scalars are undefined without a driving segment; render an em
   * dash for incident-field and elementary-current-source excitations. */
  gboolean fed = fpat_has_feedpoint();

  /* Display VSWR */
  if( fed )
    snprintf( txt, sizeof(txt)-1, "%.2f", vswr );
  else
    snprintf( txt, sizeof(txt)-1, "—" );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_vswr_entry")), txt );

  /* Display Z real */
  if( fed )
    snprintf( txt, sizeof(txt)-1, "%.1f", zreal);
  else
    snprintf( txt, sizeof(txt)-1, "—" );
  gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
          freqplots_window_builder, "freqplots_zreal_entry")), txt );

  /* Display Z imaginary */
  if( fed )
    snprintf( txt, sizeof(txt)-1, "%.1f", zimag);
  else
    snprintf( txt, sizeof(txt)-1, "—" );
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

/* Format readout column @f from measurement @m into @out.  A value below the
 * field's invalid_min renders as an em dash with the unit so sentinel results
 * read as unavailable. */
  static void
fp_readout_format(const fp_readout_field_t *f, measurement_t *m,
    char *out, int outlen)
{
  double val = m->a[f->meas_idx];

  if( val < f->invalid_min )
  {
    snprintf( out, outlen, "—%s", f->unit );
    return;
  }

  char num[32];
  snprintf( num, sizeof(num), f->fmt, val );
  snprintf( out, outlen, "%s%s", num, f->unit );
}

/* freqplots_readout_bar_new()
 *
 * Build a popup's selected-frequency readout bar: one cell per displayed
 * column, each a bold name label from meas_display_names paired with a value
 * label refreshed by freqplots_update_readout.  Stores cell and value widgets
 * in the view so the per-draw update can set text and toggle visibility.
 */
  GtkWidget *
freqplots_readout_bar_new(freqplots_view_t *v)
{
  GtkWidget *bar = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 12 );
  int i, n;

  gtk_container_set_border_width( GTK_CONTAINER(bar), 4 );

  n = fp_readout_field_count( v->filter );

  for( i = 0; i < n; i++ )
  {
    const fp_readout_field_t *f = fp_readout_field_at( v->filter, i );
    GtkWidget *cell = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 4 );
    GtkWidget *name = gtk_label_new( NULL );
    GtkWidget *val  = gtk_label_new( "—" );
    char markup[96];

    snprintf( markup, sizeof(markup), "<b>%s:</b>",
        _(meas_display_names[f->meas_idx]) );
    gtk_label_set_markup( GTK_LABEL(name), markup );

    gtk_box_pack_start( GTK_BOX(cell), name, FALSE, FALSE, 0 );
    gtk_box_pack_start( GTK_BOX(cell), val,  FALSE, FALSE, 0 );
    gtk_box_pack_start( GTK_BOX(bar),  cell, FALSE, FALSE, 0 );

    v->readout_field[i] = cell;
    v->readout_value[i] = val;
  }

  v->readout_n = n;

  return bar;
}

/* freqplots_update_readout()
 *
 * Refresh a popup's readout bar from the selected-frequency measurement.
 * Each column is shown only when its gating setting is active, so toggling a
 * graph option (net gain, S11, antenna-temperature view) adds or drops the
 * matching value in step with the graph.  The selected step is calc_data.
 * freq_step, identical to the value the green frequency line marks.
 */
  static void
freqplots_update_readout(freqplots_view_t *v)
{
  measurement_t meas;
  int fstep, i;

  if( v->readout_n <= 0 )
    return;

  fstep = fp_selected_fstep();

  if( fstep < 0 )
    return;

  meas_calc( &meas, fstep );

  for( i = 0; i < v->readout_n; i++ )
  {
    const fp_readout_field_t *f = fp_readout_field_at( v->filter, i );
    gboolean active = fp_field_cond_active( f->cond );
    char txt[48];

    gtk_widget_set_visible( v->readout_field[i], active );

    if( !active )
      continue;

    fp_readout_format( f, &meas, txt, sizeof(txt) );
    gtk_label_set_text( GTK_LABEL(v->readout_value[i]), txt );
  }
}

/*-----------------------------------------------------------------------*/

/* Emit one dual-axis XY panel through the shared plot renderer, advancing
 * the panel position.  Centralizes the Plot_Graph invocation common to every
 * XY plot type so individual renderers carry only their data preparation.
 * The orchestrator guarantees num_fsteps > 0 before any renderer runs. */
  void
fp_plot_panel(fp_plot_ctx_t *ctx, double *left, double *right, char *titles[3],
    fp_panel_t panel)
{
  if( !fp_panel_active(ctx->view, panel) )
    return;

  Plot_Graph(ctx->view, ctx->fp, left, right, ctx->fplot, ctx->num_fsteps,
      ctx->card_nfsteps, titles, ctx->posn++, panel);
}

/* Emit the Smith-chart panel through the shared renderer under the same view
 * filter gate as fp_plot_panel; posn advances only on emission. */
  void
fp_plot_smith_panel(fp_plot_ctx_t *ctx, double *fa, double *fb, double *fc,
    int nc, fp_panel_t panel)
{
  if( !fp_panel_active(ctx->view, panel) )
    return;

  Plot_Graph_Smith(ctx->view, ctx->fp, fa, fb, fc, nc,
      ctx->card_nfsteps, ctx->posn++);
}

/*-----------------------------------------------------------------------*/

/* Walk the dispatch table in panel order, running each enabled plot type's
 * accumulating renderer.  Returns FALSE when a renderer aborts the frame. */
  static gboolean
fp_run_dispatch(fp_plot_ctx_t *ctx)
{
  size_t i;

  /* The primary all-panels view skips groups whose enabled() predicate is
   * false for efficiency.  A popup runs every group and lets the per-panel
   * filter emit only its one graph, so a graph unselected in the main window
   * still draws in its popup. */
  gboolean gate_by_enabled = (ctx->view->filter == FP_PANEL_ALL);

  for( i = 0; i < G_N_ELEMENTS(fp_plot_dispatch); i++ )
  {
    if( gate_by_enabled && !fp_plot_dispatch[i].enabled() )
      continue;

    if( !fp_plot_dispatch[i].render(ctx) )
      return FALSE;
  }

  return TRUE;
}

/*-----------------------------------------------------------------------*/

/* freqloop_card_of_fmhz()
 *
 * Resolve the FR card that owns a frequency: the card whose data range contains
 * it, or failing that the card whose display extent contains it.  Single
 * authority for that judgment; green_line_eval marks the green line
 * GREEN_LINE_EXTRA exactly when this returns a card.  The display-extent
 * fallback reads the main view, the single authority for panel extents (see
 * fmhz_within_display_range), so every view resolves a frequency to the same
 * card regardless of whether its own panels have painted.
 * Returns -1 when no card covers the frequency.
 */
int
freqloop_card_of_fmhz( double fmhz )
{
  fr_plot_t *fr_plots = freqplots_main_view()->fr_plots;
  int fr;

  for( fr = 0; fr < calc_data.FR_cards; fr++ )
  {
    freq_loop_data_t *fld = &calc_data.freq_loop_data[fr];
    if( fmhz >= fld->min_freq - FREQ_EPSILON_MHZ &&
        fmhz <= fld->max_freq + FREQ_EPSILON_MHZ )
      return fr;
  }

  if( fr_plots == NULL )
    return -1;

  for( fr = 0; fr < calc_data.FR_cards; fr++ )
  {
    fr_plot_t *p = &fr_plots[fr];
    if( FR_PLOT_T_IS_VALID(p) &&
        fmhz >= p->min_fscale - FREQ_EPSILON_MHZ &&
        fmhz <= p->max_fscale + FREQ_EPSILON_MHZ )
      return fr;
  }

  return -1;
}

/* Frequency-plot scratch arrays built per redraw in _Plot_Frequency_Data():
 * the compact valid-step index, its plotted values, and per-card step counts.
 * Capacity holds every sweep step plus the green-line extra slot. */
static int    *valid_steps_map = NULL;
static double *fplot           = NULL;
static int    *card_nfsteps    = NULL;

/* freqplots_cleanup()
 *
 * Releases the frequency-plot scratch arrays and every graph type's trace
 * buffers at program exit.
 */
  void
freqplots_cleanup( void )
{
  mem_array_free( &valid_steps_map );
  mem_array_free( &fplot );
  mem_array_free( &card_nfsteps );

  fp_gain_free();
  fp_viewer_free();
  fp_vswr_free();
  fp_impedance_free();
  fp_ant_temp_free();

} /* freqplots_cleanup() */

/* Plot_Frequency_Data()
 *
 * Plots a graph of frequency-dependent parameters
 * (gain, vswr, imoedance etc) against frequency
 */
  void
_Plot_Frequency_Data( freqplots_view_t *v, cairo_t *cr )
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

  /* Primary shows every panel; a popup pins ngraph to its single graph. */
  v->ngraph = (v->filter == FP_PANEL_ALL) ? calc_data.ngraph : 1;

  fr_plots_init(v);

  if (v->fr_plots == NULL)
	  return; // nothing to do here...

  /* Clear drawingarea to the active theme's background surface; foreground
   * roles are contrast-solved against this same surface. */
  const theme_t *th = theme_active();
  cairo_set_source_rgb( cr, (double)th->colors[THEME_ROLE_BACKGROUND].r,
      (double)th->colors[THEME_ROLE_BACKGROUND].g,
      (double)th->colors[THEME_ROLE_BACKGROUND].b );
  cairo_rectangle(
      cr, 0.0, 0.0,
      (double)v->width,
      (double)v->height );
  cairo_fill( cr );

  /* Begin a new depth-buffered frame; plot types deposit segments and
   * deferred text, flushed together after all panels are drawn. */
  /* Bind this view's own base font; created once, freed at view teardown. */
  if( v->text_layout == NULL )
    v->text_layout = gtk_widget_create_pango_layout(v->drawingarea, NULL);
  fp_render_reset( &fp, cr, v->text_layout );

  /* Build compact index list; out-of-order arrivals appear immediately.
   * Capacity holds every sweep step plus the green-line extra slot. */
  mem_array_realloc(&valid_steps_map, (calc_data.steps_total + 1));
  mem_array_realloc(&fplot, (calc_data.steps_total + 1));
  mem_array_realloc(&card_nfsteps, calc_data.FR_cards);

  /* The extra slot at steps_total holds a computed green-line measurement when
   * save.fstep marks it valid; render it inside its home card at the sorted
   * frequency position so the trace stays monotonic in frequency. */
  int extra_idx  = calc_data.steps_total;
  int extra_card = save.fstep[extra_idx]
      ? freqloop_card_of_fmhz( save.freq[extra_idx] ) : -1;

  /* Walk each FR card's contiguous step block in turn, recording the actual
   * valid-point count per card so the renderer partitions the flat arrays by
   * real counts rather than the static sweep stride. */
  num_fsteps = 0;
  int card_start = 0;
  for( int fr = 0; fr < calc_data.FR_cards; fr++ )
  {
    int block_start = num_fsteps;
    int card_steps  = calc_data.freq_loop_data[fr].freq_steps;

    for( idx = card_start;
         idx < card_start + card_steps && idx < calc_data.steps_total; idx++ )
    {
      if( !save.fstep[idx] )
        continue;
      valid_steps_map[num_fsteps] = idx;
      fplot[num_fsteps]           = save.freq[idx];
      num_fsteps++;
    }

    if( fr == extra_card )
    {
      int pos = block_start;
      while( pos < num_fsteps && fplot[pos] < save.freq[extra_idx] )
        pos++;
      memmove( &valid_steps_map[pos+1], &valid_steps_map[pos],
          (size_t)(num_fsteps - pos) * sizeof(int) );
      memmove( &fplot[pos+1], &fplot[pos],
          (size_t)(num_fsteps - pos) * sizeof(double) );
      valid_steps_map[pos] = extra_idx;
      fplot[pos]           = save.freq[extra_idx];
      num_fsteps++;
    }

    card_nfsteps[fr] = num_fsteps - block_start;
    card_start      += card_steps;
  }

  /* Abort if plotting is not possible FIXME */
  if( (num_fsteps <= 0) || isFlagClear(FREQ_LOOP_READY) ||
      (isFlagClear(FREQ_LOOP_RUNNING) && isFlagClear(FREQ_LOOP_DONE)) ||
      !fp_view_any_selected(v) )
  {
    return;
  }

  // Call the underscore version because freq_data_lock is already held.
  // This makes the plot choppy during optimize, so skip that then.
  GdkEvent *prev_click_event = freqplots_pending_click(v);
  if (prev_click_event != NULL)
	  Set_Frequency_On_Click(v, prev_click_event);

  /* Compute every per-frequency measurement once per frame; meas_calc is
   * costly (antenna-temperature spherical integration) and was previously
   * repeated by each plot type.  All plot types now read these rows. */
  static measurement_t *meas_rows = NULL;
  mem_array_realloc(&meas_rows, num_fsteps);
  for( idx = 0; idx < num_fsteps; idx++ )
    meas_calc( &meas_rows[idx], valid_steps_map[idx] );

  /* Resolve every enabled plot panel through the dispatch table; each
   * renderer deposits segments and defers its text.  posn advances across
   * renderers so panels lay out top to bottom. */
  ctx.view            = v;
  ctx.fp              = &fp;
  ctx.valid_steps_map = valid_steps_map;
  ctx.fplot           = fplot;
  ctx.card_nfsteps    = card_nfsteps;
  ctx.meas_rows       = meas_rows;
  ctx.num_fsteps      = num_fsteps;
  ctx.posn            = 0;

  /* Reset the click-resolution registry before producers deposit their
   * rendered loci for this frame. */
  fp_locus_frame_begin( v );

  if( !fp_run_dispatch( &ctx ) )
    return;

  /* Flush all deposited segments depth-sorted, then paint deferred text */
  fp_render_flush( &fp );

  /* Refresh the selected-frequency readouts: the primary window's builder
   * entry grid, or a popup's per-graph label bar. */
  if( v->filter == FP_PANEL_ALL )
    Display_Frequency_Data();
  else
    freqplots_update_readout( v );


} /* Plot_Frequency_Data() */

void Plot_Frequency_Data( freqplots_view_t *view, cairo_t *cr )
{
	if (isFlagSet(ERROR_CONDX))
		return;
	g_rec_mutex_lock(&freq_data_lock);
	_Plot_Frequency_Data( view, cr );
	g_rec_mutex_unlock(&freq_data_lock);
}

/*-----------------------------------------------------------------------*/

freqplots_view_t *freqplots_main_view(void)
{
	return &fpv_main;
}

/* True when a detached popup for graph type @panel is currently open.  Lets
 * rotation observers refresh a popped-out graph even when the main window's
 * corresponding PLOT_* flag is clear. */
gboolean freqplots_popup_open(fp_panel_t panel)
{
	if (panel < 0 || panel >= FP_PANEL_COUNT)
		return FALSE;

	return fpv_popups[panel] != NULL;
}

/* Queue a redraw for the primary window and every open popup so one data or
 * parameter mutation refreshes all frequency-plot views.  Routed through
 * xnec2_widget_queue_draw to keep the main-thread g_idle guarantee. */
void freqplots_redraw_all(gboolean force)
{
	int p;

	if (fpv_main.drawingarea != NULL)
		xnec2_widget_queue_draw(fpv_main.drawingarea, force);

	for (p = 0; p < FP_PANEL_COUNT; p++)
		if (fpv_popups[p] != NULL && fpv_popups[p]->drawingarea != NULL)
			xnec2_widget_queue_draw(fpv_popups[p]->drawingarea, force);
}

/* Open the popup for graph type @panel, or raise it if already open.  A new
 * popup gets a heap view filtered to the one graph with ngraph fixed at 1. */
void freqplots_open_panel(fp_panel_t panel)
{
	freqplots_view_t *v = NULL;

	if (panel < 0 || panel >= FP_PANEL_COUNT)
		return;

	if (fpv_popups[panel] != NULL)
	{
		gtk_window_present(GTK_WINDOW(fpv_popups[panel]->window));
		return;
	}

	mem_new(&v);

	v->filter = panel;
	v->ngraph = 1;

	create_freqplots_popup_window(v, _(fp_panel_names[panel]));
	fpv_popups[panel] = v;
	gtk_widget_show_all(v->window);
}

/* Close the popup for graph type @panel: drop it from the registry and free
 * its view and plot table.  The window is destroyed by the caller's destroy
 * path, so this only releases heap state. */
void freqplots_close_panel(fp_panel_t panel)
{
	freqplots_view_t *v;

	if (panel < 0 || panel >= FP_PANEL_COUNT)
		return;

	v = fpv_popups[panel];
	if (v == NULL)
		return;

	fpv_popups[panel] = NULL;
	mem_array_free(&v->fr_plots);
	fp_locus_free(v);
	mem_free(&v->prev_click_event);
	if (v->text_layout != NULL)
		g_object_unref(v->text_layout);
	mem_free(&v);
}

/* Destroy every open popup window; each window's destroy handler frees its
 * view through freqplots_close_panel.  Called during primary-window teardown. */
void freqplots_destroy_all_popups(void)
{
	int p;

	for (p = 0; p < FP_PANEL_COUNT; p++)
		if (fpv_popups[p] != NULL)
			gtk_widget_destroy(fpv_popups[p]->window);
}

/*-----------------------------------------------------------------------*/
