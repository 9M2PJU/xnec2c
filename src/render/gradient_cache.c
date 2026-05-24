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

/* Module-internal singleton; the presentation layer (render_dispatch)
 * calls gradient_cache_get_overlay() and forwards the result to backends. */
static gradient_cache_t rdpat_cache;

/*-----------------------------------------------------------------------*/

/**
 * gradient_cache_update() - Reconcile cache with current authoritative state
 * @cache: cache instance
 * @w:     current viewport width
 * @h:     current viewport height
 *
 * Compares stored presentation_cache_key and (w, h) against current values.
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

  presentation_cache_key_t cur_key =
      presentation_cache_key_build(calc_data.freq_step);

  if( cache->surface != NULL &&
      cache->width  == w &&
      cache->height == h &&
      presentation_cache_key_match(&cache->cache_key, &cur_key) )
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
    cache->width  = 0;
    cache->height = 0;
    presentation_cache_key_invalidate(&cache->cache_key);
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
  cache->width     = w;
  cache->height    = h;
  cache->cache_key = cur_key;

} /* gradient_cache_update() */

/*-----------------------------------------------------------------------*/

/**
 * gradient_cache_get_overlay() - Presentation entry point for gradient legend
 * @w:       current viewport width
 * @h:       current viewport height
 *
 * Single presentation-layer decision point for the gradient legend.
 * Returns {NULL, 0} when the legend is disabled or dimensions are invalid.
 * Otherwise updates the module-internal singleton and returns the surface
 * coupled with its version for downstream staleness detection.
 */
  gradient_result_t
gradient_cache_get_overlay(int w, int h)
{
  gradient_result_t none = {NULL, 0};

  if( !rc_config.rdpattern_gradient_key )
    return none;

  if( w <= 0 || h <= 0 )
    return none;

  gradient_cache_update(&rdpat_cache, w, h);

  return (gradient_result_t){rdpat_cache.surface, rdpat_cache.version};

} /* gradient_cache_get_overlay() */

/*-----------------------------------------------------------------------*/
