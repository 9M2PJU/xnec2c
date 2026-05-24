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
#include "render_settings_internal.h"
#include "render_settings_common.h"
#include "../opengl/opengl_structure.h"
#include "../opengl/opengl_rdpattern.h"
#include "../opengl/opengl_msaa.h"


/*------------------------------------------------------------------------*/

/** on_render_settings_changed - Unified signal handler for all settings widgets
 *
 * Delegates to per-tab apply functions and queues redraws.
 * Suppressed during sync_from_config to prevent feedback loops.
 */
void
on_render_settings_changed(GtkWidget *widget, gpointer user_data)
{
  settings_tab_t tab;

  (void)user_data;

  tab = config_find_tab_for_widget(widget);
  if( tab >= SETTINGS_TAB_COUNT )
    return;

  config_apply_tab(tab);

  /* Redraw only the views affected by the owning tab.  No invalidate needed
   * for brightness/transparency since visual params are read from rc_config
   * each frame.  Cylinder scale and draw style changes are detected by their
   * own staleness checks. */
  switch( tab )
  {
    case SETTINGS_TAB_GENERAL:
      /* Renderer switch and ortho/rotation changes affect all views */
      opengl_structure_queue_draw();
      opengl_rdpattern_queue_draw();
      xnec2_widget_queue_draw(structure_drawingarea, TRUE);
      xnec2_widget_queue_draw(rdpattern_drawingarea, TRUE);
      break;

    case SETTINGS_TAB_OPENGL:
      opengl_structure_queue_draw();
      opengl_rdpattern_queue_draw();
      break;

    case SETTINGS_TAB_CAIRO:
      xnec2_widget_queue_draw(structure_drawingarea, TRUE);
      xnec2_widget_queue_draw(rdpattern_drawingarea, TRUE);
      break;

    default:
      BUG("on_render_settings_changed: unknown tab %d\n", (int)tab);
      break;
  }
}

/*------------------------------------------------------------------------*/

/** render_settings_sync_from_config - Populate all widgets from rc_config
 *
 * Called after window creation and when external code changes rc_config
 * (e.g., menu item handlers).
 */
void
render_settings_sync_from_config(void)
{
  /* Toolbar buttons live in main/rdpattern builders; sync them unconditionally */
  sync_ortho_toolbar_button();

  if( render_settings_builder == NULL )
    return;

  general_tab_sync();
  config_sync_tab(SETTINGS_TAB_OPENGL);
  config_sync_tab(SETTINGS_TAB_CAIRO);
}

/*------------------------------------------------------------------------*/

/** render_settings_init - Create builder, load glade files, wire signals
 *
 * Loads render_settings.glade (window + General tab), then appends
 * OpenGL and Cairo tab pages from their respective sub-glade files.
 * Idempotent; returns TRUE if the window was already created.
 */
gboolean
render_settings_init(void)
{
  if( render_settings_window != NULL )
    return TRUE;

  if( !render_settings_load_glade() )
    return FALSE;

  /* Append tab pages: General (from render_settings.glade),
   * OpenGL (from opengl_settings.glade), Cairo (from cairo_settings.glade) */
  {
    GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object(
          render_settings_builder, "render_settings_notebook"));
    GtkWidget *general_content = GTK_WIDGET(gtk_builder_get_object(
          render_settings_builder, "general_tab_content"));
    GtkWidget *gl_content = GTK_WIDGET(gtk_builder_get_object(
          render_settings_builder, "opengl_tab_content"));
    GtkWidget *cairo_content = GTK_WIDGET(gtk_builder_get_object(
          render_settings_builder, "cairo_tab_content"));

    if( notebook != NULL && general_content != NULL )
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
          general_content, gtk_label_new("General"));

    if( notebook != NULL && gl_content != NULL )
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
          gl_content, gtk_label_new("OpenGL"));

    if( notebook != NULL && cairo_content != NULL )
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
          cairo_content, gtk_label_new("Cairo"));

    /* Flip tab side for RTL locales */
    if( notebook != NULL &&
        gtk_widget_get_direction(notebook) == GTK_TEXT_DIR_RTL )
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_RIGHT);
  }

  render_settings_sync_from_config();

  return TRUE;
}

/*------------------------------------------------------------------------*/

/** render_settings_show - Show the settings window, init if needed */
void
render_settings_show(void)
{
  if( !render_settings_init() )
    return;

  render_settings_sync_from_config();
  gtk_widget_show_all(render_settings_window);
  gtk_window_present(GTK_WINDOW(render_settings_window));
}

/*------------------------------------------------------------------------*/

/** render_settings_hide - Hide the settings window */
void
render_settings_hide(void)
{
  if( render_settings_window != NULL )
    gtk_widget_hide(render_settings_window);
}

/*------------------------------------------------------------------------*/

/* Signal handler: menu item activate */
void
on_render_settings_show_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void)menuitem;
  (void)user_data;

  render_settings_show();
}

/*------------------------------------------------------------------------*/

/* Signal handler: Close button */
void
on_render_settings_close_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  render_settings_hide();
}

/*------------------------------------------------------------------------*/

/* Signal handler: window close (X button) */
gboolean
on_render_settings_window_delete_event(
    GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;

  render_settings_hide();
  return TRUE;
}

/*------------------------------------------------------------------------*/

/* Signal handler: window destroy */
void
on_render_settings_window_destroy(GtkWidget *widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;

  if( render_settings_builder != NULL )
  {
    g_object_unref(render_settings_builder);
    render_settings_builder = NULL;
  }

  render_settings_window = NULL;
}

#endif /* HAVE_OPENGL */
