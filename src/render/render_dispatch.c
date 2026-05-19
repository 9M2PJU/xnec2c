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
 * render_dispatch: unified precondition checking, mode selection, and dispatch.
 *
 * Single entry point render() acquires freq_data_lock, evaluates all content
 * flags via render_check(), and calls through the render_ops_t vtable.  Backends
 * are leaf functions: zero flag evaluation, zero lock management.
 */

#include "render_dispatch.h"
#include "gradient_cache.h"
#include "../shared.h"
#include "../cairo/cairo_draw.h"
#include "../prerender/prerender_farfield.h"
#include "../rdpattern_ui.h"
#include "../structure_ui.h"

/* Last render_check result for the rdpattern view; updated by render() on every call */
static render_check_result_t last_rdpat_check;

/**
 * render_get_last_rdpat_check() - Return cached rdpattern precondition result
 *
 * Returns a pointer to the result of the most recent render_check() call made
 * for VIEW_RDPATTERN.  Consumers (overlay provider, shift-scroll handler, axes
 * callback) read overlay_active and mode from this cache instead of re-evaluating
 * content-selection flags.  Valid after the first rdpattern render() call.
 */
const render_check_result_t *
render_get_last_rdpat_check(void)
{
  return &last_rdpat_check;
}

/* Cairo backend operations vtable for structure window */
const render_ops_t cairo_struct_ops =
{
  .draw_farfield        = NULL,
  .draw_nearfield       = NULL,
  .draw_structure       = cairo_draw_structure,
  .draw_axes            = cairo_draw_axes,
  .init_empty           = cairo_init_empty,
  .set_status           = cairo_set_status,
  .set_gradient         = cairo_set_gradient,
};

/* Cairo backend operations vtable for radiation pattern window */
const render_ops_t cairo_rdpat_ops =
{
  .draw_farfield        = cairo_draw_farfield,
  .draw_nearfield       = cairo_draw_nearfield,
  .draw_structure       = cairo_draw_structure,
  .draw_axes            = cairo_draw_axes,
  .init_empty           = cairo_init_empty,
  .set_status           = cairo_set_status,
  .set_gradient         = cairo_set_gradient,
};

/*-----------------------------------------------------------------------*/

  static const char *
render_rdpattern_mode_message(void)
{
  gboolean has_rp = isFlagSet(ENABLE_RDPAT);
  gboolean has_nf = isFlagSet(ENABLE_NEAREH);

  if( !has_rp && !has_nf )
    return STATUS_MSG_NO_RP_NO_NEAREH;

  if( !has_rp )
    return STATUS_MSG_SELECT_NEARFIELD;

  if( !has_nf )
    return STATUS_MSG_SELECT_GAINPAT;

  return STATUS_MSG_SELECT_MODE;
}

/*-----------------------------------------------------------------------*/

/** render_check_nearfield() - Resolve near-field preconditions
 * @r: result struct with fstep already set; populated on return
 *
 * DRAW_EHFIELD is already confirmed set by caller.
 */
  static void
render_check_nearfield(render_check_result_t *r)
{
  if( isFlagSet(ENABLE_NEAREH) && NF_FSTEP_AVAILABLE(r->fstep) )
  {
    r->mode = RENDER_MODE_NEARFIELD;
    return;
  }

  if( isFlagSet(SUPPRESS_INTERMEDIATE_REDRAWS) )
  {
    r->status = RENDER_SUPPRESS;
    return;
  }

  if( isFlagClear(ENABLE_NEAREH) )
  {
    r->status = RENDER_NO_NF_CARD;
    r->message = STATUS_MSG_NO_NEAREH_CARDS;
  }
  else
  {
    r->status = RENDER_NF_NOT_READY;
    r->message = STATUS_MSG_START_FREQLOOP;
  }
}

/*-----------------------------------------------------------------------*/

/** render_check_farfield() - Resolve far-field preconditions
 * @r: result struct with fstep already set; populated on return
 *
 * DRAW_GAIN is already confirmed set by caller.
 */
  static void
