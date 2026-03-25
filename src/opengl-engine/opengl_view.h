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

/* Maximum renderables per view (constrained by guint32 active_mask in render loop) */
#define MAX_RENDERABLES 32

/* Number of depth-peel passes for order-independent transparency.
 * 4 covers: near patch face, far patch face, near cylinder wall,
 * far cylinder wall — or structure front/back + radiation front/back. */
#define PEEL_PASSES 4

/* Texture unit used for the previous-pass depth texture during depth peeling */
#define PEEL_DEPTH_TEX_UNIT 2

/* Convert transparency percentage (0/25/50/75) to alpha multiplier */
#define DRAG_ALPHA_FROM_LEVEL(lvl) \
    ((lvl) == 0 ? 1.0f : 1.0f - ((lvl) / 100.0f))

/* View content provided by scene generator */
typedef struct
{
  gl_draw_batch_t batches[GL_VIEW_MAX_BATCHES];
  int batch_count;
  int vertex_stride;
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

/* Per-frame render parameters passed to each renderable's render callback */
typedef struct gl_render_params_s
{
  mat4 mvp;
  mat4 mv;
  float alpha;
  int peel_pass;

} gl_render_params_t;

/* Uniform locations for depth-peel discard logic (shared by all peel-aware shaders) */
typedef struct
{
  GLint peel_depth;
  GLint peel_pass;

} gl_peel_uniform_locs_t;

/* Renderable interface callback types */
typedef void (*gl_render_fn)(void *ctx, const gl_render_params_t *params);
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

  /* Sort priority for transparent pass (lower value renders first).
   * With depth peeling, per-pixel layering is handled by the peel
   * passes; sort_order controls renderable draw order within each
   * pass — the GL_ZERO dest blend factor means the nearest fragment
   * at each pixel wins, so order matters only at equal depth. */
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
typedef struct gl_view_state_s
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

  /* Patch current flow phase animation offset (radians) */
  float flow_phase;

  /* LIC noise texture (256x256 grayscale, shared by all renderables) */
  GLuint noise_tex;

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

  /* Depth-peel transparency state */
  GLuint peel_fbo[2];          /* ping-pong FBOs for depth peeling (single-sample resolve targets) */
  GLuint peel_depth_tex[2];    /* depth textures attached to peel FBOs */
  GLuint peel_color_tex;       /* shared color texture for current layer */
  GLuint accum_fbo;            /* accumulation FBO (under-operator) */
  GLuint accum_color_tex;      /* RGBA accumulation texture */

  /* Multisampled peel FBOs — used for rasterization when MSAA is active,
   * then blit-resolved to single-sample peel_fbo[] for shader reads */
  GLuint peel_ms_fbo[2];      /* multisampled ping-pong FBOs */
  GLuint peel_ms_color_rbo[2]; /* MS color renderbuffers */
  GLuint peel_ms_depth_rbo[2]; /* MS depth renderbuffers */
  GLuint composite_program;    /* fullscreen quad shader program */
  GLuint composite_vs;         /* composite vertex shader */
  GLuint composite_fs;         /* composite fragment shader */
  GLuint composite_vao;        /* fullscreen triangle VAO */
  GLuint composite_vbo;        /* fullscreen triangle VBO (3 vertices) */
  GLint  composite_u_layer;    /* sampler uniform: current peel layer */
  int peel_width;              /* current peel texture dimensions */
  int peel_height;
  int peel_msaa_samples;       /* MSAA sample count at last peel creation */

  gboolean initialized;

} gl_view_state_t;

/* Sorting entry for the transparent render pass */
typedef struct
{
  int index;
  int sort_order;
  float alpha;
  float depth;

} gl_trans_item_t;

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


#endif /* HAVE_OPENGL */
#endif /* OPENGL_VIEW_H */
