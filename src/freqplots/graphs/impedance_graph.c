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
 * impedance_graph: Impedance plot type.
 *
 * Plots real/imaginary or magnitude/phase impedance against frequency and
 * the Smith chart locus.  All four impedance columns come from the shared
 * meas_calc() source so every plot type reads one measurement origin.
 */

#include "../freqplots_internal.h"
#include "../../shared.h"

  int
fp_impedance_enabled(void)
{
  /* The three impedance panels share one feedpoint availability. */
  return (rc_config.freqplots_zrlzim_togglebutton ||
      rc_config.freqplots_zmgzph_togglebutton ||
      rc_config.freqplots_smith_togglebutton) &&
    fp_panel_available( FP_PANEL_ZRLZIM );
}

/* Impedance trace buffers, reused across fp_impedance_render() calls. */
static double *zreal_c = NULL, *zimag_c = NULL;
static double *zmagn_c = NULL, *zphase_c = NULL;

/* fp_impedance_free()
 *
 * Releases the impedance trace buffers.
 */
  void
fp_impedance_free( void )
{
  mem_array_free( &zreal_c );
  mem_array_free( &zimag_c );
  mem_array_free( &zmagn_c );
  mem_array_free( &zphase_c );

} /* fp_impedance_free() */

  gboolean
fp_impedance_render(fp_plot_ctx_t *ctx)
{
  char *titles[3];

  mem_array_realloc(&zreal_c, ctx->num_fsteps);
  mem_array_realloc(&zimag_c, ctx->num_fsteps);
  mem_array_realloc(&zmagn_c, ctx->num_fsteps);
  mem_array_realloc(&zphase_c, ctx->num_fsteps);

  fp_meas_column_t cols[] = {
    { zreal_c,  MEAS_ZREAL  },
    { zimag_c,  MEAS_ZIMAG  },
    { zmagn_c,  MEAS_ZMAG   },
    { zphase_c, MEAS_ZPHASE },
  };
  fp_fill_meas_columns( ctx, cols, 4 );

  /* Deposit every impedance panel unconditionally; the dispatch gate emits
   * each only when selected (primary) or pinned (popup). */
  titles[0] = _("Z-real");
  titles[1] = _("Impedance vs Frequency");
  titles[2] = _("Z-imag");
  fp_plot_panel(ctx, zreal_c, zimag_c, titles, FP_PANEL_ZRLZIM);

  titles[0] = _("Z-magn");
  titles[1] = _("Impedance vs Frequency");
  titles[2] = _("Z-phase");
  fp_plot_panel(ctx, zmagn_c, zphase_c, titles, FP_PANEL_ZMGZPH);

  fp_plot_smith_panel(ctx, zreal_c, zimag_c, ctx->fplot,
      ctx->num_fsteps, FP_PANEL_SMITH);

  return TRUE;
}
