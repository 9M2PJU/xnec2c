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

#include "config_hooks.h"
#include "shared.h"
#include "callbacks.h"
#include "rdpattern_ui.h"
#include "structure_ui.h"

#ifdef HAVE_OPENGL
#include "opengl/opengl_structure.h"
#endif

/*------------------------------------------------------------------------*/

void
hook_polarization(void)
{
  Set_Polarization(calc_data.pol_type);
}

/*------------------------------------------------------------------------*/

void
hook_common_projection(void)
{
  opengl_common_projection_sync();
}

/*------------------------------------------------------------------------*/

void
hook_flow_direction(void)
{
  gboolean animatable;

#ifdef HAVE_OPENGL
  opengl_structure_invalidate();
#endif
  Queue_Structure_Redraw();
  Queue_Radiation_Redraw();

  /* Animation produces no visible change for phase-invariant modes;
   * grey out the Animate menu item for those modes. */
  animatable =
    (rc_config.current_flow_visualization_mode == FLOW_DIR_REFERENCE_PHASE ||
     rc_config.current_flow_visualization_mode == FLOW_DIR_LIC ||
     rc_config.current_flow_visualization_mode == FLOW_DIR_WIREFRAME);

  gtk_widget_set_sensitive(
      Builder_Get_Object(main_window_builder, "main_structure_animate"),
      animatable);
}

/*------------------------------------------------------------------------*/

/* Ortho toolbar image entries: builder pointer-to-pointer and image id.
 * The toggle buttons themselves are config_widget elements bound to
 * opengl_orthographic; only the decorative icon swap is hook-owned. */
static const struct
{
  GtkBuilder **builder;
  const gchar  *img_id;
} ortho_toolbars[] = {
  { &main_window_builder,      "main_ortho_image"      },
  { &rdpattern_window_builder, "rdpattern_ortho_image" },
};

void
hook_orthographic(void)
{
  const gchar *icon = rc_config.opengl_orthographic
      ? "/ortho_cube.svg" : "/persp_cube.svg";
  int i;

  for( i = 0; i < (int)(sizeof(ortho_toolbars) / sizeof(ortho_toolbars[0])); i++ )
  {
    GtkWidget *img;

    if( *ortho_toolbars[i].builder == NULL )
      continue;

    img = GTK_WIDGET(gtk_builder_get_object(*ortho_toolbars[i].builder,
        ortho_toolbars[i].img_id));
    if( img != NULL )
      gtk_image_set_from_resource(GTK_IMAGE(img), icon);
  }

  Queue_Structure_Redraw();
  Queue_Radiation_Redraw();
}

/*------------------------------------------------------------------------*/

void
hook_frequency(void)
{
  /* No frequency data loaded yet (eg config_widget_run_hooks() called from
   * Restore_GUI_State() before any NEC2 file is read); mirrors the guard
   * in freq_step_update_ui(). */
  if( save.freq == NULL )
    return;

  if( isFlagSet(FREQ_LOOP_RUNNING) )
    return;

  if( isFlagSet(FREQ_LOOP_INIT) )
    return;

  if( rc_config.freq_apply )
    user_set_frequency(calc_data.fmhz_save);
  else
    freq_display_update(calc_data.fmhz_save);
}

/*------------------------------------------------------------------------*/

void
hook_main_currents(void)
{
  Main_Currents_Togglebutton_Toggled(rc_config.main_currents_togglebutton);
}

void
hook_main_charges(void)
{
  Main_Charges_Togglebutton_Toggled(rc_config.main_charges_togglebutton);
}

/*------------------------------------------------------------------------*/

void
hook_rdpat_e_field(void)
{
  if( rc_config.rdpattern_e_field )
    SetFlag( DRAW_EFIELD );
  else
    ClearFlag( DRAW_EFIELD );
  Set_Window_Labels();
  if( isFlagSet(DRAW_EHFIELD) )
    xnec2_widget_queue_draw( rdpattern_drawingarea, TRUE );
}

void
hook_rdpat_h_field(void)
{
  if( rc_config.rdpattern_h_field )
    SetFlag( DRAW_HFIELD );
  else
    ClearFlag( DRAW_HFIELD );
  Set_Window_Labels();
  if( isFlagSet(DRAW_EHFIELD) )
    xnec2_widget_queue_draw( rdpattern_drawingarea, TRUE );
}

void
hook_rdpat_poynting(void)
{
  if( rc_config.rdpattern_poynting_vector )
    SetFlag( DRAW_POYNTING );
  else
    ClearFlag( DRAW_POYNTING );
  Set_Window_Labels();
  if( isFlagSet(DRAW_EHFIELD) )
    xnec2_widget_queue_draw( rdpattern_drawingarea, TRUE );
}

void
hook_rdpat_overlay(void)
{
  if( rc_config.rdpattern_overlay_structure )
    SetFlag( OVERLAY_STRUCT );
  else
    ClearFlag( OVERLAY_STRUCT );
  xnec2_widget_queue_draw( rdpattern_drawingarea, TRUE );
}

void
hook_rdpat_gradient_key(void)
{
  xnec2_widget_queue_draw( rdpattern_drawingarea, TRUE );
}

void
hook_rdpat_draw_style(void)
{
  Queue_Radiation_Redraw();
}