render_check_farfield(render_check_result_t *r)
{
  if( isFlagClear(ENABLE_RDPAT) )
  {
    r->status = RENDER_NO_RP_CARD;
    r->message = STATUS_MSG_NO_RP_CARD;
    return;
  }

  if( r->fstep < 0 )
  {
    r->status = RENDER_NO_DATA;
    r->message = STATUS_MSG_NO_RDPAT_DATA;
    return;
  }

  r->mode = RENDER_MODE_FARFIELD;
}

/*-----------------------------------------------------------------------*/

/** render_check_rdpattern() - Resolve radiation pattern mode and preconditions
 * @r: result struct with fstep already set; populated on return
 *
 * Near-field takes priority over far-field.
 */
  static void
render_check_rdpattern(render_check_result_t *r)
{
  if( isFlagSet(DRAW_EHFIELD) )
    render_check_nearfield(r);
  else if( isFlagSet(DRAW_GAIN) )
    render_check_farfield(r);
  else
  {
    r->status = RENDER_NO_MODE;
    r->message = render_rdpattern_mode_message();
  }

  r->overlay_active = isFlagSet(OVERLAY_STRUCT);
}

/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

  render_check_result_t
render_check(view_type_t view_type)
{
  render_check_result_t r = { .status = RENDER_OK, .mode = RENDER_MODE_NONE,
    .fstep = -1, .message = NULL, .overlay_active = FALSE };

  /* No content available while input file is being parsed */
  if( isFlagSet(INPUT_PENDING) )
  {
    r.status = RENDER_NO_GEOMETRY;
    r.message = STATUS_MSG_OPEN_FILE;
    return r;
  }

  r.fstep = calc_data.freq_step;

  if( view_type == VIEW_STRUCTURE )
  {
    if( data.n == 0 && data.m == 0 )
    {
      r.status = RENDER_NO_GEOMETRY;
      r.message = STATUS_MSG_OPEN_FILE;
      return r;
    }
    r.mode = RENDER_MODE_STRUCTURE;
    return r;
  }

  render_check_rdpattern(&r);
  return r;
}

/*-----------------------------------------------------------------------*/

/** build_struct_draw_params() - Resolve structure draw colors from current flags
 * @fstep: frequency step index
 *
 * Selects wire_colors and patch_colors from precomputed struct_colors
 * based on DRAW_CURRENTS / DRAW_CHARGES flags, or falls back to
 * geometry-mode seg_rgb / patch_rgb.
 */
  static struct_draw_params_t
build_struct_draw_params(int fstep)
{
  struct_draw_params_t params;
  int fs = fstep;

  if( isFlagSet(DRAW_CURRENTS) && CRNT_FSTEP_AVAILABLE(fs) && struct_colors )
  {
    params.wire_colors  = struct_colors[fs].wire_crnt_rgb;
    params.wire_widths  = seg_width;
    params.patch_colors = struct_colors[fs].patch_crnt_rgb;
    params.cmax = fmax((double)struct_colors[fs].wire_crnt_cmax,
                       (double)struct_colors[fs].patch_crnt_cmax);
    params.show_flow = TRUE;
  }
  else if( isFlagSet(DRAW_CHARGES) && CRNT_FSTEP_AVAILABLE(fs) && struct_colors )
  {
    params.wire_colors  = struct_colors[fs].wire_chrg_rgb;
    params.wire_widths  = seg_width;
    params.patch_colors = patch_rgb;
    params.cmax = (double)struct_colors[fs].wire_chrg_cmax;
    params.show_flow = FALSE;
  }
  else
  {
    params.wire_colors  = seg_rgb;
    params.wire_widths  = seg_width;
    params.patch_colors = patch_rgb;
    params.cmax = 0.0;
    params.show_flow = FALSE;
  }

  params.fstep = fs;
  return params;
}

/*-----------------------------------------------------------------------*/

  gboolean
