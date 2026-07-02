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

/*
 * render_settings_common: dispatch table assembly, reset operations, and
 * glade resource loading.  Per-widget IO and tab-level apply/sync live in
 * render_settings_apply.c.
 */

#include "../shared.h"
#include "../callbacks.h"
#include "../rc_config.h"
#include "render_settings_common.h"
#include "render_settings_internal.h"

/* Authoritative storage for the settings window widget; extern-declared in
 * render_settings_internal.h.  Defined here because render_settings_load_glade
 * is the sole write site. */
GtkWidget *render_settings_window = NULL;

/* Top-level dispatch table indexed by settings_tab_t */
const config_tab_defaults_t *render_tab_defaults[SETTINGS_TAB_COUNT] = {
  [SETTINGS_TAB_GENERAL] = &general_tab_defaults,
  [SETTINGS_TAB_OPENGL]  = &opengl_tab_defaults,
  [SETTINGS_TAB_CAIRO]   = &cairo_tab_defaults,
};

/*------------------------------------------------------------------------*/

/** config_reset_tab_user - Reset one tab and invoke post_apply hooks
 * @tab: which tab to reset
 *
 * Joins each render row to its persistence row by field pointer, applies the
 * rc_config_vars default, and invokes post_apply when the value changed.
 * Radio siblings share one field; each field resets once through its first
 * occurrence in the tab.
 */
void
config_reset_tab_user(settings_tab_t tab)
{
  const config_tab_defaults_t *td = render_tab_defaults[tab];
  int i, j;

  for( i = 0; i < td->count; i++ )
  {
    const config_default_t *e = &td->entries[i];
    rc_config_vars_t *v;
    char old_val[sizeof(double)];

    /* Skip a field already reset by an earlier entry (radio siblings). */
    for( j = 0; j < i && td->entries[j].field != e->field; j++ )
      ;
    if( j < i )
      continue;

    v = rc_config_find_by_field(e->field);
    if( v == NULL )
    {
      BUG("config_reset_tab_user: field has no rc_config_vars row\n");
      continue;
    }

    memcpy(old_val, e->field, e->size);
    rc_config_set_default(v);

    if( e->post_apply != NULL && memcmp(old_val, e->field, e->size) != 0 )
      e->post_apply();
  }
}

/*------------------------------------------------------------------------*/

/** render_settings_load_glade - Load all three settings glade resources
 *
 * Creates the builder, loads render_settings.glade, opengl_settings.glade,
 * and cairo_settings.glade, connects signals, and extracts the window widget.
 * Returns TRUE on success; on failure, logs the error and returns FALSE.
 */
gboolean
render_settings_load_glade(void)
{
  GError *gerror = NULL;

  render_settings_builder = gtk_builder_new();
  if( !gtk_builder_add_from_resource(render_settings_builder,
        "/settings/render_settings.glade", &gerror) )
  {
    pr_err("render_settings_init: failed to load glade: %s\n",
        gerror->message);
    g_error_free(gerror);
    return FALSE;
  }

#ifdef HAVE_OPENGL
  if( !gtk_builder_add_from_resource(render_settings_builder,
        "/settings/opengl_settings.glade", &gerror) )
  {
    pr_err("render_settings_init: failed to load opengl glade: %s\n",
        gerror->message);
    g_error_free(gerror);
    return FALSE;
  }
#endif

  if( !gtk_builder_add_from_resource(render_settings_builder,
        "/settings/cairo_settings.glade", &gerror) )
  {
    pr_err("render_settings_init: failed to load cairo glade: %s\n",
        gerror->message);
    g_error_free(gerror);
    return FALSE;
  }

  gtk_builder_connect_signals(render_settings_builder, NULL);

  render_settings_window = GTK_WIDGET(
      gtk_builder_get_object(render_settings_builder,
        "render_settings_window"));
  if( render_settings_window == NULL )
  {
    pr_err("render_settings_init: "
        "failed to get render_settings_window from glade\n");
    return FALSE;
  }

  return TRUE;
}
