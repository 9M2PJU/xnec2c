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

/* Which window is being rendered */
typedef enum
{
  RENDER_VIEW_RDPATTERN,
  RENDER_VIEW_STRUCTURE
} render_view_t;

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
  RENDER_NO_MODE
} render_status_t;

/* Dispatch-resolved structure draw parameters — passed to draw_structure backends */
typedef struct
{
  const rgb_f_t *wire_colors;   /* seg_rgb | wire_crnt_rgb | wire_chrg_rgb */
  const rgb_f_t *patch_colors;  /* patch_rgb | patch_crnt_rgb */
  double         cmax;          /* fmax(wire_crnt_cmax, patch_crnt_cmax) or 0.0 */
  gboolean       show_flow;     /* TRUE only for DRAW_CURRENTS */
  int            fstep;         /* for crnt_fstep[] access */
} struct_draw_params_t;

/* Dispatch-resolved far-field draw parameters — passed to draw_farfield backends */
typedef struct
{
  gboolean active;      /* TRUE when OVERLAY_STRUCT + excitation center available */
  float    x, y, z;    /* excitation center (structure space) */
  float    view_scale;  /* structure extent for base_scale computation */
  double   scale_adj;   /* overlay scale adjustment from user shift+scroll */
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
  gboolean (*draw_farfield)(void *ctx, int fstep, float zoom, const ff_draw_params_t *ff);

  /* Draw near E/H/Poynting field vectors; returns TRUE on success.
   * Backend iterates fields[0..n_fields-1], one batch per entry. */
  gboolean (*draw_nearfield)(void *ctx, float zoom,
      const near_field_point_t *origins, int npts,
      const nf_field_set_t *fields, int n_fields,
      double dr, double r_max);

  /* Draw structure geometry; returns TRUE always */
  gboolean (*draw_structure)(void *ctx, float zoom, const struct_draw_params_t *params);

  /* Initialize an empty scene (no geometry) */
  void (*init_empty)(void *ctx, float zoom);

  /* Set the status message on the scene */
  void (*set_status)(void *ctx, const char *msg);

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
render_check_result_t render_check(render_view_t view);

/**
 * render_get_last_rdpat_check() - Return cached result from last rdpattern render()
 *
 * Populated after each render() call with RENDER_VIEW_RDPATTERN.
 * Overlay provider reads overlay_active and mode without flag evaluation.
 */
const render_check_result_t *render_get_last_rdpat_check(void);

/**
 * render() - Unified render entry point for all backends
 * @ctx:               backend context (cast to gl_view_content_t* for GL, cairo context for Cairo)
 * @ops:               backend vtable
 * @view:              which window is being rendered
 * @zoom:              current zoom factor extracted by the caller
 * @overlay_scale_adj: structure overlay scale multiplier from user shift+scroll;
 *                     forwarded into ff_draw_params_t.scale_adj for farfield backends
 *
 * Acquires freq_data_lock, evaluates preconditions via render_check(), dispatches
 * through the ops vtable, and releases the lock.  Returns TRUE when a frame was
 * produced, FALSE for freeze-frame (SUPPRESS_INTERMEDIATE_REDRAWS).
 */
gboolean render(void *ctx, const render_ops_t *ops, render_view_t view, float zoom,
    double overlay_scale_adj);

#endif
