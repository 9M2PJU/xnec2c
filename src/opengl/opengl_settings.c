/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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
#include "opengl_settings.h"
#include "opengl_structure.h"
#include "opengl_rdpattern.h"
#include "opengl_msaa.h"
#include "../opengl-engine/opengl_view.h"

/* Window and key widgets */
static GtkWidget *opengl_settings_window = NULL;

/* Suppress feedback loop when syncing widgets from rc_config */
static gboolean syncing = FALSE;

/*------------------------------------------------------------------------*/

/* Unified radio entry: widget ID and integer value */
typedef struct
{
  gchar *id;
  int value;
} radio_entry_t;

/* MSAA radio button widget IDs indexed by sample count */
static const radio_entry_t msaa_radios[] = {
  { "radio_msaa_off", MSAA_OFF },
  { "radio_msaa_2x",  MSAA_2X },
  { "radio_msaa_4x",  MSAA_4X },
  { "radio_msaa_8x",  MSAA_8X },
  { "radio_msaa_16x", MSAA_16X },
};

/* Draw style radio button widget IDs indexed by style */
static const radio_entry_t style_radios[] = {
  { "radio_style_surface",   RDPAT_STYLE_SURFACE },
  { "radio_style_wireframe", RDPAT_STYLE_WIREFRAME },
  { "radio_style_both",      RDPAT_STYLE_BOTH },
};

/* Per-type brightness/transparency slider to rc_config field mapping.
 * Offsets resolve at compile time since rc_config is a global. */
static const struct
{
  gchar *bright_id;
  gchar *trans_id;
  size_t bright_off;
  size_t trans_off;
} type_slider_map[] = {
  { "scale_bright_segments", "scale_trans_segments",
    offsetof(rc_config_t, brightness_segments),
    offsetof(rc_config_t, transparency_segments) },
  { "scale_bright_patches", "scale_trans_patches",
    offsetof(rc_config_t, brightness_patches),
    offsetof(rc_config_t, transparency_patches) },
  { "scale_bright_rdpat_surface", "scale_trans_rdpat_surface",
    offsetof(rc_config_t, brightness_rdpat_surface),
    offsetof(rc_config_t, transparency_rdpat_surface) },
  { "scale_bright_rdpat_wire", "scale_trans_rdpat_wire",
    offsetof(rc_config_t, brightness_rdpat_wire),
    offsetof(rc_config_t, transparency_rdpat_wire) },
  { "scale_bright_nearfield", "scale_trans_nearfield",
    offsetof(rc_config_t, brightness_nearfield),
    offsetof(rc_config_t, transparency_nearfield) },
  { "scale_bright_ground_plane", "scale_trans_ground_plane",
    offsetof(rc_config_t, brightness_ground_plane),
    offsetof(rc_config_t, transparency_ground_plane) },
  { "scale_bright_axes", "scale_trans_axes",
    offsetof(rc_config_t, brightness_axes),
    offsetof(rc_config_t, transparency_axes) },
};

#define TYPE_SLIDER_COUNT \
  ((int)(sizeof(type_slider_map) / sizeof(type_slider_map[0])))

/*------------------------------------------------------------------------*/

/** apply_radio_group - Find active radio in a group, return its value
 * @entries: radio entry table
 * @count: number of entries
 *
 * Returns the value of the active radio button, or -1 if none found.
 */
static int
apply_radio_group(const radio_entry_t *entries, int count)
{
  int i;

  for( i = 0; i < count; i++ )
  {
    GtkWidget *r = Builder_Get_Object(opengl_settings_builder,
        entries[i].id);
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r)) )
      return entries[i].value;
  }

  return -1;
}

/** sync_radio_group - Set active radio from rc_config value
 * @entries: radio entry table
 * @count: number of entries
 * @current: current value to match
 */
static void
sync_radio_group(const radio_entry_t *entries, int count, int current)
{
  int i;

  for( i = 0; i < count; i++ )
  {
    if( entries[i].value == current )
    {
      GtkWidget *w = Builder_Get_Object(opengl_settings_builder,
          entries[i].id);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      return;
    }
  }
}

/*------------------------------------------------------------------------*/

/** opengl_settings_reset_defaults - Reset all OpenGL settings to defaults
 *
 * Restores brightness, transparency, cylinder scale, MSAA, draw style,
 * and toggle states to their compiled-in defaults via shared function.
 * Syncs widgets and triggers redraws.
 */