render(void *ctx, const render_ops_t *ops, view_t *view)
{
  render_check_result_t r;
  gboolean ok;

  if( view == NULL )
    return FALSE;

  if( isFlagSet(ERROR_CONDX) )
    return FALSE;

  g_rec_mutex_lock(&freq_data_lock);

  r = render_check(view->type);

  /* r is immutable past this point; cache for external consumers */
  if( view->type == VIEW_RDPATTERN )
    last_rdpat_check = r;

  if( r.status == RENDER_SUPPRESS )
  {
    g_rec_mutex_unlock(&freq_data_lock);
    return FALSE;
  }

  if( r.status != RENDER_OK )
  {
    if( ops->init_empty != NULL )
      ops->init_empty(ctx);
    if( ops->draw_axes != NULL )
      ops->draw_axes(ctx, RENDER_EMPTY_AXIS_EXTENT);
    ops->set_status(ctx, r.message);
    g_rec_mutex_unlock(&freq_data_lock);
    return TRUE;
  }

  switch( r.mode )
  {
    case RENDER_MODE_FARFIELD:
    {
      ff_draw_params_t ff = { .x = 0.0f, .y = 0.0f, .z = 0.0f,
        .pattern_radius = 0.0f, .off_len = 0.0f };

      ff_presentation_recompute(r.fstep);
      ff.pattern_radius = (ff_pre != NULL) ? ff_pre[r.fstep].pattern_radius : 1.0f;

      /* model_scale maps structure-space meters to pattern-space units.
       * Base scale from prerender; interactive scale_adj applied here. */
      float base_scale = (ff_pre != NULL)
          ? ff_pre[r.fstep].overlay_base_scale : 1.0f;
      float model_scale = base_scale
          * (float)rc_config.rdpattern_overlay_scale_adj;

      /* Pre-scale excitation centroid from structure space to pattern space.
       * Zero coordinates produce identity transform when no excitation exists. */
      if( r.overlay_active && isFlagSet(ENABLE_EXCITN) )
      {
        ff.x = (float)geom_pre.excitation_cx * model_scale;
        ff.y = (float)geom_pre.excitation_cy * model_scale;
        ff.z = (float)geom_pre.excitation_cz * model_scale;
        ff.off_len = sqrtf(ff.x * ff.x + ff.y * ff.y + ff.z * ff.z);
      }

      /* overlay_extent: structure-space extent that maps to the same pixel
       * positions as GL's model_scale matrix transform.
       * Derivation: p/R == p*model_scale/pattern_radius -> R = pattern_radius/model_scale */
      float overlay_extent = (model_scale > 0.001f)
          ? ff.pattern_radius / model_scale
          : (float)geom_pre.scene_radius;

      if( ops->draw_axes != NULL )
        ops->draw_axes(ctx, ff.pattern_radius);

      /* Overlay draws behind pattern; pattern lines on top */
      if( r.overlay_active && ops->draw_structure != NULL )
      {
        struct_draw_params_t sparams = build_struct_draw_params(r.fstep);
        ops->draw_structure(ctx, overlay_extent, &sparams);
      }

      ok = ops->draw_farfield(ctx, r.fstep, &ff);

      /* Resolve gradient legend surface for farfield mode and pass
       * directly to the backend via set_gradient; backends composite
       * unconditionally when the surface is non-NULL. */
      if( ok )
      {
        cairo_surface_t *gsfc = gradient_cache_get_overlay(
            view->width, view->height, NULL);
        if( gsfc != NULL )
          ops->set_gradient(ctx, gsfc);
      }

      break;
    }

    case RENDER_MODE_NEARFIELD:
    {
      near_field_t *nf = &near_field_fstep[r.fstep];
      nf_pre_t *np = &nf_pre[r.fstep];
      int npts = fpat.nrx * fpat.nry * fpat.nrz;
      nf_field_set_t fields[NF_FIELD_SETS_MAX];
      int n_fields = 0;
      double dr = geom_pre.nf_dr_norm;

      if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) && np->e_vecs )
      {
        fields[n_fields].vecs = np->e_vecs;
        n_fields++;
      }

      if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) && np->h_vecs )
      {
        fields[n_fields].vecs = np->h_vecs;
        n_fields++;
      }

      if( isFlagSet(DRAW_POYNTING) &&
          (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) &&
          np->pov_vecs )
      {
        fields[n_fields].vecs = np->pov_vecs;
        n_fields++;
      }

      /* Near-field overlay: structure in meters, same space as field vectors */
      float nf_overlay_extent = (float)nf->r_max;

      if( ops->draw_axes != NULL )
        ops->draw_axes(ctx, (float)nf->r_max);

      if( r.overlay_active && ops->draw_structure != NULL )
      {
        struct_draw_params_t sparams = build_struct_draw_params(r.fstep);
        ops->draw_structure(ctx, nf_overlay_extent, &sparams);
      }

      ClearFlag(DRAW_NEW_EHFIELD);

      if( n_fields > 0 )
      {
        ok = ops->draw_nearfield(ctx, nf->points, npts,
            fields, n_fields, dr, nf->r_max);
      }
      else
        ok = FALSE;

      break;
    }

    case RENDER_MODE_STRUCTURE:
    {
      struct_draw_params_t params = build_struct_draw_params(r.fstep);
      if( ops->draw_axes != NULL )
        ops->draw_axes(ctx, (float)geom_pre.scene_radius);
      ok = ops->draw_structure(ctx, (float)geom_pre.scene_radius, &params);
      break;
    }

    default:
      BUG("render: unhandled mode %d\n", r.mode);
      ok = FALSE;
      break;
  }

  if( !ok )
  {
    /* Data dependency not satisfied (async compute, draw-style transition,
     * or transient buffer allocation failure).
     *
     * During optimization, freeze the previous frame to avoid flicker
     * while the freq loop is in flight. */
    if( isFlagSet(SUPPRESS_INTERMEDIATE_REDRAWS) )
    {
      g_rec_mutex_unlock(&freq_data_lock);
      return FALSE;
    }

    /* Returning TRUE with an empty scene causes the render loop to
     * proceed to glClear, replacing stale content with a diagnostic.
     * Returning FALSE would skip the clear and freeze the last valid
     * frame on screen (desirable only during optimization above). */
    if( ops->init_empty != NULL )
      ops->init_empty(ctx);
    ops->set_status(ctx,
        isFlagSet(FREQ_LOOP_DONE)
        ? STATUS_MSG_NOT_READY
        : STATUS_MSG_START_FREQLOOP);
    g_rec_mutex_unlock(&freq_data_lock);
    return TRUE;
  }

  /* Post-render UI updates (under lock, main thread) */
  if( view->type == VIEW_STRUCTURE )
  {
    Draw_Structure_UI();
  }
  else if( r.mode == RENDER_MODE_FARFIELD )
  {
    Update_Rdpattern_UI();
  }

  g_rec_mutex_unlock(&freq_data_lock);
  return TRUE;
}

