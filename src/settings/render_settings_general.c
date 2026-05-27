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

#include "../shared.h"
#include "../callbacks.h"
#include "render_settings.h"
#include "render_settings_common.h"
#include "render_settings_internal.h"

#include "../structure_ui.h"
#include "../rdpattern_ui.h"


/*------------------------------------------------------------------------*/

/* Compile-time width checks for int-typed config fields */
CFG_INT_ASSERT(view_drag_constrained);
CFG_DBL_ASSERT(rdpattern_overlay_scale_adj);

/* Post-apply wrappers: read new value from rc_config after generic write */

CFG_INT_ASSERT(use_opengl_renderer);

static void post_apply_set_renderer(void)
{
  opengl_set_renderer(rc_config.use_opengl_renderer);
}

static void post_apply_set_constrained(void)
{
  opengl_set_constrained_rotation(rc_config.view_drag_constrained);
}

/*------------------------------------------------------------------------*/

/* Reset default: enable OpenGL renderer only when built with OpenGL support */
#ifdef HAVE_OPENGL
# define RENDERER_RESET_DEFAULT 1
#else
# define RENDERER_RESET_DEFAULT 0
#endif

/* General tab defaults: renderer toggle, constrained rotation, and
 * overlay scale (renderer-agnostic; used by both Cairo and OpenGL paths).
 * Orthographic projection lives in the OpenGL tab (OpenGL-only). */
static const config_default_t general_defaults[] = {
  CFG_INT(use_opengl_renderer, "chk_opengl_renderer", post_apply_set_renderer, RENDERER_RESET_DEFAULT),
  CFG_INT(view_drag_constrained, "chk_constrained_rotation", post_apply_set_constrained, 1),

  /* Overlay scale (no widget; programmatic session state via shift+scroll) */
  CFG_DBL(rdpattern_overlay_scale_adj, NULL, NULL, 1.0),
};

const config_tab_defaults_t general_tab_defaults = {
  .entries       = general_defaults,
  .count         = (int)(sizeof(general_defaults) / sizeof(general_defaults[0])),
  .session       = NULL,
  .session_count = 0,
};

/*------------------------------------------------------------------------*/


/** on_general_reset_clicked - Per-tab Reset button handler for General tab
 *
 * Resets constrained rotation, renderer toggle, and overlay scale to
 * defaults, syncs widgets, and redraws.  The renderer toggle is disabled
 * when OpenGL is unavailable (build-time or runtime context failure).
 */
void
on_general_reset_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  config_reset_tab_user(SETTINGS_TAB_GENERAL);

  general_tab_sync();
  Queue_Structure_Redraw();
  Queue_Radiation_Redraw();
}