static void
opengl_settings_reset_defaults(void)
{
  opengl_config_set_defaults();

  /* OpenGL renderer and rotation stay at current values —
   * resetting renderer mid-session would swap widgets unexpectedly */

  /* Push cylinder scale to renderer */
  opengl_structure_set_radius_scale(rc_config.opengl_cylinder_radius_scale);

  /* Apply MSAA to GL views */
  Set_MSAA_Samples(rc_config.opengl_msaa_samples);

  /* Sync widgets and redraw */
  opengl_settings_sync_from_config();
  opengl_structure_invalidate();
  opengl_structure_queue_draw();
  opengl_rdpattern_queue_draw();
}

/*------------------------------------------------------------------------*/

/** on_opengl_settings_changed - Unified signal handler for all settings widgets
 *
 * Reads all widget states into rc_config and queues redraws.
 * Suppressed during sync_from_config to prevent feedback loops.
 */
void
on_opengl_settings_changed(GtkWidget *widget, gpointer user_data)
{
  GtkWidget *chk;
  int val;

  (void)widget;
  (void)user_data;

  if( syncing )
    return;

  /* OpenGL renderer toggle — only call when value changes (widget swap) */
  chk = Builder_Get_Object(opengl_settings_builder, "chk_opengl_renderer");
  {
    gboolean new_renderer =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk));
    if( new_renderer != (gboolean)rc_config.use_opengl_renderer )
      opengl_set_renderer(new_renderer);
  }

  /* Constrained rotation — only call when value changes (arcball mode) */
  chk = Builder_Get_Object(opengl_settings_builder,
      "chk_constrained_rotation");
  {
    gboolean new_constrained =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk));
    if( new_constrained != (gboolean)rc_config.arcball_constrained_rotation )
      opengl_set_constrained_rotation(new_constrained);
  }

  /* Transparency on-click toggle */
  chk = Builder_Get_Object(opengl_settings_builder, "chk_only_on_click");
  rc_config.opengl_transparent_on_click =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk)) ? 1 : 0;

  /* Per-type brightness/transparency sliders */
  {
    GtkWidget *w;
    int i;

    for( i = 0; i < TYPE_SLIDER_COUNT; i++ )
    {
      w = Builder_Get_Object(opengl_settings_builder,
          type_slider_map[i].bright_id);
      *(float *)((char *)&rc_config + type_slider_map[i].bright_off) =
          (float)gtk_range_get_value(GTK_RANGE(w));

      w = Builder_Get_Object(opengl_settings_builder,
          type_slider_map[i].trans_id);
      *(float *)((char *)&rc_config + type_slider_map[i].trans_off) =
          (float)gtk_range_get_value(GTK_RANGE(w));
    }
  }

  /* Cylinder scale slider */
  {
    GtkWidget *scale;
    double cyl_val;

    scale = Builder_Get_Object(opengl_settings_builder,
        "scale_cylinder_scale");
    cyl_val = gtk_range_get_value(GTK_RANGE(scale));
    opengl_structure_set_radius_scale(cyl_val);
  }

  /* MSAA radio group */
  val = apply_radio_group(msaa_radios,
      (int)(sizeof(msaa_radios) / sizeof(msaa_radios[0])));
  if( val >= 0 )
    Set_MSAA_Samples(val);

  /* Draw style radio group */
  val = apply_radio_group(style_radios,
      (int)(sizeof(style_radios) / sizeof(style_radios[0])));
  if( val >= 0 )
    rc_config.rdpattern_draw_style = val;

  /* Redraw all GL views — no invalidate needed for brightness/transparency
   * since visual params are read from rc_config each frame.  Cylinder scale
   * and draw style changes are detected by their own staleness checks. */
  opengl_structure_queue_draw();
  opengl_rdpattern_queue_draw();
}

/*------------------------------------------------------------------------*/

/** opengl_settings_sync_from_config - Populate all widgets from rc_config
 *
 * Called after window creation and when external code changes rc_config
 * (e.g., menu item handlers).
 */
