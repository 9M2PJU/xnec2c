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
 * render_settings_apply: per-widget IO and tab-level apply/sync dispatch.
 *
 * Reads widget state into rc_config (apply) and writes rc_config state into
 * widgets (sync) using GTK_IS_* runtime introspection.  Signal blocking
 * during sync prevents radio-group siblings from re-firing on_render_settings_changed
 * mid-batch.
 */

#include "../shared.h"
#include "../callbacks.h"
#include "render_settings_common.h"
#include "render_settings_internal.h"

/*------------------------------------------------------------------------*/

/* Size-aware field accessors for the config dispatch engine.
 * rc_config fields may be typed as enums (eg cairo_antialias_t) whose
 * storage width is implementation-defined; these helpers read/write
 * through the field's actual width via memcpy, avoiding aliasing and
 * width mismatches from direct pointer casts. */

static inline int
field_read_int(const void *field, size_t size)
{
  int val = 0;
  memcpy(&val, field, size);
  return val;
}

static inline void
field_write_int(void *field, size_t size, int val)
{
  memcpy(field, &val, size);
}

static inline float
field_read_float(const void *field)
{
  float val;
  memcpy(&val, field, sizeof(float));
  return val;
}

static inline void
field_write_float(void *field, float val)
{
  memcpy(field, &val, sizeof(float));
}

static inline double
field_read_double(const void *field)
{
  double val;
  memcpy(&val, field, sizeof(double));
  return val;
}

static inline void
field_write_double(void *field, double val)
{
  memcpy(field, &val, sizeof(double));
}

/*------------------------------------------------------------------------*/

/** config_apply_entry - Read one widget into its rc_config field
 * @e: dispatch entry describing the field, widget, and hook
 *
 * Uses GTK_IS_* runtime introspection to determine widget type.
 * Calls post_apply only when the field value changes.
 */
static void
config_apply_entry(const config_default_t *e)
{
  GtkWidget *w;
  union { int i; float f; double d; } old_val;

  if( e->widget_id == NULL )
    return;

  w = Builder_Get_Object(render_settings_builder, e->widget_id);
  if( w == NULL )
  {
    BUG("config_apply_entry: missing widget '%s'\n", e->widget_id);
    return;
  }

  /* Save old value for change detection */
  memcpy(&old_val, e->field, e->size);

  if( GTK_IS_RADIO_BUTTON(w) )
  {
    /* Only write from the active radio in a group */
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) )
      field_write_int(e->field, e->size, e->def.i);
  }
  else if( GTK_IS_SPIN_BUTTON(w) )
  {
    field_write_int(e->field, e->size,
        (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)));
  }
  else if( GTK_IS_RANGE(w) )
  {
    if( e->size == sizeof(float) )
      field_write_float(e->field, (float)gtk_range_get_value(GTK_RANGE(w)));
    else
      field_write_double(e->field, gtk_range_get_value(GTK_RANGE(w)));
  }
  else if( GTK_IS_TOGGLE_BUTTON(w) )
  {
    field_write_int(e->field, e->size,
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) ? 1 : 0);
  }
  else
  {
    BUG("config_apply_entry: unknown widget type for '%s'\n", e->widget_id);
    return;
  }

  /* Invoke post-apply hook when value changed */
  if( e->post_apply != NULL && memcmp(&old_val, e->field, e->size) != 0 )
    e->post_apply();
}

/*------------------------------------------------------------------------*/

/** config_sync_entry - Write one rc_config field into its widget
 * @e: dispatch entry describing the field, widget, and default
 *
 * Uses GTK_IS_* runtime introspection to determine widget type.
 * For radio buttons, activates the entry whose def.i matches the field.
 */
static void
config_sync_entry(const config_default_t *e)
{
  GtkWidget *w;

  if( e->widget_id == NULL )
    return;

  w = Builder_Get_Object(render_settings_builder, e->widget_id);
  if( w == NULL )
  {
    BUG("config_sync_entry: missing widget '%s'\n", e->widget_id);
    return;
  }

  if( GTK_IS_RADIO_BUTTON(w) )
  {
    /* Activate only when field matches this entry's enum value */
    if( field_read_int(e->field, e->size) == e->def.i )
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
  }
  else if( GTK_IS_SPIN_BUTTON(w) )
  {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
        (gdouble)field_read_int(e->field, e->size));
  }
  else if( GTK_IS_RANGE(w) )
  {
    if( e->size == sizeof(float) )
      gtk_range_set_value(GTK_RANGE(w), (gdouble)field_read_float(e->field));
    else
      gtk_range_set_value(GTK_RANGE(w), field_read_double(e->field));
  }
  else if( GTK_IS_TOGGLE_BUTTON(w) )
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
        field_read_int(e->field, e->size));
  }
  else
  {
    BUG("config_sync_entry: unknown widget type for '%s'\n", e->widget_id);
  }
}

