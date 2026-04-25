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

#include "opengl_structure.h"
#include "opengl_rdpattern.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

#include "opengl_structure_geometry.h"
#include "opengl_settings.h"
#include "../opengl-engine/opengl_view.h"
#include "../opengl-engine/opengl_view_notice.h"

/* Maximum allowed radius scale */
#define CYLINDER_RADIUS_SCALE_MAX 100.0

/* Mutable cylinder radius scale factor (user-adjustable via Ctrl+scroll) */
static double cylinder_radius_scale = 1.0;


/* Widget pointer for queue_redraw */
static GtkWidget *structure_gl_widget = NULL;

/* Vertex attribute layout for lit-color shader (shared with overlay consumers) */
const gl_vertex_attrib_t opengl_structure_attribs[3] = {
  { "position", 3, 0 },
  { "normal",   3, 4 * (int)sizeof(float) },
  { "color",    4, 8 * (int)sizeof(float) }
};

/* Vertex attribute layout for chevron shader (structure_vertex_t) */
const gl_vertex_attrib_t opengl_chevron_attribs[7] = {
  { "position",   3, 0 },
  { "normal",     3, 4 * (int)sizeof(float) },
  { "color",      4, 8 * (int)sizeof(float) },
  { "uv",         2, 12 * (int)sizeof(float) },
  { "flow_data",  4, 14 * (int)sizeof(float) },
  { "tangent1",   3, 18 * (int)sizeof(float) },
  { "tangent2",   3, 21 * (int)sizeof(float) }
};

/*-----------------------------------------------------------------------*/

/** opengl_structure_get_radius_scale() - Return current cylinder radius display scale factor
 */
  double
opengl_structure_get_radius_scale(void)
{
  return( cylinder_radius_scale );

} /* opengl_structure_get_radius_scale() */

/*-----------------------------------------------------------------------*/

/** opengl_structure_set_radius_scale() - Set cylinder radius display scale factor
 * @scale: new scale value, clamped to [0, CYLINDER_RADIUS_SCALE_MAX]
 *
 * Syncs to rc_config; geometry regenerates on next render.
 */
  void
opengl_structure_set_radius_scale(double scale)
{
  if( scale < 0.0 )
  {
    scale = 0.0;
  }

  if( scale > CYLINDER_RADIUS_SCALE_MAX )
  {
    scale = CYLINDER_RADIUS_SCALE_MAX;
  }

  cylinder_radius_scale = scale;
  rc_config.opengl_cylinder_radius_scale = scale;

} /* opengl_structure_set_radius_scale() */

/*-----------------------------------------------------------------------*/

/** opengl_structure_on_ctrl_scroll() - Ctrl+scroll handler for adjusting cylinder radius scale
 * @widget: source scroll widget
 * @event: scroll event
 * @view_state: pointer to gl_view_state_t
 *
 * Shared by structure and rdpattern scene providers.
 */
  gboolean
