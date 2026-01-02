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

#ifndef OPENGL_RDPATTERN_H
#define OPENGL_RDPATTERN_H 1

#include "common.h"

#ifdef HAVE_OPENGL
#include "opengl_renderer.h"

/* Radiation pattern OpenGL state */
typedef struct
{
  gl_shader_t shader;
  GLuint vao;
  GLuint vbo;
  GLint mvp_location;
  GLint position_location;
  GLint color_location;
  arcball_state_t arcball;
  int triangle_count;
  gboolean initialized;

} rdpattern_gl_state_t;

/* Public API */
GtkWidget* opengl_rdpattern_create_widget(void);
void opengl_rdpattern_cleanup(void);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_RDPATTERN_H */
