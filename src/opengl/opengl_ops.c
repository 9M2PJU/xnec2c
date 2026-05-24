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
 * opengl_ops: Unified GL backend vtable for render_ops_t dispatch.
 *
 * Assembles leaf renderers from opengl_structure.c and opengl_rdpattern.c
 * into the single vtable consumed by render().  GL axes are handled via
 * separate renderables, not through the render_ops_t dispatch.
 */

#include "../shared.h"

#ifdef HAVE_OPENGL

#include "opengl_structure.h"
#include "opengl_rdpattern.h"
#include "../opengl-engine/opengl_view.h"

/* Unified GL backend vtable; render() gates slot calls by mode */
const render_ops_t gl_ops =
{
  .draw_farfield  = gl_rdpat_draw_farfield,
  .draw_nearfield = gl_rdpat_draw_nearfield,
  .draw_structure = gl_draw_structure,
  .draw_axes      = NULL,
  .init_empty     = gl_view_init_empty,
  .set_status     = gl_view_set_status,
  .set_gradient   = gl_view_set_gradient,
};

#endif /* HAVE_OPENGL */
