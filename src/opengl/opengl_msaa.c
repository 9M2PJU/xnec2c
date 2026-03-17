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

/* Set_MSAA_Samples()
 *
 * Change MSAA sample count for all GL views
 */
  void
Set_MSAA_Samples(int samples)
{
  static char *msaa_widget_names[] = {
    "main_opengl_msaa_off",
    NULL,
    "main_opengl_msaa_2x",
    NULL,
    "main_opengl_msaa_4x",
    NULL, NULL, NULL,
    "main_opengl_msaa_8x",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "main_opengl_msaa_16x"
  };

  GtkWidget *widget;
  gl_view_state_t *state;

  rc_config.opengl_msaa_samples = samples;

  /* Update menu selection */
  if( samples <= 16 && msaa_widget_names[samples] )
  {
    widget = Builder_Get_Object(main_window_builder, msaa_widget_names[samples]);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
  }

  /* Recreate MSAA resources for structure view */
  {
    GtkWidget *w = opengl_structure_get_widget();
    if( w )
    {
      state = gl_view_get_state(w);
      if( state )
      {
        gtk_gl_area_make_current(GTK_GL_AREA(w));
        gl_view_recreate_msaa(state, samples);
        gtk_widget_queue_draw(w);
      }
    }
  }

  /* Recreate MSAA resources for radiation pattern view */
  {
    GtkWidget *w = opengl_rdpattern_get_widget();
    if( w )
    {
      state = gl_view_get_state(w);
      if( state )
      {
        gtk_gl_area_make_current(GTK_GL_AREA(w));
        gl_view_recreate_msaa(state, samples);
        gtk_widget_queue_draw(w);
      }
    }
  }

} /* Set_MSAA_Samples() */

#endif /* HAVE_OPENGL */
