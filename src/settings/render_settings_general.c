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

#ifdef HAVE_OPENGL
#include "../callbacks.h"
#include "render_settings.h"
#include "render_settings_common.h"
#include "render_settings_internal.h"
#include "../opengl/opengl_structure.h"
#include "../opengl/opengl_rdpattern.h"
#include "../opengl/opengl_state.h"

/*------------------------------------------------------------------------*/

/* Compile-time width checks for int-typed config fields */
CFG_INT_ASSERT(use_opengl_renderer);
CFG_INT_ASSERT(view_drag_constrained);
CFG_INT_ASSERT(opengl_orthographic);
CFG_DBL_ASSERT(rdpattern_overlay_scale_adj);

/* Post-apply wrappers: read new value from rc_config after generic write */

static void post_apply_set_renderer(void)
{
  opengl_set_renderer(rc_config.use_opengl_renderer);
}

static void post_apply_set_constrained(void)
{
  opengl_set_constrained_rotation(rc_config.view_drag_constrained);
}

static void post_apply_sync_ortho(void)
{
  sync_ortho_toolbar_button();
}

/*------------------------------------------------------------------------*/

/* General tab defaults: widget-bound fields and widget-less rendering defaults */
static const config_default_t general_defaults[] = {
  CFG_INT(use_opengl_renderer, "chk_opengl_renderer", post_apply_set_renderer, 1),
  CFG_INT(view_drag_constrained, "chk_constrained_rotation", post_apply_set_constrained, 1),
  CFG_INT(opengl_orthographic, "chk_orthographic", post_apply_sync_ortho, 1),
  CFG_DBL(rdpattern_overlay_scale_adj, NULL, NULL, 1.0),
};

const config_tab_defaults_t general_tab_defaults = {
  .entries       = general_defaults,
  .count         = (int)(sizeof(general_defaults) / sizeof(general_defaults[0])),
  .session       = NULL,
  .session_count = 0,
};

/*------------------------------------------------------------------------*/

/** general_tab_sync - Populate General tab widgets from rc_config
 *
 * Uses generic dispatch for field values, then applies GL-availability
 * sensitivity logic to the renderer toggle widget.
 */
void
general_tab_sync(void)
{
  GtkWidget *w;

  config_sync_tab(SETTINGS_TAB_GENERAL);

  /* Disable renderer toggle when OpenGL is unavailable on this display */
  w = Builder_Get_Object(render_settings_builder, "chk_opengl_renderer");
  if( w != NULL )
  {
    if( opengl_gl_context_failed() )
    {
      gtk_widget_set_sensitive(w, FALSE);
      gtk_widget_set_tooltip_text(w,
          "OpenGL is not available on this display.\n"
          "Cairo rendering is active.");
    }
    else
    {
      gtk_widget_set_sensitive(w, TRUE);
      gtk_widget_set_tooltip_text(w, NULL);
    }
  }
}

/*------------------------------------------------------------------------*/

/** on_general_reset_clicked - Per-tab Reset button handler for General tab
 *
 * Resets OpenGL renderer, constrained rotation, and orthographic projection
 * to defaults via config_reset_tab_user (which invokes post_apply hooks for
 * changed fields), then syncs widgets and redraws.
 */
void
on_general_reset_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  config_reset_tab_user(SETTINGS_TAB_GENERAL);

  general_tab_sync();
  opengl_structure_queue_draw();
  opengl_rdpattern_queue_draw();
}

#endif /* HAVE_OPENGL */
