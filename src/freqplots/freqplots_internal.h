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

/* Plot-rectangle accessors and width sync (freqplots_core.c) */
fr_plot_t    *get_fr_plot(int posn, int fr);
GdkRectangle *get_plot_rect(int posn, int fr);
void          fr_plot_sync_widths(fr_plot_t *fr_plot);
void          print_fr_plot(fr_plot_t *p);

/* Scale fitting and scale labels (freqplots_scale.c) */
double Fit_to_Scale(double *max, double *min, int *nval);
void   Fit_to_Scale2(double *max1, double *min1,
    double *max2, double *min2, int *nval);
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
void Plot_Graph(fp_render_t *fp,
    double *y_left, double *y_right, double *x, int nx,
    char *titles[], int posn);
void Plot_Graph_Smith(fp_render_t *fp,
    double *fa, double *fb, double *fc, int nc, int posn);

/* Resolve a Smith-chart click to a locus frequency; FALSE when the chart is
 * not displayed or the click is outside it (graphs/smith_graph.c). */
gboolean fp_smith_freq_at_pixel(double px, double py,
    gboolean snap_to_step, double *fmhz);

/* Per-frame data shared with every dispatched plot renderer.  posn advances
 * as each renderer claims drawing-area panels. */
typedef struct
{
  fp_render_t   *fp;              /* per-frame depth-buffer render handle */
  int           *valid_steps_map; /* compact step indices into save.* arrays */
  double        *fplot;           /* x-axis frequencies, one per valid step */
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
    double *left, double *right, char *titles[3]);

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
void      save_click_event(GdkEvent *e);
GdkEvent *freqplots_pending_click(void);

/* Layout-cache for window-resize detection in Plot_Graph; owned by
 * freqplots_xy.c, reset by Plots_Window_Killed() in freqplots_input.c. */
extern int fr_prev_width_available;
extern int fr_prev_ngraphs;

#endif /* FREQPLOTS_INTERNAL_H */