/*-----------------------------------------------------------------------*/

/**
 * render_cairo() - GTK3 draw signal handler for Cairo rendering
 * @widget:    the GtkDrawingArea receiving the draw signal
 * @cr:        Cairo context provided by GTK
 * @user_data: unused
 *
 * Determines the view from the widget identity, builds a
 * cairo_render_ctx_t, and dispatches through render() with the
 * appropriate Cairo backend ops vtable.
 */
  gboolean
render_cairo(cairo_t *cr, view_t *v, const render_ops_t *ops)
{
  if( isFlagSet(INPUT_PENDING) )
    return FALSE;

  /* ERROR_CONDX: no rendering but return TRUE to stop GTK signal
   * propagation (prevents further draw handler invocations). */
  if( isFlagSet(ERROR_CONDX) )
    return TRUE;

  if( v == NULL )
    return FALSE;

  cairo_render_ctx_t ctx =
  {
    .cr     = cr,
    .view   = v,
  };

  /* Clear drawing surface */
  cairo_set_source_rgb(cr, BLACK);
  cairo_rectangle(cr, 0.0, 0.0, (double)v->width, (double)v->height);
  cairo_fill(cr);

  render((void *)&ctx, ops, v);

  return TRUE;
}

/*-----------------------------------------------------------------------*/
