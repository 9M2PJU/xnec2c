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

#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H 1

#include "common.h"

#ifdef HAVE_OPENGL
#include <GL/gl.h>
#include <cglm/cglm.h>

/* Shader container */
typedef struct
{
  GLuint program;
  GLuint vertex;
  GLuint fragment;

} gl_shader_t;

/* Arcball camera state */
typedef struct
{
  vec3 eye;
  vec3 center;
  mat4 rotation;
  float distance;
  float zoom;

} arcball_state_t;

/* Shader functions */
gboolean gl_shader_load(gl_shader_t *shader,
  const char *vertex_path, const char *fragment_path);
void gl_shader_destroy(gl_shader_t *shader);

/* Arcball functions */
void arcball_init(arcball_state_t *state, float distance);
void arcball_get_mvp(arcball_state_t *state, mat4 dest,
  float aspect, float fov);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_RENDERER_H */
