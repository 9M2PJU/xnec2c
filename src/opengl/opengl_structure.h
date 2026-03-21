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

#ifndef OPENGL_STRUCTURE_H
#define OPENGL_STRUCTURE_H 1

#include "common.h"

/* Draw mode for structure rendering */
typedef enum
{
  STRUCTURE_DRAW_GEOMETRY = 0,
  STRUCTURE_DRAW_CURRENTS,
  STRUCTURE_DRAW_CHARGES

} structure_draw_mode_t;

/* Flow direction visualization mode for patch currents */
typedef enum
{
  FLOW_DIR_REFERENCE_PHASE = 0,
  FLOW_DIR_POLARIZATION_TILT,
  FLOW_DIR_PEAK_MAGNITUDE,
  FLOW_DIR_LIC

} flow_direction_mode_t;

/* Shared structure geometry for overlay rendering */
typedef struct
{
  void *vertices;
  int vertex_count;
  int vertex_stride;
  float view_scale;
  unsigned int draw_mode;
  unsigned int generation;

  /* Vertex index where wire geometry ends and patch geometry begins.
   * Used by the renderer to issue separate draw calls with different
   * polygon offset settings for wire vs patch depth priority. */
  int wire_vertex_count;

  /* Excitation center coordinates in structure space */
  gboolean has_excitation_center;
  double excitation_center_x;
  double excitation_center_y;
  double excitation_center_z;

} structure_overlay_data_t;

/* Public API - always available, stubs when no OpenGL */
GtkWidget* opengl_structure_create_widget(void);
void opengl_structure_cleanup(void);
void opengl_structure_invalidate(void);
void opengl_structure_queue_draw(void);

/* Scale below which segments render as lines instead of cylinders */
#define CYLINDER_SCALE_LINE_THRESHOLD 1.0

#ifdef HAVE_OPENGL
#include "../opengl-engine/opengl_renderer.h"

/* Extended vertex with UV and flow data for chevron shader.
 * First 48 bytes are layout-identical to lit_color_point_t. */
typedef struct
{
  point_f_3d_t point;
  point_f_3d_t normal;
  rgba_f_t color;
  float uv[2];
  float flow_data[4];   /* Re(ct1), Im(ct1), Re(ct2), Im(ct2) */
  float depth_bias;     /* NDC depth offset for coplanar z-fighting */

} structure_vertex_t;

/* Vertex attribute layout for lit-color shader (structure rendering) */
extern const gl_vertex_attrib_t opengl_structure_attribs[3];

/* Vertex attribute layout for chevron shader (5 attribs: pos/norm/color/uv/flow) */
extern const gl_vertex_attrib_t opengl_chevron_attribs[6];

arcball_state_t* opengl_structure_get_arcball(void);
GtkWidget* opengl_structure_get_widget(void);

/* Get/set cylinder radius display scale factor */
double opengl_structure_get_radius_scale(void);
void opengl_structure_set_radius_scale(double scale);

/* Ctrl+scroll handler for adjusting cylinder radius scale.
 * Usable by any scene provider that renders structure geometry. */
gboolean opengl_structure_on_ctrl_scroll(
    GtkWidget *widget, GdkEventScroll *event, gpointer view_state);

/* Update shared geometry buffer from global data/crnt/flags.
 * Call before reading shared geometry to ensure freshness. */
void opengl_structure_update_shared_geometry(void);

/* Return read-only pointer to shared geometry data */
const structure_overlay_data_t* opengl_structure_get_shared_geometry(void);

/* Update spin button display text without emitting value_changed signal */
void opengl_update_spin_display(GtkSpinButton *spin, double angle);

/* Timeout callback for flow phase animation (FLOW_ANIMATE flag) */
gboolean Animate_Flow_Phase(gpointer udata);

/* Reset flow phase to zero on both CPU geometry and GPU views */
void opengl_structure_reset_flow_phase(void);

#endif

#endif /* OPENGL_STRUCTURE_H */