void
opengl_settings_sync_from_config(void)
{
  GtkWidget *w;
  int i;

  if( opengl_settings_builder == NULL )
    return;

  syncing = TRUE;

  /* Checkbuttons */
  w = Builder_Get_Object(opengl_settings_builder, "chk_opengl_renderer");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
      rc_config.use_opengl_renderer);

  w = Builder_Get_Object(opengl_settings_builder,
      "chk_constrained_rotation");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
      rc_config.arcball_constrained_rotation);

  w = Builder_Get_Object(opengl_settings_builder, "chk_only_on_click");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
      rc_config.opengl_transparent_on_click);

  /* Cylinder scale slider */
  w = Builder_Get_Object(opengl_settings_builder, "scale_cylinder_scale");
  gtk_range_set_value(GTK_RANGE(w),
      rc_config.opengl_cylinder_radius_scale);

  /* Per-type brightness/transparency sliders */
  for( i = 0; i < TYPE_SLIDER_COUNT; i++ )
  {
    w = Builder_Get_Object(opengl_settings_builder,
        type_slider_map[i].bright_id);
    gtk_range_set_value(GTK_RANGE(w),
        *(float *)((char *)&rc_config + type_slider_map[i].bright_off));

    w = Builder_Get_Object(opengl_settings_builder,
        type_slider_map[i].trans_id);
    gtk_range_set_value(GTK_RANGE(w),
        *(float *)((char *)&rc_config + type_slider_map[i].trans_off));
  }

  /* Radio groups */
  sync_radio_group(msaa_radios,
      (int)(sizeof(msaa_radios) / sizeof(msaa_radios[0])),
      rc_config.opengl_msaa_samples);
  sync_radio_group(style_radios,
      (int)(sizeof(style_radios) / sizeof(style_radios[0])),
      rc_config.rdpattern_draw_style);

  syncing = FALSE;
}

/*------------------------------------------------------------------------*/

/** opengl_settings_init - Create builder, load glade, wire signals */
gboolean
opengl_settings_init(void)
{
  GError *gerror = NULL;

  if( opengl_settings_window != NULL )
    return TRUE;

  opengl_settings_builder = gtk_builder_new();
  if( !gtk_builder_add_from_resource(opengl_settings_builder,
        "/opengl_settings.glade", &gerror) )
  {
    pr_err("opengl_settings_init: failed to load glade: %s\n",
        gerror->message);
    g_error_free(gerror);
    return FALSE;
  }

  gtk_builder_connect_signals(opengl_settings_builder, NULL);

  opengl_settings_window = GTK_WIDGET(
      gtk_builder_get_object(opengl_settings_builder,
        "opengl_settings_window"));
  if( opengl_settings_window == NULL )
  {
    pr_err("opengl_settings_init: "
        "failed to get opengl_settings_window from glade\n");
    return FALSE;
  }

  opengl_settings_sync_from_config();

  return TRUE;
}

/*------------------------------------------------------------------------*/

/** opengl_settings_show - Show the settings window, init if needed */
void
opengl_settings_show(void)
{
  if( !opengl_settings_init() )
    return;

  opengl_settings_sync_from_config();
  gtk_widget_show_all(opengl_settings_window);
  gtk_window_present(GTK_WINDOW(opengl_settings_window));
}

/*------------------------------------------------------------------------*/

/** opengl_settings_hide - Hide the settings window */
void
opengl_settings_hide(void)
{
  if( opengl_settings_window != NULL )
    gtk_widget_hide(opengl_settings_window);
}

/*------------------------------------------------------------------------*/

/* Signal handler: menu item activate */
void
on_opengl_settings_show_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void)menuitem;
  (void)user_data;

  opengl_settings_show();
}

/*------------------------------------------------------------------------*/

/* Signal handler: Close button */
void
on_opengl_settings_close_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  opengl_settings_hide();
}

/*------------------------------------------------------------------------*/

/* Signal handler: Reset to Defaults button */
void
on_opengl_settings_reset_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  (void)user_data;

  opengl_settings_reset_defaults();
}

/*------------------------------------------------------------------------*/

/* Signal handler: window close (X button) */
gboolean
on_opengl_settings_window_delete_event(
    GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;

  opengl_settings_hide();
  return TRUE;
}

/*------------------------------------------------------------------------*/

/* Signal handler: window destroy */
void
on_opengl_settings_window_destroy(GtkWidget *widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;

  if( opengl_settings_builder != NULL )
  {
    g_object_unref(opengl_settings_builder);
    opengl_settings_builder = NULL;
  }

  opengl_settings_window = NULL;
}

#else /* !HAVE_OPENGL */
/* Avoid empty translation unit warning */
typedef int opengl_settings_dummy;
#endif /* HAVE_OPENGL */
