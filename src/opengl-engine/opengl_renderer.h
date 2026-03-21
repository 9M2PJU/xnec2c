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
#include "opengl_arcball.h"

/* Vertex attribute descriptor */
typedef struct
{
  const char *name;
  int components;
  int offset;

} gl_vertex_attrib_t;

/* Shader container */
typedef struct
{
  GLuint program;
  GLuint vertex;
  GLuint fragment;

} gl_shader_t;

/* Delete a GL resource (array-style API: glDelete*(GLsizei, const GLuint*))
 * if non-zero, then zero the handle */
#define GL_DELETE(fn, id) \
  do { if( (id) ) { fn(1, &(id)); (id) = 0; } } while(0)

/* Delete a GL resource (single-handle API: glDelete*(GLuint))
 * if non-zero, then zero the handle */
#define GL_DELETE_OBJ(fn, id) \
  do { if( (id) ) { fn(id); (id) = 0; } } while(0)

/* Shader functions */
gboolean gl_shader_load(gl_shader_t *shader,
  const char *vertex_path, const char *fragment_path);
void gl_shader_destroy(gl_shader_t *shader);


#endif /* HAVE_OPENGL */
#endif /* OPENGL_RENDERER_H */
