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
#include <epoxy/gl.h>
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
  vec3 up;
  mat4 rotation;
  float distance;
  float zoom;
  vec2 pan_offset;
  float last_x;
  float last_y;
  int drag_button;
  float aspect;

} arcball_state_t;

/* Generic GL instance base */
typedef struct
{
  gl_shader_t shader;
  GLuint vao;
  GLuint vbo;
  GLint mvp_location;
  arcball_state_t *arcball;
  gboolean initialized;

} gl_instance_t;

/* Shader functions */
gboolean gl_shader_load(gl_shader_t *shader,
  const char *vertex_path, const char *fragment_path);
void gl_shader_destroy(gl_shader_t *shader);

/* Arcball functions */
arcball_state_t* arcball_new(float distance, float aspect);
void arcball_free(arcball_state_t *ab);
void arcball_set_aspect(arcball_state_t *ab, float aspect);
void arcball_set_distance(arcball_state_t *ab, float distance);
void arcball_begin_drag(arcball_state_t *ab, int button, float x, float y);
void arcball_drag(arcball_state_t *ab, float x, float y);
void arcball_end_drag(arcball_state_t *ab);
void arcball_zoom(arcball_state_t *ab, float delta);
void arcball_pan(arcball_state_t *ab, float dx, float dy);
void arcball_get_mvp(arcball_state_t *ab, mat4 dest, float zoom, float fov);

/* GL instance functions */
gl_instance_t* gl_instance_new(const char *vert_shader, const char *frag_shader,
  float arcball_distance, float aspect);
void gl_instance_free(gl_instance_t *inst);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_RENDERER_H */
