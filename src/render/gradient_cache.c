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
 * gradient_cache: Self-validating gradient legend surface cache.
 *
 * Single source of truth for the gradient legend rendering.
 * The presentation layer (render_dispatch) calls gradient_cache_get_overlay()
 * during render(); the returned surface is then forwarded to whichever
 * backend (Cairo or GL) is active via render_check_result_t.
 * Correctness is enforced by storing the inputs that produced the surface
 * and comparing against current authoritative state on each update call.
 */
#include "gradient_cache.h"
#include "../shared.h"
#include "../rdpattern_ui.h"
#include <math.h>

/* Module-internal singleton; the presentation layer (render_dispatch)
 * calls gradient_cache_get_overlay() and forwards the result to backends. */
static gradient_cache_t rdpat_cache;

/*-----------------------------------------------------------------------*/

/* Epsilon for freq_mhz comparison: 1e-6 MHz = 1 Hz, below any meaningful
 * frequency resolution in NEC2 calculations. */
#define GRAD_FREQ_EPS 1e-6

/**
 * gradient_cache_update() - Reconcile cache with current authoritative state
 * @cache: cache instance
 * @w:     current viewport width
 * @h:     current viewport height
 *
 * Compares (freq_mhz, gain_style, pol_type, w, h) against current values.
 * On any mismatch the old surface is destroyed and a fresh ARGB32 surface
 * is created, Draw_Color_Legend_Overlay() is called to populate it, and
 * the input snapshot is stored.  Matching inputs are a no-op.
 */
  void
gradient_cache_update(gradient_cache_t *cache, int w, int h)
{
  /* Reject invalid dimensions (transient during widget teardown or
   * before the first resize event propagates). */
  if( w <= 0 || h <= 0 )
    return;

  double cur_freq_mhz  = calc_data.freq_mhz;
  int    cur_gain_style = rc_config.gain_style;
  int    cur_pol_type   = calc_data.pol_type;
  int    fstep          = calc_data.freq_step;

  /* Read current max/min gain for the active polarization.
   * These change when the forked child sends computed radiation data
   * back to the parent, even when freq_mhz itself is unchanged. */
  double cur_max_gain = 0.0;
  double cur_min_gain = 0.0;
  if( rad_pattern != NULL &&
      fstep >= 0 && fstep <= calc_data.steps_total &&
      rad_pattern[fstep].max_gain != NULL &&
      rad_pattern[fstep].min_gain != NULL )
  {
    cur_max_gain = rad_pattern[fstep].max_gain[cur_pol_type];
    cur_min_gain = rad_pattern[fstep].min_gain[cur_pol_type];
  }

  if( cache->surface != NULL &&
      cache->width      == w                                    &&
      cache->height     == h                                    &&
      dl_feq_eps(cache->freq_mhz, cur_freq_mhz, GRAD_FREQ_EPS) &&
      cache->gain_style == cur_gain_style                       &&
      cache->pol_type   == cur_pol_type                         &&
      dl_feq(cache->max_gain, cur_max_gain)                     &&
      dl_feq(cache->min_gain, cur_min_gain) )
    return;

  if( cache->surface != NULL )
    cairo_surface_destroy(cache->surface);

  cache->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  if( cairo_surface_status(cache->surface) != CAIRO_STATUS_SUCCESS )
  {
    pr_warn("gradient_cache_update: surface creation failed (%dx%d)\n", w, h);
    cairo_surface_destroy(cache->surface);
    cache->surface    = NULL;
    /* Invalidate input snapshot so a transient retry rebuilds even when
     * dimensions and authoritative state remain unchanged. */
    cache->width      = 0;
    cache->height     = 0;
    cache->freq_mhz   = 0.0;
    cache->gain_style = -1;
    cache->pol_type   = -1;
    cache->max_gain   = 0.0;
    cache->min_gain   = 0.0;
    return;
  }

  {
    cairo_t *cr = cairo_create(cache->surface);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    Draw_Color_Legend_Overlay(cr);
    cairo_destroy(cr);
  }

  cache->version++;
  cache->width      = w;
  cache->height     = h;
  cache->freq_mhz   = cur_freq_mhz;
  cache->gain_style = cur_gain_style;
  cache->pol_type   = cur_pol_type;
  cache->max_gain   = cur_max_gain;
  cache->min_gain   = cur_min_gain;

} /* gradient_cache_update() */

/*-----------------------------------------------------------------------*/

/**
 * gradient_cache_get_overlay() - Presentation entry point for gradient legend
 * @w:       current viewport width
 * @h:       current viewport height
 * @version: if non-NULL, receives the cache version for staleness detection
 *
 * Single presentation-layer decision point for the gradient legend.
 * Returns NULL when the legend is disabled or dimensions are invalid.
 * Otherwise updates the module-internal singleton and returns the surface.
 */
  cairo_surface_t *
gradient_cache_get_overlay(int w, int h, uint64_t *version)
{
  if( !rc_config.rdpattern_gradient_key )
    return NULL;

  if( w <= 0 || h <= 0 )
    return NULL;

  gradient_cache_update(&rdpat_cache, w, h);

  if( version != NULL )
    *version = rdpat_cache.version;

  return rdpat_cache.surface;

} /* gradient_cache_get_overlay() */

/*-----------------------------------------------------------------------*/
