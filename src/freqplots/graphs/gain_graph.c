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
 * gain_graph: Max-gain plot type.
 *
 * Plots raw max gain against frequency, paired with F/B ratio or net gain,
 * and optionally the max-gain direction.  All per-frequency values come from
 * the shared meas_calc() column source.
 */

#include "../freqplots_internal.h"
#include "../../shared.h"

  int
fp_gain_enabled(void)
{
  return rc_config.freqplots_gmax_togglebutton && isFlagSet(ENABLE_RDPAT);
}

/* Max-gain plot trace buffers, reused across fp_gain_render() calls. */
static double *gmax = NULL, *netgain = NULL;
static double *gdir_tht = NULL, *gdir_phi = NULL, *fbratio = NULL;

/* fp_gain_free()
 *
 * Releases the max-gain plot trace buffers.
 */
  void
fp_gain_free( void )
{
  mem_array_free( &gmax );
  mem_array_free( &netgain );
  mem_array_free( &gdir_tht );
  mem_array_free( &gdir_phi );
  mem_array_free( &fbratio );

} /* fp_gain_free() */

  gboolean
fp_gain_render(fp_plot_ctx_t *ctx)
{
  char *titles[3];
  gboolean no_fbr;
  int idx;

  /* Allocate max gmax and directions */
  mem_array_realloc(&gmax, ctx->num_fsteps);
  mem_array_realloc(&gdir_tht, ctx->num_fsteps);
  mem_array_realloc(&gdir_phi, ctx->num_fsteps);
  mem_array_realloc(&fbratio, ctx->num_fsteps);
  if( rc_config.freqplots_net_gain )
    mem_array_realloc(&netgain, ctx->num_fsteps);

  /* Gather max gain, direction, and F/B ratio in one measurement pass */
  fp_meas_column_t cols[5];
  int ncols = 0;
  cols[ncols++] = (fp_meas_column_t){ gmax,     MEAS_GAIN_MAX   };
  cols[ncols++] = (fp_meas_column_t){ gdir_tht, MEAS_GAIN_THETA };
  cols[ncols++] = (fp_meas_column_t){ gdir_phi, MEAS_GAIN_PHI   };
  cols[ncols++] = (fp_meas_column_t){ fbratio,  MEAS_FB_RATIO   };
  if( rc_config.freqplots_net_gain )
    cols[ncols++] = (fp_meas_column_t){ netgain, MEAS_GAIN_NET };
  fp_fill_meas_columns( ctx, cols, ncols );

  /* F/B ratio is unavailable when any step reports a negative value */
  no_fbr = FALSE;
  for( idx = 0; idx < ctx->num_fsteps && !no_fbr; idx++ )
    no_fbr = ( fbratio[idx] < 0 );

  /* Plot gain and f/b ratio (if possible) graph(s) */
  titles[0] = _("Raw Gain dBi");
  if( no_fbr || rc_config.freqplots_net_gain )
  {
    if( rc_config.freqplots_net_gain )
    {
      titles[1] = _("Max Gain & Net Gain vs Frequency");
      titles[2] = _("Net Gain dBi");
      fp_plot_panel(ctx, gmax, netgain, titles, FP_PANEL_GAIN);
    }
    else
    {
      titles[1] = _("Max Gain & F/B Ratio vs Frequency");
      titles[2] = "        ";
      fp_plot_panel(ctx, gmax, NULL, titles, FP_PANEL_GAIN);
    }
  }
  else
  {
    titles[1] = _("Max Gain & F/B Ratio vs Frequency");
    titles[2] = _("F/B Ratio dB");
    fp_plot_panel(ctx, gmax, fbratio, titles, FP_PANEL_GAIN);
  }

  /* Deposit max gain direction unconditionally; the dispatch gate emits it
   * only when selected (primary) or pinned (popup). */
  titles[0] = _("Rad Angle - deg");
  titles[1] = _("Max Gain Direction vs Frequency");
  titles[2] = _("Phi - deg");
  fp_plot_panel(ctx, gdir_tht, gdir_phi, titles, FP_PANEL_GAIN_DIR);

  return TRUE;
}
