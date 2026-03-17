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

/* Drag transparency: alpha multiplier applied to marked renderables during drag */
#define DRAG_ALPHA_FACTOR 0.5f

/* View content provided by scene generator */
typedef struct
{
  void *vertices;
  int vertex_count;
  int vertex_stride;
  GLenum draw_mode;
  float r_max;
  float clip_extent;
  float zoom;
  float model_scale;
  gboolean show_gradient;
  unsigned int generation;

  /* When TRUE, engine uses model_scale as-is (no ovl_model_scale_adj) */
  gboolean scale_adj_locked;

  /* Centered text overlay rendered when no data to display; NULL = none */
  const char *status_message;

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

/* Called once per frame before far_extent; used for content generation
 * in renderables that must produce data before extent is known */
typedef void (*gl_generate_fn)(void *ctx);

/* Unified renderable — all 3D objects implement this */
typedef struct
{
  gl_render_fn render;
  gl_prepare_fn prepare;
  gl_destroy_fn destroy;
  gl_active_fn is_active;
  gl_extent_fn far_extent;

  /* Called once per frame before far_extent; generates content for
   * renderables that must populate data before extent is known */
  gl_generate_fn generate;

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

  /* Optional ctrl+scroll handler for segment radius scaling */
  gboolean (*on_ctrl_scroll)(GtkWidget *widget, GdkEventScroll *event, gpointer view_state);

  /* Optional predicate controlling ground plane visibility.
   * When NULL, ground plane renderable is not created for this view. */
  gl_active_fn ground_plane_is_active;

  /* Optional predicate controlling axes visibility.
   * When NULL, the default opengl_axes_is_active predicate applies. */
  gl_active_fn axes_is_active;

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

  /* Cached clip planes from main render pass — shared by all renderables
   * so depth values are in the same projection space */
  float cached_near_plane;
  float cached_far_plane;

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
  cairo_surface_t *tooltip_surface;
  int tooltip_surf_width;
  int tooltip_surf_height;
  gboolean tooltip_surface_valid;

  /* Status message overlay (persistent, no fade) */
  cairo_gl_overlay_t *status_overlay;
  cairo_surface_t *status_surface;
  int status_surf_width;
  int status_surf_height;
  const char *status_last_text;

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
/* gl_view_setup_attribs()
 *
 * Configure vertex attribute pointers in VAO. Called once during prepare
 * when VBO data changes. VAO retains this state for subsequent renders.
 */
void gl_view_setup_attribs(
    GLuint vao,
    GLuint vbo,
    const gl_vertex_attrib_t *attribs,
    const GLint *attrib_locations,
    int attrib_count,
    int vertex_stride);

/* gl_view_draw_pass()
 *
 * Execute a rendering pass. VAO already has attrib config from prepare.
 */
void gl_view_draw_pass(
    GLuint shader_program,
    GLint mvp_location,
    mat4 mvp,
    GLuint vao,
    GLenum draw_mode,
    int vertex_count);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_VIEW_H */
