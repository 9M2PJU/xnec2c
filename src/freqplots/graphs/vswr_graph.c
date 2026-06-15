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
 * vswr_graph: VSWR plot type.
 *
 * Plots VSWR against frequency, optionally paired with the S11 return-loss
 * trace.  Both values come from the shared meas_calc() column source; VSWR is
 * clamped when the rc-config option is active.
 */

#include "../freqplots_internal.h"
#include "../../shared.h"

  int
fp_vswr_enabled(void)
{
  return isFlagSet(PLOT_VSWR);
}

  gboolean
fp_vswr_render(fp_plot_ctx_t *ctx)
{
  static double *vswr = NULL, *s11 = NULL;
  char *titles[3];
  int idx;
  size_t mreq = (size_t)ctx->num_fsteps * sizeof(double);

  /* Plotting frame titles */
  titles[0] = _("VSWR");
  if( rc_config.freqplots_s11 )
  {
    titles[1] = _("VSWR & S11 vs Frequency");
    titles[2] = _("S11 dB");
  }
  else
  {
    titles[1] = _("VSWR vs Frequency");
    titles[2] = "";
  }

  /* Gather VSWR and optional S11 in one measurement pass */
  mem_realloc((void **)&vswr, mreq);
  if( rc_config.freqplots_s11 )
    mem_realloc((void **)&s11, mreq);

  fp_meas_column_t cols[2];
  int ncols = 0;
  cols[ncols++] = (fp_meas_column_t){ vswr, MEAS_VSWR };
  if( rc_config.freqplots_s11 )
    cols[ncols++] = (fp_meas_column_t){ s11, MEAS_S11 };
  fp_fill_meas_columns( ctx, cols, ncols );

  if( rc_config.freqplots_clamp_vswr )
    for( idx = 0; idx < ctx->num_fsteps; idx++ )
      if( vswr[idx] > 10.0 )
        vswr[idx] = 10.0;

  fp_plot_panel(ctx, vswr, (rc_config.freqplots_s11 ? s11 : NULL), titles);

  return TRUE;
}