opengl_structure_on_ctrl_scroll(
    GtkWidget *widget, GdkEventScroll *event, gpointer view_state)
{
  gl_view_state_t *state;
  double scale, new_scale;

  state = (gl_view_state_t *)view_state;

  if( !state )
    return( FALSE );

  /* Compute increment matching zoom scroll feel */
  scale = compute_zoom_scale(
      (int)(state->viewport_height * state->aspect),
      (int)state->viewport_height,
      (cylinder_radius_scale > 0.1 ? cylinder_radius_scale : 0.1) * 100.0);

  new_scale = cylinder_radius_scale;

  if( event->direction == GDK_SCROLL_UP )
  {
    /* Scroll up: thicker. If at zero, jump to threshold. */
    if( new_scale < CYLINDER_SCALE_LINE_THRESHOLD )
    {
      new_scale = CYLINDER_SCALE_LINE_THRESHOLD;
    }
    else
    {
      new_scale *= (1.0 + 0.1 * scale);
    }
  }
  else if( event->direction == GDK_SCROLL_DOWN )
  {
    /* Scroll down: thinner. Below threshold snaps to zero (line mode). */
    new_scale /= (1.0 + 0.1 * scale);

    if( new_scale < CYLINDER_SCALE_LINE_THRESHOLD )
    {
      new_scale = 0.0;
    }
  }
  else
  {
    return( FALSE );
  }

  opengl_structure_set_radius_scale(new_scale);
  opengl_settings_sync_from_config();

  /* Queue redraw on the event source widget */
  xnec2_widget_queue_draw(widget, TRUE);

  /* Queue redraw on cross-window GL areas */
  if( structure_gl_widget && widget != structure_gl_widget )
    xnec2_widget_queue_draw(structure_gl_widget, TRUE);

  {
    GtkWidget *rdpat_w = opengl_rdpattern_get_widget();
    if( rdpat_w && widget != rdpat_w )
      xnec2_widget_queue_draw(rdpat_w, TRUE);
  }

  return( TRUE );

} /* opengl_structure_on_ctrl_scroll() */

/*-----------------------------------------------------------------------*/

/** opengl_structure_show_ctrl_notice() - Show ctrl+scroll notice once per session
 * @widget: GL area widget to display notice on
 *
 * Shared guard across all views so the message appears only once
 * regardless of which view renders first.
 */
  void
opengl_structure_show_ctrl_notice(GtkWidget *widget)
{
  static gboolean shown = FALSE;

  if( shown || !widget )
    return;

  gl_view_show_notice(widget, "Ctrl+Scroll: Wire Radius",
      2500, GL_NOTICE_BOTTOM_LEFT);
  shown = TRUE;

} /* opengl_structure_show_ctrl_notice() */

/*-----------------------------------------------------------------------*/

/** structure_scene_generate() - Scene provider generate callback
 * @out: view content to populate
 *
 * Delegates to shared geometry updater, then fills view content.
 */
  static gboolean
structure_scene_generate(gl_view_content_t *out)
{
  g_rec_mutex_lock(&freq_data_lock);

  const structure_overlay_data_t *geom;
  float zoom;

  opengl_structure_show_ctrl_notice(structure_gl_widget);

  opengl_structure_update_shared_geometry();
  geom = opengl_structure_get_shared_geometry();

  /* Zoom from view->zoom; view_set_zoom() is the authoritative writer. */
  zoom = (structure_view != NULL) ? structure_view->zoom : 1.0f;

  if( zoom < 0.01f )
    zoom = 0.01f;

  memcpy(out->batches, geom->batches,
      (size_t)geom->batch_count * sizeof(geom->batches[0]));
  out->batch_count = geom->batch_count;
  out->vertex_stride = geom->vertex_stride;
  out->r_max = (geom->batch_count > 0) ? geom->view_scale : 1.5f;
  out->clip_extent = out->r_max;
  out->zoom = zoom;
  out->model_scale = 1.0f;
  out->show_gradient = FALSE;
  out->generation = geom->generation;

  /* Prompt user to open a file when no geometry is loaded */
  if( geom->batch_count == 0 )
    out->status_message = "File ▸ Open to load an NEC file";

  g_rec_mutex_unlock(&freq_data_lock);
  return( TRUE );
}

/*-----------------------------------------------------------------------*/

/** structure_scene_cleanup() - Scene provider cleanup callback
 */
  static void
structure_scene_cleanup(void)
{
  opengl_structure_geometry_cleanup();
}

/*-----------------------------------------------------------------------*/

/** structure_scene_post_render() - Scene provider post-render callback
 *
 * Updates structure-window UI readouts (viewer gain, frequency step)
 * after each rendered frame, matching the Cairo draw path via the
 * shared Draw_Structure_UI() helper.
 */
  static void
