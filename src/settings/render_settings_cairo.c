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
#include "render_settings_common.h"
#include "render_settings_internal.h"

/*------------------------------------------------------------------------*/

/* Compile-time width checks for int-typed config fields */
CFG_INT_ASSERT(cairo_antialias);
CFG_INT_ASSERT(cairo_color_quant);
CFG_INT_ASSERT(cairo_line_cap);
CFG_INT_ASSERT(cairo_depth_bins);

/* Cairo tab dispatch table: anti-aliasing, color quantization, line cap, depth bins.
 * Radio groups: first entry carries the reset default value. */
static const config_default_t cairo_defaults[] = {
  /* Anti-aliasing radio group */
  CFG_INT(cairo_antialias, "radio_cairo_antialias_default", NULL, CAIRO_ANTIALIAS_DEFAULT),
  CFG_INT_RADIO(cairo_antialias, "radio_cairo_antialias_fast",    NULL, CAIRO_ANTIALIAS_FAST),
  CFG_INT_RADIO(cairo_antialias, "radio_cairo_antialias_none",    NULL, CAIRO_ANTIALIAS_NONE),

  /* Color quantization radio group (value = number of levels; 0=off) */
  CFG_INT(cairo_color_quant, "radio_cairo_color_quant_off", NULL, 0),
  CFG_INT_RADIO(cairo_color_quant, "radio_cairo_color_quant_8",   NULL, 8),
  CFG_INT_RADIO(cairo_color_quant, "radio_cairo_color_quant_64",  NULL, 64),
  CFG_INT_RADIO(cairo_color_quant, "radio_cairo_color_quant_128", NULL, 128),
  CFG_INT_RADIO(cairo_color_quant, "radio_cairo_color_quant_256", NULL, 256),

  /* Line cap radio group */
  CFG_INT(cairo_line_cap, "radio_cairo_line_cap_butt",   NULL, CAIRO_LINE_CAP_BUTT),
  CFG_INT_RADIO(cairo_line_cap, "radio_cairo_line_cap_round",  NULL, CAIRO_LINE_CAP_ROUND),
  CFG_INT_RADIO(cairo_line_cap, "radio_cairo_line_cap_square", NULL, CAIRO_LINE_CAP_SQUARE),

  /* Depth bins spinner */
  CFG_INT(cairo_depth_bins, "spin_cairo_depth_bins", NULL, 16),
};

const config_tab_defaults_t cairo_tab_defaults = {
  .entries       = cairo_defaults,
  .count         = (int)(sizeof(cairo_defaults) / sizeof(cairo_defaults[0])),
  .session       = NULL,
  .session_count = 0,
};

/*------------------------------------------------------------------------*/

/** on_cairo_tab_reset_clicked - Per-tab Reset button handler for Cairo tab
 *
 * Restores Cairo rendering settings to defaults, syncs widgets, and
 * redraws Cairo drawing areas only.
 */
void
on_cairo_tab_reset_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  config_reset_tab(SETTINGS_TAB_CAIRO);
  config_sync_tab(SETTINGS_TAB_CAIRO);
  xnec2_widget_queue_draw(structure_drawingarea, TRUE);
  xnec2_widget_queue_draw(rdpattern_drawingarea, TRUE);
}
