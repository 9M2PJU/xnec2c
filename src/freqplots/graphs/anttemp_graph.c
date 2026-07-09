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
 * anttemp_graph: Antenna-noise-temperature plot type.
 *
 * Plots G/Ta against the antenna noise temperature (Ta or TA per the View
 * toggle).  Per-frequency values come from the shared meas_calc() source with
 * sentinel-guarded selection of the right-axis temperature.
 */

#include "../freqplots_internal.h"
#include "../../shared.h"

  int
fp_ant_temp_enabled(void)
{
  return rc_config.freqplots_ant_temp_togglebutton && isFlagSet(ENABLE_RDPAT);
}

/* Antenna-temperature trace buffers, reused across fp_ant_temp_render() calls. */
static double *gt_buf = NULL, *temp_buf = NULL;

/* fp_ant_temp_free()
 *
 * Releases the antenna-temperature trace buffers.
 */
  void
fp_ant_temp_free( void )
{
  mem_array_free( &gt_buf );
  mem_array_free( &temp_buf );

} /* fp_ant_temp_free() */

  gboolean
fp_ant_temp_render(fp_plot_ctx_t *ctx)
{
  char *titles[3];
  int idx;

  mem_array_realloc(&gt_buf, ctx->num_fsteps);
  mem_array_realloc(&temp_buf, ctx->num_fsteps);

  for( idx = 0; idx < ctx->num_fsteps; idx++ )
  {
    measurement_t *meas = &ctx->meas_rows[idx];
    gt_buf[idx] = (meas->gt > -999.0) ? meas->gt : 0.0;

    /* Right axis: Ta when View toggle active, TA otherwise */
    if( rc_config.freqplots_show_ant_temp )
      temp_buf[idx] = (meas->ant_temp >= 0.0) ? meas->ant_temp : 0.0;
    else
      temp_buf[idx] = (meas->ant_temp_tot >= 0.0) ? meas->ant_temp_tot : 0.0;
  }

  titles[0] = _("G/Ta (dB)");
  titles[1] = _("G/Ta & TA - Antenna noise temperature");
  if( rc_config.freqplots_show_ant_temp )
    titles[2] = _("Ta (K)");
  else
    titles[2] = _("TA (K)");

  fp_plot_panel(ctx, gt_buf, temp_buf, titles, FP_PANEL_ANT_TEMP);

  return TRUE;
}