void
hook_rdpat_gain(void)
{
  Rdpattern_Gain_Togglebutton_Toggled(rc_config.rdpattern_gain_togglebutton);
}

void
hook_rdpat_eh(void)
{
  Rdpattern_EH_Togglebutton_Toggled(rc_config.rdpattern_eh_togglebutton);
}

/*------------------------------------------------------------------------*/

/* freqplots_recount_ngraph - derive the active-plot count from the live
 * PLOT_* select flags.
 *
 * calc_data.ngraph is a derived value; each select hook is called at most
 * once per real change and unconditionally at window-creation time via
 * config_widget_run_hooks(), so recomputing from the flags (rather than
 * incrementing/decrementing per call) keeps every hook idempotent.
 */
static void
freqplots_recount_ngraph(void)
{
  static const unsigned long long select_flags[] = {
    PLOT_GMAX, PLOT_GAIN_DIR, PLOT_GVIEWER, PLOT_VSWR,
    PLOT_ZREAL_ZIMAG, PLOT_ZMAG_ZPHASE, PLOT_SMITH, PLOT_ANT_TEMP,
  };
  int i, n = 0;

  for( i = 0; i < (int)(sizeof(select_flags) / sizeof(select_flags[0])); i++ )
    if( isFlagSet(select_flags[i]) )
      n++;

  calc_data.ngraph = n;
}

/* freqplots_select_changed - apply one PLOT_* select flag from its field
 * @field_active: current rc_config toggle-button field value
 * @flag: the PLOT_* bit this field owns
 *
 * Shared body for the eight freqplots select-toggle hooks.
 */
static void
freqplots_select_changed(int field_active, unsigned long long int flag)
{
  if( field_active )
    SetFlag( flag | PLOT_SELECT );
  else
    ClearFlag( flag );

  freqplots_recount_ngraph();

  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

void hook_freqplots_gmax(void)
{
  freqplots_select_changed(rc_config.freqplots_gmax_togglebutton, PLOT_GMAX);
}

void hook_freqplots_gdir(void)
{
  freqplots_select_changed(rc_config.freqplots_gdir_togglebutton, PLOT_GAIN_DIR);
}

void hook_freqplots_gviewer(void)
{
  freqplots_select_changed(rc_config.freqplots_gviewer_togglebutton, PLOT_GVIEWER);
}

void hook_freqplots_vswr(void)
{
  freqplots_select_changed(rc_config.freqplots_vswr_togglebutton, PLOT_VSWR);
}

void hook_freqplots_zrlzim(void)
{
  freqplots_select_changed(rc_config.freqplots_zrlzim_togglebutton, PLOT_ZREAL_ZIMAG);
}

void hook_freqplots_zmgzph(void)
{
  freqplots_select_changed(rc_config.freqplots_zmgzph_togglebutton, PLOT_ZMAG_ZPHASE);
}

void hook_freqplots_smith(void)
{
  freqplots_select_changed(rc_config.freqplots_smith_togglebutton, PLOT_SMITH);
}

void hook_freqplots_ant_temp(void)
{
  freqplots_select_changed(rc_config.freqplots_ant_temp_togglebutton, PLOT_ANT_TEMP);
}

void
hook_freqplots_net_gain(void)
{
  if( rc_config.freqplots_net_gain )
    SetFlag( PLOT_NETGAIN );
  else
    ClearFlag( PLOT_NETGAIN );

  /* Net gain gates the gain and viewer popups' port pull-downs, so re-gate
   * every open port-aware combo when the setting flips. */
  freqplots_refresh_port_combos();

  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

void
hook_freqplots_min_max(void)
{
  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

void
hook_freqplots_clamp_vswr(void)
{
  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

void
hook_freqplots_show_ant_temp(void)
{
  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

void
hook_freqplots_round_x_axis(void)
{
  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

void
hook_freqplots_swap_click(void)
{
  /* Affects only the interpretation of future clicks; no redraw. */
}

void
hook_freqplots_s11(void)
{
  if( isFlagSet(PLOT_ENABLED) && isFlagSet(FREQ_LOOP_DONE) )
    freqplots_redraw_all(TRUE);
}

/*------------------------------------------------------------------------*/

/* Apply-frequency checkbutton tree: session-only, no persistence row.
 * File-scope storage so the binding registry holds a pointer with static
 * lifetime; an inline compound literal would die when config_hooks_init()
 * returns. */
static const config_widget_tree_t *const freq_apply_tree =
  CONFIG_WIDGET_TREE( .groups = CONFIG_WIDGET_GROUPS(
    CONFIG_WIDGET_GROUP( .builder = &main_window_builder,
      .elements = CONFIG_WIDGETS(
        CONFIG_WIDGET( .widget_id = "main_freq_checkbutton" ), NULL ) ),
    CONFIG_WIDGET_GROUP( .builder = &rdpattern_window_builder,
      .elements = CONFIG_WIDGETS(
        CONFIG_WIDGET( .widget_id = "rdpattern_freq_checkbutton" ), NULL ) ),
    NULL ) );

void
config_hooks_init(void)
{
  config_widget_register( &rc_config.freq_apply, sizeof(rc_config.freq_apply),
    freq_apply_tree );
}
