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

#ifndef OPENGL_AXES_H
#define OPENGL_AXES_H 1

#include "common.h"

#ifdef HAVE_OPENGL
#include "opengl_renderer.h"

typedef struct
{
  GLuint lines_vao;
  GLuint lines_vbo;
  gl_shader_t *line_shader;
  GLint line_mvp_loc;
  GLint line_pos_loc;
  GLint line_col_loc;

  GLuint labels_vao;
  GLuint labels_vbo;
  GLuint label_texture;
  gl_shader_t label_shader;
  GLint label_mvp_loc;
  GLint label_tex_loc;
  GLint label_pos_loc;
  GLint label_uv_loc;

  float r_max;
  gboolean initialized;

} opengl_axes_t;

opengl_axes_t* opengl_axes_new(gl_shader_t *line_shader);
void opengl_axes_free(opengl_axes_t *axes);
void opengl_axes_set_scale(opengl_axes_t *axes, float r_max);
void opengl_axes_render(opengl_axes_t *axes, mat4 mvp);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_AXES_H */
