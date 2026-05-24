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
 * opengl_state: cross-cutting OpenGL backend runtime state.
 *
 * Latches the result of the most recent GtkGLArea realize attempt.  Once
 * latched FALSE-on-failure, callers refuse to re-enable the OpenGL
 * renderer until process restart.
 */

#include "../shared.h"

#ifdef HAVE_OPENGL
#include "opengl_state.h"

/*------------------------------------------------------------------------*/

/* Latched failure flag for the GL context */
static gboolean opengl_context_failed = FALSE;

/*------------------------------------------------------------------------*/

gboolean
opengl_gl_context_failed(void)
{
  return( opengl_context_failed );

} /* opengl_gl_context_failed() */

/*------------------------------------------------------------------------*/

void
opengl_gl_init_failed(gpointer _unused)
{
  if( opengl_context_failed )
    return;

  (void)_unused;
  opengl_context_failed = TRUE;
  pr_warn("OpenGL is not available on this display; switching to Cairo renderer. See 'OpenGL Renderer' in View > Rendering Settings.\n");
  opengl_set_renderer(FALSE);

} /* opengl_gl_init_failed() */

#endif /* HAVE_OPENGL */
