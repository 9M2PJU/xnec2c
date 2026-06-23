/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  The official website and doumentation for xnec2c is available here:
 *    https://www.xnec2c.org/
 */

/*
 * freqplots_theme_menu: builds the frequency-plots View > Color Theme submenu
 * from the core theme registry (src/themes/theme.c).  The submenu lives on the
 * freqplots window, so its construction is freqplot-specific UI and stays in
 * the freqplots tree; the theme model and registry are core and stay under
 * themes/.  The "Inverted" check item is an orthogonal axis above one radio
 * group of base theme names.
 */

#include "../interface.h"
#include "../shared.h"
#include "../callbacks.h"
#include "../i18n.h"
#include "../themes/theme.h"

/* freqplots_invert_item_sync()
 *
 * Match the Inverted item's sensitivity to whether base carries an inverted
 * variant; a theme without one (legacy, smith-derived) leaves the item
 * insensitive and shows an explanatory tooltip, set only in that state. */
  void
freqplots_invert_item_sync( GtkWidget *invert, const char *base )
{
  gboolean has = theme_has_inverted( base );

  gtk_widget_set_sensitive( invert, has );
  gtk_widget_set_tooltip_text( invert,
      has ? NULL : _("This theme has no inverted variant") );
}

/* freqplots_theme_radio_append()
 *
 * Build one base-theme radio item into the shared group, tagging it with its
 * base name and the Inverted item it governs, and syncing its active state to
 * the persisted selection without feeding the load back as user interaction. */
  static void
freqplots_theme_radio_append( GtkWidget *menu, GSList **group,
    GtkWidget *invert, const theme_menu_entry_t *e )
{
  GtkWidget *item = gtk_radio_menu_item_new_with_label( *group, e->display );

  *group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(item) );

  g_object_set_data_full( G_OBJECT(item), THEME_DATA_BASE,
      g_strdup(e->base_name), g_free );
  g_object_set_data( G_OBJECT(item), THEME_DATA_INVERT_ITEM, invert );

  SIGNAL_BLOCK( item, on_freqplots_theme_activate );
  gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(item),
      g_strcmp0( e->base_name, rc_config.freqplots_theme ) == 0 );
  SIGNAL_UNBLOCK( item, on_freqplots_theme_activate );

  g_signal_connect( item, "activate",
      G_CALLBACK(on_freqplots_theme_activate), NULL );
  g_signal_connect( item, "select",
      G_CALLBACK(on_freqplots_theme_select), NULL );
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), item );
}

/* freqplots_theme_menu_build()
 *
 * Generates the View menu's Color Theme submenu from the theme registry into
 * the glade-supplied container.  The "Inverted" check item is an orthogonal
 * axis above one radio group of base theme names; legacy heads the list as the
 * default, user custom themes follow it, and a separator divides that top
 * group from the built-in themes in registry order.
 * Programmatic active-state sync blocks the handlers so the load does not feed
 * back as user interaction. */
  void
freqplots_theme_menu_build( GtkBuilder *builder )
{
  GtkWidget       *menu = Builder_Get_Object( builder, "freqplots_color_theme_menu_menu" );
  GtkWidget       *invert;
  const GPtrArray *entries = theme_menu_entries();
  GSList          *group = NULL;
  guint            i;

  if( menu == NULL || entries == NULL )
    return;

  invert = gtk_check_menu_item_new_with_mnemonic( _("_Invert") );
  SIGNAL_BLOCK( invert, on_freqplots_theme_invert_toggled );
  gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(invert),
      rc_config.freqplots_theme_invert );
  SIGNAL_UNBLOCK( invert, on_freqplots_theme_invert_toggled );
  freqplots_invert_item_sync( invert, rc_config.freqplots_theme );
  g_signal_connect( invert, "toggled",
      G_CALLBACK(on_freqplots_theme_invert_toggled), NULL );
  gtk_menu_shell_append( GTK_MENU_SHELL(menu), invert );

  /* Legacy heads the radio list; user custom themes follow it so they sit at
   * the top, adjacent to the default and easy to find, set off by a separator
   * from the built-in themes. */
  for( i = 0; i < entries->len; i++ )
  {
    theme_menu_entry_t *e = g_ptr_array_index( entries, i );

    if( g_strcmp0( e->base_name, "legacy" ) != 0 )
      continue;

    freqplots_theme_radio_append( menu, &group, invert, e );
    break;
  }

  for( i = 0; i < entries->len; i++ )
  {
    theme_menu_entry_t *e = g_ptr_array_index( entries, i );

    if( !e->user_origin || g_strcmp0( e->base_name, "legacy" ) == 0 )
      continue;

    freqplots_theme_radio_append( menu, &group, invert, e );
  }

  gtk_menu_shell_append( GTK_MENU_SHELL(menu), gtk_separator_menu_item_new() );

  for( i = 0; i < entries->len; i++ )
  {
    theme_menu_entry_t *e = g_ptr_array_index( entries, i );

    if( g_strcmp0( e->base_name, "legacy" ) == 0 || e->user_origin )
      continue;

    freqplots_theme_radio_append( menu, &group, invert, e );
  }

  /* Revert an uncommitted hover preview when the theme list collapses; a
   * click clears the preview through activate before this hide fires. */
  g_signal_connect( menu, "hide",
      G_CALLBACK(on_freqplots_theme_menu_hide), NULL );

  gtk_widget_show_all( menu );
}
