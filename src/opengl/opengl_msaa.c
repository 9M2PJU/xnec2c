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

#include "opengl_msaa.h"
#include "opengl_rdpattern.h"
#include "opengl_structure.h"
#include "../opengl-engine/opengl_view.h"
#include "../opengl-engine/opengl_view_msaa.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/* MSAA sample count to menu widget name mapping */
typedef struct { int samples; const char *name; } msaa_widget_entry_t;

static const msaa_widget_entry_t msaa_widget_map[] = {
  {  0, "main_opengl_msaa_off" },
  {  2, "main_opengl_msaa_2x"  },
  {  4, "main_opengl_msaa_4x"  },
  {  8, "main_opengl_msaa_8x"  },
  { 16, "main_opengl_msaa_16x" }
};

/*-----------------------------------------------------------------------*/

/** msaa_update_view() - Recreate MSAA framebuffer resources for one GL view and queue redraw
 * @get_widget: function returning the GtkWidget for the target view
 * @samples: new MSAA sample count
 */
  static void
msaa_update_view(GtkWidget *(*get_widget)(void), int samples)
{
  GtkWidget *w;
  gl_view_state_t *state;

  w = get_widget();
  if( !w )
    return;

  state = gl_view_get_state(w);
  if( !state )
    return;

  gtk_gl_area_make_current(GTK_GL_AREA(w));
  gl_view_recreate_msaa(state, samples);
  gtk_widget_queue_draw(w);

} /* msaa_update_view() */

/*-----------------------------------------------------------------------*/

/** Set_MSAA_Samples() - Change MSAA sample count for all GL views
 * @samples: new sample count (0, 2, 4, 8, or 16)
 */
  void
Set_MSAA_Samples(int samples)
{
  GtkWidget *widget;
  int mi;

  rc_config.opengl_msaa_samples = samples;

  /* Update menu selection via struct map linear scan */
  for( mi = 0; mi < (int)(sizeof(msaa_widget_map) / sizeof(msaa_widget_map[0])); mi++ )
  {
    if( msaa_widget_map[mi].samples == samples )
    {
      widget = Builder_Get_Object(main_window_builder,
          (gchar *)msaa_widget_map[mi].name);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
      break;
    }
  }

  msaa_update_view(opengl_structure_get_widget, samples);
  msaa_update_view(opengl_rdpattern_get_widget, samples);

} /* Set_MSAA_Samples() */

#endif /* HAVE_OPENGL */
