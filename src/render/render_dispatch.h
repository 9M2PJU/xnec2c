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

/* Result of render_check(): mode, status, and display metadata */
typedef struct
{
  render_status_t  status;
  render_mode_t    mode;
  int              fstep;
  const char      *message;   /* STATUS_MSG_* pointer; NULL when RENDER_OK */
  gboolean         show_gradient;
} render_check_result_t;

/* Backend operations vtable — dispatch decides what to draw; backends draw it */
typedef struct
{
  /* Draw far-field gain pattern; returns TRUE on success, FALSE on data miss */
  gboolean (*draw_farfield)(void *ctx, int fstep, float zoom);

  /* Draw near E/H field; returns TRUE on success, FALSE on data miss */
  gboolean (*draw_nearfield)(void *ctx, int fstep, float zoom);

  /* Draw structure geometry; returns TRUE always */
  gboolean (*draw_structure)(void *ctx, float zoom);

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
 * render() - Unified render entry point for all backends
 * @ctx:  backend context (cast to gl_view_content_t* for GL, cairo context for Cairo)
 * @ops:  backend vtable
 * @view: which window is being rendered
 * @zoom: current zoom factor extracted by the caller
 *
 * Acquires freq_data_lock, evaluates preconditions via render_check(), dispatches
 * through the ops vtable, and releases the lock.  Returns TRUE when a frame was
 * produced, FALSE for freeze-frame (SUPPRESS_INTERMEDIATE_REDRAWS).
 */
gboolean render(void *ctx, const render_ops_t *ops, render_view_t view, float zoom);

#endif
