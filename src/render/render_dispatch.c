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
#include "../shared.h"

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

  r->mode = RENDER_MODE_FARFIELD;
  r->show_gradient = rc_config.rdpattern_gradient_key;
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
}

/*-----------------------------------------------------------------------*/

  render_check_result_t
render_check(render_view_t view)
{
  render_check_result_t r = { RENDER_OK, RENDER_MODE_NONE, -1, NULL, FALSE };

  r.fstep = calc_data.freq_step;

  if( view == RENDER_VIEW_STRUCTURE )
  {
    r.mode = RENDER_MODE_STRUCTURE;
    return r;
  }

  render_check_rdpattern(&r);
  return r;
}

/*-----------------------------------------------------------------------*/

  gboolean
render(void *ctx, const render_ops_t *ops, render_view_t view, float zoom)
{
  render_check_result_t r;
  gboolean ok;

  g_rec_mutex_lock(&freq_data_lock);

  r = render_check(view);

  if( r.status == RENDER_SUPPRESS )
  {
    g_rec_mutex_unlock(&freq_data_lock);
    return FALSE;
  }

  if( r.status != RENDER_OK )
  {
    ops->init_empty(ctx, zoom);
    ops->set_status(ctx, r.message);
    ops->set_gradient(ctx, FALSE);
    g_rec_mutex_unlock(&freq_data_lock);
    return TRUE;
  }

  switch( r.mode )
  {
    case RENDER_MODE_FARFIELD:
      ok = ops->draw_farfield(ctx, r.fstep, zoom);
      break;

    case RENDER_MODE_NEARFIELD:
      ok = ops->draw_nearfield(ctx, r.fstep, zoom);
      break;

    case RENDER_MODE_STRUCTURE:
      ok = ops->draw_structure(ctx, zoom);
      break;

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
    ops->init_empty(ctx, zoom);
    ops->set_status(ctx,
        isFlagSet(FREQ_LOOP_DONE)
        ? STATUS_MSG_NOT_READY
        : STATUS_MSG_START_FREQLOOP);
    ops->set_gradient(ctx, FALSE);
    g_rec_mutex_unlock(&freq_data_lock);
    return TRUE;
  }

  ops->set_gradient(ctx, r.show_gradient);

  g_rec_mutex_unlock(&freq_data_lock);
  return TRUE;
}

/*-----------------------------------------------------------------------*/
