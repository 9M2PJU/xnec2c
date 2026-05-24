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

#ifndef OPENGL_STATE_H
#define OPENGL_STATE_H 1

#include "../common.h"

#ifdef HAVE_OPENGL

/** opengl_gl_context_failed - Return TRUE when GL context creation failed
 *
 * Read by callers (settings dialog renderer-toggle, opengl_set_renderer) to
 * gate operations that depend on OpenGL availability at runtime.
 */
gboolean opengl_gl_context_failed(void);

/** opengl_gl_init_failed - GSourceOnceFunc; disable OpenGL renderer
 * @_unused: idle callback data, ignored
 *
 * Queued via g_idle_add_once from gl_view_config_t.on_gl_init_failed when
 * GtkGLArea realize fails.  Switches both structure and rdpattern views to
 * Cairo fallback and latches the failure state.
 */
void opengl_gl_init_failed(gpointer _unused);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_STATE_H */
