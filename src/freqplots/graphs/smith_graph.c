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
 * smith_graph: Smith-chart frequency plot type.
 *
 * The reactance/resistance background arcs are sampled into grid-layer
 * segments, the impedance locus and its markers deposit on the magenta
 * layer, and the current-frequency marker on the top layer.
 */

#include "../freqplots_internal.h"
#include "../../shared.h"

#include <math.h>

  static void
Calculate_Smith( double zr, double zi, double z0, double *re, double *im )
{
  complex double z = zr / z0 + I * zi / z0;
  complex double r = ( z - 1.0 ) / ( z + 1.0 );
  *re = creal( r );
  *im = cimag( r );
}

/*-----------------------------------------------------------------------*/

/* Plot_Graph_Smith()
 *
 * Plots graphs of two functions against a common variable
 */
  void
Plot_Graph_Smith(
	fp_render_t *fp,
	double *fa, double *fb, double *fc,
	int nc, int posn )
{
  int plot_height, plot_y_position;
  int idx;
  GdkPoint *points = NULL;
  int scale, x0, y0, x, y;
  double re, im;

  GdkRectangle plot_rect;

  /* Pango layout size */
  static int layout_width, layout_height, width1, height;

  (void)fc;

  pango_text_size(freqplots_drawingarea, &layout_width, &layout_height, "000000");

  /* Available height for each graph.
   * (np=number of graphs to be plotted) */
  plot_height = freqplots_height / calc_data.ngraph;
  plot_y_position   = ( freqplots_height * posn) / calc_data.ngraph;

  /* Plot box rectangle */
  plot_rect.x = layout_width + 4;
  plot_rect.y = plot_y_position + 2;
  plot_rect.width = freqplots_width - 8 - 2 * layout_width;
  plot_rect.height = plot_height - 8 - 2 * layout_height;

  /* Reserve vertical space for the chart title row */
  pango_text_size(freqplots_drawingarea, &width1, &height, _("Smith Chart") );
  plot_rect.y += height;

  x0 = plot_rect.x + plot_rect.width  / 2;
  y0 = plot_rect.y + plot_rect.height / 2;
  scale = plot_rect.width;
  if( scale > plot_rect.height )
      scale = plot_rect.height;

  /* Draw smith background */
  for( idx = 0; idx < 3; idx++ )
  {
       int div = 2 << idx;
       double angle[] = { 1.0, 1.2, 1.35 };
       fp_add_arc( fp, x0 + scale / 2 - scale / div, y0, scale / div,
           0, M_2PI, FP_Z_GRID, COLOR_GREY );
       fp_add_arc( fp, x0 + scale / 2, y0 - scale / div, scale / div,
           M_PI / 2.0, M_PI * angle[idx], FP_Z_GRID, COLOR_GREY );
       fp_add_arc( fp, x0 + scale / 2, y0 + scale / div, scale / div,
           M_PI * 1.5, ( 2.0 - angle[idx]) * M_PI, FP_Z_GRID, COLOR_GREY );
  }

  /* Calculate points to plot */
  mem_alloc((void **)&points, (size_t)nc * sizeof(GdkPoint));

  if( points == NULL )
  {
    pr_err("memory allocation for points failed\n");
    Stop( ERR_OK, _("Draw_Graph():"
          "Memory allocation for points failed") );
    return;
  }

  for( idx = 0; idx < nc; idx++ )
  {
    Calculate_Smith( fa[idx], fb[idx], calc_data.zo, &re, &im );

    // flip plot vertically because negative imaginary is the bottom half
    im = -im;

    points[idx].x = x0 + (gint)( re * scale / 2 );
    points[idx].y = y0 + (gint)( im * scale / 2 );
    fp_add_filled_square( fp, points[idx].x, points[idx].y, 6,
        FP_Z_LEFT, COLOR_MAGENTA );
  }

  /* Draw the graph */
  fp_add_polyline( fp, points, nc, FP_Z_LEFT, COLOR_MAGENTA );
  mem_free((void **)&points);

  /* Draw a vertical line to show current freq if it was
   * changed by a user click on the plots drawingarea */
  if( calc_data.fmhz_save > 0.0 )
  {
    rgb_f_t green_c = isFlagSet(SY_OPTIMIZER_ACTIVE)
        ? COLOR_DARK_GREEN : COLOR_GREEN;

    Calculate_Smith( creal(netcx.zped), cimag(netcx.zped), calc_data.zo, &re, &im );

    // flip plot vertically because negative imaginary is the bottom half
    im = -im;

    x = x0 + (gint)( re * scale / 2 );
    y = y0 + (gint)( im * scale / 2 );
    fp_add_filled_square( fp, x, y, 8, FP_Z_GREEN, green_c );
  }

} /* Plot_Graph_Smith() */
