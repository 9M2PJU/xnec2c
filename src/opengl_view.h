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
#include "opengl_gradient_overlay.h"

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

/* Overlay configuration for second rendering pass */
typedef struct
{
  const char *vertex_shader_path;
  const char *fragment_shader_path;
  const gl_vertex_attrib_t *attribs;
  int attrib_count;

} gl_overlay_config_t;

/* Renderable interface callback types */
typedef void (*gl_render_fn)(void *ctx, mat4 mvp, float alpha);
typedef void (*gl_prepare_fn)(void *ctx, float r_max);
typedef void (*gl_destroy_fn)(void *ctx);
typedef gboolean (*gl_active_fn)(void *ctx);
typedef float (*gl_extent_fn)(void *ctx, float r_max);

/* Unified renderable — all 3D objects implement this */
typedef struct
{
  gl_render_fn render;
  gl_prepare_fn prepare;
  gl_destroy_fn destroy;
  gl_active_fn is_active;
  gl_extent_fn far_extent;
  void *ctx;
  float alpha;
  vec3 origin;

  /* Sort priority for transparent pass (lower renders first) */
  int transparent_sort_order;

  /* Whether alpha is reduced during drag interaction */
  gboolean transparent_on_drag;

} gl_renderable_t;

/* Scene provider interface */
typedef struct
{
  gboolean (*generate)(gl_view_content_t *out);
  void (*post_render)(void);
  void (*cleanup)(void);

  /* Optional overlay (second shader pass) */
  const gl_overlay_config_t *overlay_config;
  gboolean (*overlay_generate)(const gl_view_content_t *primary, gl_view_content_t *out);
  void (*overlay_cleanup)(void);

  /* Optional shift+scroll handler for custom behavior */
  gboolean (*on_shift_scroll)(GtkWidget *widget, GdkEventScroll *event, gpointer view_state);

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
  GArray *renderables;
  gradient_overlay_t *overlay;
  gl_view_config_t *config;
  gl_scene_provider_t *scene;
  gl_view_content_t content;
  unsigned int last_generation;
  arcball_state_t *arcball;
  GtkSpinButton **zoom_spinbutton;

  /* Per-view projection parameters (not in arcball, which can be shared) */
  float aspect;
  float viewport_height;
  float fov_rad;

  /* Per-view pan state (independent from arcball rotation) */
  vec2 pan_offset;
  float cached_camera_distance;

  /* Overlay scale adjustment (user-controlled via shift+scroll) */
  float ovl_model_scale_adj;

  /* Tooltip state for transient messages */
  gboolean tooltip_active;
  char *tooltip_text;
  double tooltip_alpha;
  gint64 tooltip_start_time;
  guint tooltip_timeout_id;
  int tooltip_hold_ms;
  cairo_gl_overlay_t *tooltip_overlay;

  /* Drag transparency state */
  gboolean drag_active;
  float drag_alpha_factor;

  /* MSAA state */
  GLuint msaa_fbo;
  GLuint msaa_color_rbo;
  GLuint msaa_depth_rbo;
  int msaa_samples;
  int msaa_width;
  int msaa_height;

  gboolean initialized;

} gl_view_state_t;

/* Public API */
GtkWidget* gl_view_create_widget(
    gl_view_config_t *config,
    gl_scene_provider_t *scene,
    arcball_state_t *arcball,
    GtkSpinButton **zoom_spinbutton);

gl_view_state_t* gl_view_get_state(GtkWidget *widget);
void gl_view_set_arcball(GtkWidget *widget, arcball_state_t *arcball);
void gl_view_sync_arcball(GtkWidget *widget, double wr, double wi);
void gl_view_reset_pan(GtkWidget *widget);
void gl_view_show_tooltip(GtkWidget *widget, const char *text, int duration_ms);
void Set_MSAA_Samples(int samples);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_VIEW_H */