structure_scene_post_render(void)
{
  g_rec_mutex_lock(&freq_data_lock);
  Draw_Structure_UI();
  g_rec_mutex_unlock(&freq_data_lock);

} /* structure_scene_post_render() */

/*-----------------------------------------------------------------------*/

/* Static scene configuration and provider */
static gl_view_config_t structure_view_config = {
  .vertex_shader_path = "/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "/gl/lit-color-fragment.glsl",
  .attribs = opengl_chevron_attribs,
  .attrib_count = 7,
  .vertex_stride = (int)sizeof(structure_vertex_t),
  .has_gradient = FALSE,
  .gradient_draw = NULL,
  .on_gl_init_failed = opengl_gl_init_failed
};

/** opengl_structure_ground_plane_is_active() - Report whether ground plane should render
 * @_ctx: unused context pointer
 *
 * Ground plane is visible when a real ground is defined and enabled,
 * matching the NEC2 perfect/real ground condition used during calculation.
 */
  static gboolean
opengl_structure_ground_plane_is_active(void *_ctx)
{
  (void)_ctx;

  return( gnd.ksymp == 2 && gnd.iperf >= 0 );

} /* opengl_structure_ground_plane_is_active() */

/*-----------------------------------------------------------------------*/

static gl_scene_provider_t structure_scene_provider = {
  .generate                 = structure_scene_generate,
  .post_render              = structure_scene_post_render,
  .cleanup                  = structure_scene_cleanup,
  .overlay_config           = NULL,
  .overlay_generate         = NULL,
  .overlay_cleanup          = NULL,
  .on_ctrl_scroll           = opengl_structure_on_ctrl_scroll,
  .ground_plane_is_active   = opengl_structure_ground_plane_is_active
};

/*-----------------------------------------------------------------------*/

/** structure_view_changed_cb() - view_t change callback for structure view
 * @v:           view that changed (structure_view)
 * @_user_data:  unused
 *
 * Invoked by view_notify_change() whenever rotation, pan, zoom or
 * extent changes.  Queues redraws on the Cairo path (Queue_Structure_Redraw)
 * and the GL widget, and refreshes the WR/WI spin display.  Propagation
 * to the rdpattern view under common-projection sharing is handled
 * internally by view_t via the rotation_follower link established by
 * view_share_master().  Bound as changed_cb at view_new() in main.c.
 */
  void
structure_view_changed_cb(view_t *v, gpointer _user_data)
{
  (void)_user_data;

  view_update_spin_display( v );
  Queue_Structure_Redraw();

  if( structure_gl_widget )
    xnec2_widget_queue_draw( structure_gl_widget, TRUE );

} /* structure_view_changed_cb() */

/*-----------------------------------------------------------------------*/

/** opengl_structure_create_widget_impl() - Create the OpenGL structure widget using the generic view engine
 *
 * Returns existing widget if already created.
 */
  static GtkWidget*