/*------------------------------------------------------------------------*/

/** config_apply_entries - Apply a batch of entries
 * @entries: array of dispatch entries
 * @count:   number of entries
 */
static void
config_apply_entries(const config_default_t *entries, int count)
{
  int i;

  for( i = 0; i < count; i++ )
    config_apply_entry(&entries[i]);
}

/*------------------------------------------------------------------------*/

/** config_sync_entries - Sync a batch of entries with signal blocking
 * @entries: array of dispatch entries
 * @count:   number of entries
 *
 * Blocks on_render_settings_changed on all widgets in the batch before
 * writing any values, preventing radio group sibling toggles from
 * triggering re-application of stale values mid-sync.
 */
static void
config_sync_entries(const config_default_t *entries, int count)
{
  int i;
  GtkWidget *w;

  /* Block signals on all widgets in the batch */
  for( i = 0; i < count; i++ )
  {
    if( entries[i].widget_id == NULL )
      continue;
    w = Builder_Get_Object(render_settings_builder, entries[i].widget_id);
    if( w != NULL )
      SIGNAL_BLOCK(w, on_render_settings_changed);
  }

  /* Write rc_config values into widgets */
  for( i = 0; i < count; i++ )
    config_sync_entry(&entries[i]);

  /* Unblock signals */
  for( i = 0; i < count; i++ )
  {
    if( entries[i].widget_id == NULL )
      continue;
    w = Builder_Get_Object(render_settings_builder, entries[i].widget_id);
    if( w != NULL )
      SIGNAL_UNBLOCK(w, on_render_settings_changed);
  }
}

/*------------------------------------------------------------------------*/

/** config_apply_tab - Read widgets into rc_config for one tab
 * @tab: which tab to apply
 *
 * Iterates both entries and session arrays.
 */
void
config_apply_tab(settings_tab_t tab)
{
  const config_tab_defaults_t *td = render_tab_defaults[tab];

  config_apply_entries(td->entries, td->count);
  if( td->session_count > 0 )
    config_apply_entries(td->session, td->session_count);
}

/*------------------------------------------------------------------------*/

/** config_sync_tab - Write rc_config values into widgets for one tab
 * @tab: which tab to sync
 *
 * Iterates both entries and session arrays.
 */
void
config_sync_tab(settings_tab_t tab)
{
  /* Settings dialog not yet built; no widgets to sync */
  if( render_settings_builder == NULL )
    return;

  const config_tab_defaults_t *td = render_tab_defaults[tab];

  config_sync_entries(td->entries, td->count);
  if( td->session_count > 0 )
    config_sync_entries(td->session, td->session_count);
}

/*------------------------------------------------------------------------*/

/**
 * config_find_tab_for_widget() - Look up the tab that owns a widget
 * @widget: the GtkWidget that fired a settings signal
 *
 * Iterates the entries and session arrays of each tab in render_tab_defaults,
 * resolving each widget_id via Builder_Get_Object and comparing the pointer.
 * Returns SETTINGS_TAB_COUNT when no owning tab is found.
 */
  settings_tab_t
config_find_tab_for_widget(GtkWidget *widget)
{
  int t, i;

  for( t = 0; t < SETTINGS_TAB_COUNT; t++ )
  {
    const config_tab_defaults_t *td = render_tab_defaults[t];
    int counts[2]                   = { td->count, td->session_count };
    const config_default_t *arrays[2] = { td->entries, td->session };
    int a;

    for( a = 0; a < 2; a++ )
    {
      if( arrays[a] == NULL )
        continue;

      for( i = 0; i < counts[a]; i++ )
      {
        GtkWidget *w;

        if( arrays[a][i].widget_id == NULL )
          continue;

        w = Builder_Get_Object(render_settings_builder, arrays[a][i].widget_id);
        if( w == widget )
          return (settings_tab_t)t;
      }
    }
  }

  return SETTINGS_TAB_COUNT;
}
