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

#ifndef GRADIENT_CACHE_H
#define GRADIENT_CACHE_H 1

#include <cairo.h>
#include <stdint.h>

/*
 * gradient_cache: Self-validating gradient legend surface cache.
 *
 * Called by the presentation layer (render_dispatch) during render();
 * the returned surface is forwarded to backends via render_check_result_t.
 * Stores the inputs that produced the cached surface alongside the
 * surface itself.  gradient_cache_update() compares stored inputs against
 * current authoritative state; a mismatch triggers a re-render.
 * No dirty flags — no per-call-site invalidation obligations.
 */
typedef struct
{
  cairo_surface_t *surface; /* cached ARGB32 gradient rendering */
  uint64_t version;    /* monotonic counter; increments on rebuild */
  int    width;             /* width at render time */
  int    height;            /* height at render time */
  double freq_mhz;          /* calc_data.freq_mhz at render time */
  int    gain_style;        /* rc_config.gain_style at render time */
  int    pol_type;          /* calc_data.pol_type at render time */
  double max_gain;          /* rad_pattern[fstep].max_gain[pol] at render time */
  double min_gain;          /* rad_pattern[fstep].min_gain[pol] at render time */
} gradient_cache_t;

/*-----------------------------------------------------------------------
 * API
 *----------------------------------------------------------------------*/

/**
 * gradient_cache_update() - Reconcile cache with current authoritative state
 * @cache:  cache instance
 * @w:      current viewport width
 * @h:      current viewport height
 *
 * Compares stored (freq_mhz, gain_style, pol_type, w, h) against current values.
 * On mismatch: destroys old surface, creates new ARGB32 surface, calls
 * Draw_Color_Legend_Overlay(), stores surface and input snapshot.
 * On match: no-op.
 */
void gradient_cache_update(gradient_cache_t *cache, int w, int h);

/**
 * gradient_cache_get_overlay() - Presentation entry point for gradient legend
 * @w:       current viewport width
 * @h:       current viewport height
 * @version: if non-NULL, receives the cache version for staleness detection
 *
 * Checks rc_config.rdpattern_gradient_key; returns NULL when disabled or
 * dimensions are invalid.  Otherwise updates the module-internal singleton
 * cache and returns the rendered ARGB32 surface.
 */
cairo_surface_t *gradient_cache_get_overlay(int w, int h, uint64_t *version);

#endif /* GRADIENT_CACHE_H */
