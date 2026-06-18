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

#ifndef FREQPLOTS_INTERNAL_H
#define FREQPLOTS_INTERNAL_H  1

#include "../common.h"
#include "../plot_freqdata.h"
#include "../measurements.h"
#include "freqplots_render.h"

/* Map an axis value to its pixel x within rect, rounding to the nearest pixel.
 * Single source for x-axis sample placement, shared by the trace renderer and
 * the click-resolution rail so snap points land on the drawn trace. */
static inline int
fp_axis_pixel_x(const GdkRectangle *rect, double val, double vmin, double vmax)
{
  return rect->x + (int)((double)rect->width * (val - vmin) / (vmax - vmin) + 0.5);
}

/* Per-view plot-table init, rectangle accessors, and width sync
 * (freqplots_core.c) */
void          fr_plots_init(freqplots_view_t *v);
fr_plot_t    *get_fr_plot(freqplots_view_t *v, int posn, int fr);
GdkRectangle *get_plot_rect(freqplots_view_t *v, int posn, int fr);
void          fr_plot_sync_widths(freqplots_view_t *v, fr_plot_t *fr_plot);
fr_plot_t    *fr_plot_at(freqplots_view_t *v, double px, double py);
void          print_fr_plot(fr_plot_t *p);

/* Scale fitting and scale labels (freqplots_scale.c) */
double Fit_to_Scale(double *max, double *min, int *nval);
void   Fit_to_Scale2(double *max1, double *min1,
    double *max2, double *min2, int *nval);
void   fscale_extent_fit(freq_loop_data_t *fld, int round_x_axis,
    double *min, double *max, int *nval);
void   Plot_Horizontal_Scale(fp_render_t *fp,
    rgb_f_t c,
    int x, int y, int width,
    double max, double min, int nval);
void   Plot_Vertical_Scale(fp_render_t *fp,
    rgb_f_t c,
    int x, int y, int height,
    double max, double min, int nval);

/* Shared XY drawing primitives (freqplots_graph.c) */
void Draw_Plotting_Frame(fp_render_t *fp, gchar **title,
    GdkRectangle *rect, int nhor, int nvert);
void Draw_Graph(fp_render_t *fp,
    rgb_f_t trace_c,
    GdkRectangle *rect,
    double *a, double *b,
    double amax, double amin,
    double bmax, double bmin,
    int nval, int nval_max, int side);

/* Plot types (freqplots_xy.c, graphs/smith_graph.c) */
void Plot_Graph(freqplots_view_t *v, fp_render_t *fp,
    double *y_left, double *y_right, double *x, int nx,
    int *card_nfsteps, char *titles[], int posn, fp_panel_t panel);
void Plot_Graph_Smith(freqplots_view_t *v, fp_render_t *fp,
    double *fa, double *fb, double *fc, int nc, int *card_nfsteps, int posn);

/* Per-frame data shared with every dispatched plot renderer.  posn advances
 * as each renderer claims drawing-area panels. */
typedef struct
{
  freqplots_view_t *view;         /* active render target for this frame */
  fp_render_t   *fp;              /* per-frame depth-buffer render handle */
  int           *valid_steps_map; /* compact step indices into save.* arrays */
  double        *fplot;           /* x-axis frequencies, one per valid step */
  int           *card_nfsteps;    /* valid rendered point count per FR card */
  measurement_t *meas_rows;       /* one full measurement per valid step,
                                   * computed once per frame and shared by
                                   * every plot type */
  int            num_fsteps;      /* count of valid steps */
  int            posn;            /* next free panel position */
} fp_plot_ctx_t;

/* One output column bound to a measurement_t union field by MEAS_* index. */
typedef struct
{
  double *dst;      /* destination buffer holding num_fsteps doubles */
  int     meas_idx; /* index into measurement_t.a[] */
} fp_meas_column_t;

/* Dispatch-core helpers shared by every plot type (freqplots_core.c) */
void fp_fill_meas_columns(fp_plot_ctx_t *ctx,
    const fp_meas_column_t *cols, int ncols);
void fp_plot_panel(fp_plot_ctx_t *ctx,
    double *left, double *right, char *titles[3], fp_panel_t panel);
void fp_plot_smith_panel(fp_plot_ctx_t *ctx,
    double *fa, double *fb, double *fc, int nc, fp_panel_t panel);

/* Per-type plot renderers; the enabled predicate gates the accumulating
 * render pass.  Defined one type per file (freqplots_<type>.c). */
int      fp_gain_enabled(void);
gboolean fp_gain_render(fp_plot_ctx_t *ctx);
int      fp_viewer_enabled(void);
gboolean fp_viewer_render(fp_plot_ctx_t *ctx);
int      fp_vswr_enabled(void);
gboolean fp_vswr_render(fp_plot_ctx_t *ctx);
int      fp_impedance_enabled(void);
gboolean fp_impedance_render(fp_plot_ctx_t *ctx);
int      fp_ant_temp_enabled(void);
gboolean fp_ant_temp_render(fp_plot_ctx_t *ctx);

/* Click-event state shared between the data and input modules
 * (freqplots_input.c) */
void      save_click_event(freqplots_view_t *v, GdkEvent *e);
GdkEvent *freqplots_pending_click(freqplots_view_t *v);

#endif /* FREQPLOTS_INTERNAL_H */
