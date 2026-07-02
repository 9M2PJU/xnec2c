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
#include "../opengl/opengl_structure.h"
#include "../opengl/opengl_msaa.h"
#include "../structure_ui.h"
#include "../rdpattern_ui.h"

/*------------------------------------------------------------------------*/

/* Compile-time width checks for int-typed config fields */
CFG_INT_ASSERT(opengl_msaa_samples);
CFG_INT_ASSERT(opengl_transparent_on_click);
CFG_INT_ASSERT(rdpattern_draw_style);

CFG_FLT_ASSERT(brightness_segments);
CFG_FLT_ASSERT(brightness_patches);
CFG_FLT_ASSERT(brightness_rdpat_surface);
CFG_FLT_ASSERT(brightness_rdpat_wire);
CFG_FLT_ASSERT(brightness_nearfield);
CFG_FLT_ASSERT(brightness_ground_plane);
CFG_FLT_ASSERT(brightness_axes);
CFG_FLT_ASSERT(transparency_segments);
CFG_FLT_ASSERT(transparency_patches);
CFG_FLT_ASSERT(transparency_rdpat_surface);
CFG_FLT_ASSERT(transparency_rdpat_wire);
CFG_FLT_ASSERT(transparency_nearfield);
CFG_FLT_ASSERT(transparency_ground_plane);
CFG_FLT_ASSERT(transparency_axes);
CFG_INT_ASSERT(opengl_orthographic);
CFG_DBL_ASSERT(opengl_cylinder_radius_scale);

/* Post-apply wrappers: read value from rc_config after generic write */

static void post_apply_sync_ortho(void)
{
  sync_ortho_toolbar_button();
}


static void post_apply_set_msaa(void)
{
  Set_MSAA_Samples(rc_config.opengl_msaa_samples);
}

static void post_apply_set_radius_scale(void)
{
  opengl_structure_set_radius_scale(rc_config.opengl_cylinder_radius_scale);
}

/*------------------------------------------------------------------------*/

/* OpenGL tab dispatch table: brightness, transparency, toggles, radios.
 * Radio entries bind each widget to its enum value; reset defaults come
 * from rc_config_vars. */
static const config_default_t opengl_defaults[] = {
  /* Orthographic projection toggle (widget in OpenGL tab glade;
   * config entry here since it only affects the OpenGL renderer) */
  CFG_INT_W(opengl_orthographic, "chk_orthographic", post_apply_sync_ortho),

  /* Per-type brightness sliders (0.0=black, 1.0=full) */
  CFG_FLT_W(brightness_segments,      "scale_bright_segments",      NULL),
  CFG_FLT_W(brightness_patches,       "scale_bright_patches",       NULL),
  CFG_FLT_W(brightness_rdpat_surface, "scale_bright_rdpat_surface", NULL),
  CFG_FLT_W(brightness_rdpat_wire,    "scale_bright_rdpat_wire",    NULL),
  CFG_FLT_W(brightness_nearfield,     "scale_bright_nearfield",     NULL),
  CFG_FLT_W(brightness_ground_plane,  "scale_bright_ground_plane",  NULL),
  CFG_FLT_W(brightness_axes,          "scale_bright_axes",          NULL),

  /* Per-type transparency sliders (0.0=opaque, 1.0=fully transparent) */
  CFG_FLT_W(transparency_segments,      "scale_trans_segments",      NULL),
  CFG_FLT_W(transparency_patches,       "scale_trans_patches",       NULL),
  CFG_FLT_W(transparency_rdpat_surface, "scale_trans_rdpat_surface", NULL),
  CFG_FLT_W(transparency_rdpat_wire,    "scale_trans_rdpat_wire",    NULL),
  CFG_FLT_W(transparency_nearfield,     "scale_trans_nearfield",     NULL),
  CFG_FLT_W(transparency_ground_plane,  "scale_trans_ground_plane",  NULL),
  CFG_FLT_W(transparency_axes,          "scale_trans_axes",          NULL),

  /* Cylinder radius scale slider */
  CFG_DBL_W(opengl_cylinder_radius_scale, "scale_cylinder_scale",
      post_apply_set_radius_scale),

  /* Transparency on-click toggle */
  CFG_INT_W(opengl_transparent_on_click, "chk_only_on_click", NULL),

  /* MSAA radio group; each entry binds a widget to its enum value */
  CFG_INT_RADIO(opengl_msaa_samples, "radio_msaa_4x",  post_apply_set_msaa, MSAA_4X),
  CFG_INT_RADIO(opengl_msaa_samples, "radio_msaa_off", post_apply_set_msaa, MSAA_OFF),
  CFG_INT_RADIO(opengl_msaa_samples, "radio_msaa_2x",  post_apply_set_msaa, MSAA_2X),
  CFG_INT_RADIO(opengl_msaa_samples, "radio_msaa_8x",  post_apply_set_msaa, MSAA_8X),
  CFG_INT_RADIO(opengl_msaa_samples, "radio_msaa_16x", post_apply_set_msaa, MSAA_16X),

  /* Draw style radio group; each entry binds a widget to its enum value */
  CFG_INT_RADIO(rdpattern_draw_style, "radio_style_both",      NULL, RDPAT_STYLE_BOTH),
  CFG_INT_RADIO(rdpattern_draw_style, "radio_style_surface",   NULL, RDPAT_STYLE_SURFACE),
  CFG_INT_RADIO(rdpattern_draw_style, "radio_style_wireframe", NULL, RDPAT_STYLE_WIREFRAME),
};

const config_tab_defaults_t opengl_tab_defaults = {
  .entries       = opengl_defaults,
  .count         = (int)(sizeof(opengl_defaults) / sizeof(opengl_defaults[0])),
  .session       = NULL,
  .session_count = 0,
};

/*------------------------------------------------------------------------*/

/** on_opengl_tab_reset_clicked - Per-tab Reset button handler for OpenGL tab
 *
 * Restores orthographic projection, brightness, transparency, cylinder scale,
 * MSAA, and draw style to compiled-in defaults. Syncs widgets and redraws
 * OpenGL views.
 */
void
on_opengl_tab_reset_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  config_reset_tab_user(SETTINGS_TAB_OPENGL);
  config_sync_tab(SETTINGS_TAB_OPENGL);
  opengl_structure_invalidate();
  Queue_Structure_Redraw();
  Queue_Radiation_Redraw();
}
