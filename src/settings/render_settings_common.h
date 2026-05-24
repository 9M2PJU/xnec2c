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

#ifndef RENDER_SETTINGS_COMMON_H
#define RENDER_SETTINGS_COMMON_H 1

#include "../common.h"

/*------------------------------------------------------------------------*/

/* Settings tab indices for dispatch */
typedef enum {
  SETTINGS_TAB_GENERAL = 0,
  SETTINGS_TAB_OPENGL,
  SETTINGS_TAB_CAIRO,
  SETTINGS_TAB_COUNT
} settings_tab_t;

/* Per-field dispatch entry: maps one rc_config field to its widget and default */
typedef struct {
  void        *field;       /* &rc_config.some_field */
  size_t       size;        /* sizeof(rc_config.some_field) */
  const char  *widget_id;   /* glade widget id, or NULL if no widget */
  void       (*post_apply)(void); /* called after field changes; NULL = none */
  union { int i; float f; double d; } def;
  gboolean     is_default;  /* TRUE = reset-default entry for this field */
} config_default_t;

/* Per-tab collection of dispatch entries */
typedef struct {
  const config_default_t *entries;
  int                     count;
  const config_default_t *session;       /* apply/sync only; not reset */
  int                     session_count;
} config_tab_defaults_t;

/* Compile-time type checks for config dispatch macros.
 * CFG_INT uses sizeof (enum types are int-width, harmless for memcpy).
 * CFG_FLT/CFG_DBL use _Generic to catch the dangerous int/float confusion
 * that sizeof alone cannot detect (sizeof(int) == sizeof(float) == 4). */
#define CFG_INT_ASSERT(field) \
  _Static_assert(sizeof(rc_config.field) == sizeof(int), \
      "CFG_INT: " #field " is not int-width")

#define CFG_FLT_ASSERT(field) \
  _Static_assert(_Generic(rc_config.field, float: 1, default: 0), \
      "CFG_FLT: " #field " is not float")

#define CFG_DBL_ASSERT(field) \
  _Static_assert(_Generic(rc_config.field, double: 1, default: 0), \
      "CFG_DBL: " #field " is not double")

/* Table entry macros — type validated at compile time via
 * CFG_{INT,FLT,DBL}_ASSERT placed at file scope in each per-tab module. */
#define CFG_INT(field, wid, hook, val) \
  { &rc_config.field, sizeof(rc_config.field), wid, hook, { .i = (val) }, TRUE }

#define CFG_FLT(field, wid, hook, val) \
  { &rc_config.field, sizeof(rc_config.field), wid, hook, { .f = (val) }, TRUE }

#define CFG_DBL(field, wid, hook, val) \
  { &rc_config.field, sizeof(rc_config.field), wid, hook, { .d = (val) }, TRUE }

/* Radio group sibling: same field, different value, not used for reset */
#define CFG_INT_RADIO(field, wid, hook, val) \
  { &rc_config.field, sizeof(rc_config.field), wid, hook, { .i = (val) }, FALSE }

/*------------------------------------------------------------------------*/

/* Top-level dispatch table indexed by settings_tab_t (pointer array
 * because per-tab instances reside in separate translation units) */
extern const config_tab_defaults_t *render_tab_defaults[SETTINGS_TAB_COUNT];

/** config_reset_tab - Reset one tab's fields to compiled-in defaults
 * @tab: which tab to reset
 *
 * Iterates only the entries array (not session); memcpy each default.
 * Does not invoke post_apply hooks (safe for init-time use before widgets exist).
 */
void config_reset_tab(settings_tab_t tab);

/** config_reset_tab_user - Reset one tab and invoke post_apply hooks
 * @tab: which tab to reset
 *
 * Same as config_reset_tab but invokes post_apply for each entry whose
 * field value changed.  For user-initiated resets only (widgets must exist).
 */
void config_reset_tab_user(settings_tab_t tab);

/** config_reset_all - Reset all tabs to compiled-in defaults */
void config_reset_all(void);

/** config_apply_tab - Read widgets into rc_config for one tab
 * @tab: which tab to apply
 *
 * Iterates both entries and session arrays.  Uses GTK_IS_* introspection
 * to determine widget type and read the appropriate value.  Calls
 * post_apply when the field value changed.
 */
void config_apply_tab(settings_tab_t tab);

/** config_sync_tab - Write rc_config values into widgets for one tab
 * @tab: which tab to sync
 *
 * Iterates both entries and session arrays.  Uses GTK_IS_* introspection
 * to determine widget type and write the appropriate value.
 */
void config_sync_tab(settings_tab_t tab);

/*------------------------------------------------------------------------*/

/**
 * config_find_tab_for_widget() - Look up the settings tab that owns a widget
 * @widget: the GtkWidget that fired a settings signal
 *
 * Returns the settings_tab_t of the owning tab, or SETTINGS_TAB_COUNT when
 * the widget is not found in any tab's dispatch table.
 */
settings_tab_t config_find_tab_for_widget(GtkWidget *widget);

/** render_settings_load_glade - Load all three settings glade resources
 *
 * Creates the builder, loads render_settings.glade, opengl_settings.glade,
 * and cairo_settings.glade, connects signals, and extracts the window widget.
 * Returns TRUE on success; on failure, logs the error and returns FALSE.
 * Caller is responsible for setting render_settings_window on success.
 */
gboolean render_settings_load_glade(void);

#endif /* RENDER_SETTINGS_COMMON_H */
