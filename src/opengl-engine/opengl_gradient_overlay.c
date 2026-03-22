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

#include "opengl_gradient_overlay.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/** update_overlay_texture() - Regenerate overlay texture using Cairo
 * @overlay: overlay instance
 */
  static void
update_overlay_texture(gradient_overlay_t *overlay)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  int width, height;

  if( !overlay || !overlay->base )
    return;

  width = overlay->base->width;
  height = overlay->base->height;

  if( width <= 0 || height <= 0 )
    return;

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create(surface);

  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
  cairo_paint(cr);

  if( overlay->draw_func )
    overlay->draw_func(cr);

  cairo_destroy(cr);

  cairo_gl_overlay_upload(overlay->base, surface);

  cairo_surface_destroy(surface);

  overlay->needs_update = FALSE;

} /* update_overlay_texture() */

/*-----------------------------------------------------------------------*/

/** gradient_overlay_new() - Allocate and initialize gradient overlay renderer
 * @draw_func: Cairo drawing function callback
 */
  gradient_overlay_t*
gradient_overlay_new(gradient_draw_fn draw_func)
{
  gradient_overlay_t *overlay;

  overlay = g_new0(gradient_overlay_t, 1);

  overlay->base = cairo_gl_overlay_new();
  if( !overlay->base )
  {
    pr_err("Failed to create cairo overlay base\n");
    g_free(overlay);
    return( NULL );
  }

  overlay->draw_func = draw_func;
  overlay->needs_update = TRUE;

  return( overlay );

} /* gradient_overlay_new() */

/*-----------------------------------------------------------------------*/

/** gradient_overlay_free() - Free gradient overlay resources
 * @overlay: overlay instance to free
 */
  void
gradient_overlay_free(gradient_overlay_t *overlay)
{
  if( !overlay )
    return;

  cairo_gl_overlay_free(overlay->base);
  g_free(overlay);

} /* gradient_overlay_free() */

/*-----------------------------------------------------------------------*/

/** gradient_overlay_set_viewport() - Update viewport dimensions and mark texture for regeneration
 * @overlay: overlay instance
 * @width: viewport width in pixels
 * @height: viewport height in pixels
 */
  void
gradient_overlay_set_viewport(gradient_overlay_t *overlay, int width, int height)
{
  if( !overlay || !overlay->base )
    return;

  if( overlay->base->width != width || overlay->base->height != height )
  {
    cairo_gl_overlay_set_size(overlay->base, width, height);
    overlay->needs_update = TRUE;
  }

} /* gradient_overlay_set_viewport() */

/*-----------------------------------------------------------------------*/

/** gradient_overlay_mark_dirty() - Mark overlay texture for regeneration
 * @overlay: overlay instance
 */
  void
gradient_overlay_mark_dirty(gradient_overlay_t *overlay)
{
  if( overlay )
    overlay->needs_update = TRUE;

} /* gradient_overlay_mark_dirty() */

/*-----------------------------------------------------------------------*/

/** gradient_overlay_render() - Render gradient overlay in screen space
 * @overlay: overlay instance
 */
  void
gradient_overlay_render(gradient_overlay_t *overlay)
{
  if( !overlay || !overlay->base )
    return;

  if( overlay->needs_update )
    update_overlay_texture(overlay);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

  cairo_gl_overlay_render(overlay->base);

  glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);

} /* gradient_overlay_render() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
