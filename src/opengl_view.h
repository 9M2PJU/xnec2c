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

#ifndef OPENGL_VIEW_H
#define OPENGL_VIEW_H 1

#include "common.h"

#ifdef HAVE_OPENGL
#include "opengl_renderer.h"
#include "opengl_axes.h"
#include "opengl_gradient_overlay.h"

/* Vertex attribute descriptor */
typedef struct
{
  const char *name;
  int components;
  int offset;

} gl_vertex_attrib_t;

/* View content provided by scene generator */
typedef struct
{
  void *vertices;
  int vertex_count;
  int vertex_stride;
  GLenum draw_mode;
  float r_max;
  float zoom;
  float model_scale;
  gboolean show_gradient;
  unsigned int generation;

} gl_view_content_t;

/* Scene provider interface */
typedef struct
{
  gboolean (*generate)(gl_view_content_t *out);
  void (*post_render)(void);
  void (*cleanup)(void);

} gl_scene_provider_t;

/* Static view configuration */
typedef struct
{
  const char *vertex_shader_path;
  const char *fragment_shader_path;
  const gl_vertex_attrib_t *attribs;
  int attrib_count;
  int vertex_stride;
  gboolean has_gradient;
  void (*gradient_draw)(cairo_t *cr);

} gl_view_config_t;

/* View state (engine-internal) */
typedef struct
{
  gl_shader_t shader;
  GLuint vao;
  GLuint vbo;
  GLint mvp_location;
  GLint *attrib_locations;
  opengl_axes_t *axes;
  gradient_overlay_t *overlay;
  gl_view_config_t *config;
  gl_scene_provider_t *scene;
  gl_view_content_t content;
  unsigned int last_generation;
  arcball_state_t *arcball;
  GtkSpinButton **zoom_spinbutton;
  float current_zoom;
  gboolean initialized;

} gl_view_state_t;

/* Public API */
GtkWidget* gl_view_create_widget(
    gl_view_config_t *config,
    gl_scene_provider_t *scene,
    arcball_state_t *arcball,
    GtkSpinButton **zoom_spinbutton);

gl_view_state_t* gl_view_get_state(GtkWidget *widget);
void gl_view_invalidate(GtkWidget *widget);
void gl_view_sync_arcball(GtkWidget *widget, double wr, double wi);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_VIEW_H */
