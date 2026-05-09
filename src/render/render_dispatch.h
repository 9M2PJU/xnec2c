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

#ifndef RENDER_DISPATCH_H
#define RENDER_DISPATCH_H       1

#include "../common.h"
#include "../prerender/prerender_color.h"
#include "../prerender/prerender_state.h"
#include "render_message.h"

/* Content mode resolved by render_check() */
typedef enum
{
  RENDER_MODE_NONE,
  RENDER_MODE_FARFIELD,
  RENDER_MODE_NEARFIELD,
  RENDER_MODE_STRUCTURE
} render_mode_t;

/* Precondition check outcome */
typedef enum
{
  RENDER_OK,
  RENDER_SUPPRESS,      /* freeze-frame (SUPPRESS_INTERMEDIATE_REDRAWS) */
  RENDER_NO_RP_CARD,
  RENDER_NO_NF_CARD,
  RENDER_NF_NOT_READY,
  RENDER_NO_DATA,
  RENDER_NO_GEOMETRY,   /* VIEW_STRUCTURE with no geometry loaded (data.n == data.m == 0) */
  RENDER_NO_MODE
} render_status_t;

/* Dispatch-resolved structure draw parameters — passed to draw_structure backends */
typedef struct
{
  const rgb_f_t *wire_colors;   /* seg_rgb | wire_crnt_rgb | wire_chrg_rgb */
  const float   *wire_widths;   /* seg_width [data.n] per-segment line widths */
  const rgb_f_t *patch_colors;  /* patch_rgb | patch_crnt_rgb */
  double         cmax;          /* fmax(wire_crnt_cmax, patch_crnt_cmax) or 0.0 */
  gboolean       show_flow;     /* TRUE only for DRAW_CURRENTS */
  int            fstep;         /* for crnt_fstep[] access */
} struct_draw_params_t;

/* Dispatch-resolved far-field draw parameters — passed to draw_farfield backends.
 * Excitation centroid coordinates are pre-scaled to pattern space by dispatch.
 * Zero coordinates produce identity transform when no excitation center exists. */
typedef struct
{
  float x, y, z;          /* excitation center, pre-scaled to pattern space */
  float pattern_radius;    /* radiation pattern r_max (from ff_pre[fstep]) */
  float off_len;           /* sqrt(x²+y²+z²); clip extent = pattern_radius + off_len */
} ff_draw_params_t;

/* Near-field vector set descriptor — one per active field type (E, H, Poynting).
 * Dispatch builds 0-3 of these; backend iterates and emits one batch per entry. */
typedef struct
{
  const nf_vector_t     *vecs;        /* pre-scaled displacement + baked color */
} nf_field_set_t;

#define NF_FIELD_SETS_MAX 3

/* Result of render_check(): mode, status, and display metadata */
typedef struct
{
  render_status_t  status;
  render_mode_t    mode;
  int              fstep;
  const char      *message;      /* STATUS_MSG_* pointer; NULL when RENDER_OK */
  gboolean         show_gradient;
  gboolean         overlay_active; /* resolved from OVERLAY_STRUCT */
} render_check_result_t;

/* Backend operations vtable — dispatch decides what to draw; backends draw it */
typedef struct
{
  /* Draw far-field gain pattern; returns TRUE on success, FALSE on data miss */
  gboolean (*draw_farfield)(void *ctx, int fstep, const ff_draw_params_t *ff);

  /* Draw near E/H/Poynting field vectors; returns TRUE on success.
   * Backend iterates fields[0..n_fields-1], one batch per entry. */
  gboolean (*draw_nearfield)(void *ctx,
      const near_field_point_t *origins, int npts,
      const nf_field_set_t *fields, int n_fields,
      double dr, double r_max);

  /* Draw structure geometry; returns TRUE always.
   * extent: content half-extent for projection scaling. */
  gboolean (*draw_structure)(void *ctx, float extent, const struct_draw_params_t *params);

  /* Initialize an empty scene (no geometry) */
  void (*init_empty)(void *ctx);

  /* Set the status message on the scene */
  void (*set_status)(void *ctx, const char *msg);

  /* Draw xyz axes for the primary content extent */
  void (*draw_axes)(void *ctx, float extent);

  /* Enable or disable gradient display */
  void (*set_gradient)(void *ctx, gboolean show);
} render_ops_t;

/**
 * render_check() - Unified precondition cascade for rdpattern and structure views
 * @view: which window is being rendered
 *
 * Returns a render_check_result_t with status RENDER_OK when all preconditions
 * pass and the mode is determined.  On failure, message points to a STATUS_MSG_*
 * constant and status describes the failure reason.
 */
render_check_result_t render_check(view_type_t view_type);

/**
 * render() - Unified render entry point for all backends
 * @ctx:  backend context (cast to gl_view_content_t* for GL, cairo context for Cairo)
 * @ops:  backend vtable
 * @view: view_t* for the window being rendered; view->type selects mode,
 *        view->zoom provides the zoom factor
 *
 * Acquires freq_data_lock, evaluates preconditions via render_check(), dispatches
 * through the ops vtable, and releases the lock.  Reads overlay scale from
 * rc_config.rdpattern_overlay_scale_adj.
 * Returns TRUE when a frame was produced, FALSE for freeze-frame.
 */
gboolean render(void *ctx, const render_ops_t *ops, view_t *view);

/* Cairo backend vtables; used by the glade draw handlers in callbacks.c
 * to pass the correct ops to render_cairo() without a widget identity check. */
extern const render_ops_t cairo_struct_ops;
extern const render_ops_t cairo_rdpat_ops;

/**
 * render_cairo() - Cairo draw path shared by both glade draw handlers
 * @cr:  Cairo context provided by GTK
 * @v:   view resolved by the calling glade handler
 * @ops: Cairo backend vtable resolved by the calling glade handler
 *
 * Builds a cairo_render_ctx_t and dispatches through render().
 * Identity resolution is the caller's responsibility; no widget
 * pointer is accepted here.
 * Returns TRUE when a frame was produced, FALSE for freeze-frame or
 * when @v is NULL.
 */
gboolean render_cairo(cairo_t *cr, view_t *v, const render_ops_t *ops);

#endif