opengl_structure_create_widget_impl(void)
{
  /* Return existing widget if already created */
  if( structure_gl_widget )
    return( structure_gl_widget );

  /* Load persisted radius scale from config; zero means line mode */
  cylinder_radius_scale = rc_config.opengl_cylinder_radius_scale;

  if( structure_view == NULL )
    return( NULL );

  structure_gl_widget = gl_view_create_widget(
      &structure_view_config,
      &structure_scene_provider,
      structure_view );

  return( structure_gl_widget );
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_get_widget() - Return the structure GL widget
 */
  GtkWidget*
opengl_structure_get_widget(void)
{
  return( structure_gl_widget );
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_cleanup_impl() - Cleanup structure GL resources
 */
  static void
opengl_structure_cleanup_impl(void)
{
  structure_scene_cleanup();
  structure_gl_widget = NULL;
}

#endif /* HAVE_OPENGL */

/*-----------------------------------------------------------------------*/

/** set_view_flow_phase() - Set flow phase on a GL widget and queue redraw
 * @w: GL widget (NULL-safe, returns silently)
 * @phase: absolute phase in radians
 */
  static void
set_view_flow_phase(GtkWidget *w, float phase)
{
  gl_view_state_t *state;

  if( !w )
    return;

  state = gl_view_get_state(w);
  if( state )
    state->flow_phase = phase;

  xnec2_widget_queue_draw(w, TRUE);
}

/*-----------------------------------------------------------------------*/

/** advance_view_flow_phase() - Advance flow phase with 2-pi wrap
 * @w: GL widget (NULL-safe, returns 0)
 * @step: phase increment in radians
 *
 * Returns new phase value.  Does not queue redraw; caller controls
 * draw ordering relative to line-mode geometry invalidation.
 */
  static float
advance_view_flow_phase(GtkWidget *w, float step)
{
  gl_view_state_t *state;

  if( !w )
    return( 0.0f );

  state = gl_view_get_state(w);
  if( !state )
    return( 0.0f );

  state->flow_phase += step;
  if( state->flow_phase > (float)M_2PI )
    state->flow_phase -= (float)M_2PI;

  return( state->flow_phase );
}

/*-----------------------------------------------------------------------*/

/** Animate_Flow_Phase() - Timeout callback advancing patch current flow phase
 * @udata: unused
 *
 * Advances flow_phase on both structure and rdpattern GL views by
 * near_field.anim_step radians per tick. Returns FALSE to stop the
 * timeout when FLOW_ANIMATE is cleared.
 */
  gboolean
Animate_Flow_Phase(gpointer udata)
{

  if( isFlagClear(FLOW_ANIMATE) )
  {
    flow_anim_tag = 0;
    return( FALSE );
  }

  advance_view_flow_phase(
      structure_gl_widget, (float)near_field.anim_step);

  if( structure_gl_widget )
    xnec2_widget_queue_draw(structure_gl_widget, TRUE);

  /* Advance phase on rdpattern overlay view */
  {
    GtkWidget *rdpat_w = opengl_rdpattern_get_widget();

    advance_view_flow_phase(rdpat_w, (float)near_field.anim_step);
    if( rdpat_w )
      xnec2_widget_queue_draw(rdpat_w, TRUE);
  }

  return( TRUE );

} /* Animate_Flow_Phase() */

/*-----------------------------------------------------------------------*/

/** opengl_structure_reset_flow_phase() - Reset flow phase to zero on all views
 *
 * Called when flow animation stops (cancel/destroy) to return arrows
 * and chevrons to the static reference-phase direction.
 */
  void
opengl_structure_reset_flow_phase(void)
{
#ifdef HAVE_OPENGL
  set_view_flow_phase(structure_gl_widget, 0.0f);
  set_view_flow_phase(opengl_rdpattern_get_widget(), 0.0f);
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_create_widget() - Public API: create structure GL widget
 */
  GtkWidget*
opengl_structure_create_widget(void)
{
#ifdef HAVE_OPENGL
  return( opengl_structure_create_widget_impl() );
#else
  return( NULL );
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_cleanup() - Public API: cleanup structure resources
 */
  void
opengl_structure_cleanup(void)
{
#ifdef HAVE_OPENGL
  opengl_structure_cleanup_impl();
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_invalidate() - Public API: mark cached geometry stale
 *
 * Forces regeneration from current NEC2 data arrays on next render.
 * Call after geometry reload.
 */
  void
opengl_structure_invalidate(void)
{
#ifdef HAVE_OPENGL
  opengl_structure_geometry_invalidate();
#endif
}

/*-----------------------------------------------------------------------*/

/** opengl_structure_queue_draw() - Public API: queue redraw of OpenGL structure widget
 */
  void
opengl_structure_queue_draw(void)
{
#ifdef HAVE_OPENGL
  if( structure_gl_widget != NULL )
    xnec2_widget_queue_draw( structure_gl_widget, TRUE );
#endif
}
