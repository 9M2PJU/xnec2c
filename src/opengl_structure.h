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

/* Shared structure geometry for overlay rendering */
typedef struct
{
  void *vertices;
  int vertex_count;
  int vertex_stride;
  float view_scale;
  unsigned int generation;

} structure_overlay_data_t;

/* Public API - always available, stubs when no OpenGL */
GtkWidget* opengl_structure_create_widget(void);
void opengl_structure_cleanup(void);
void opengl_structure_queue_draw(void);

#ifdef HAVE_OPENGL
#include "opengl_renderer.h"

/* Vertex attribute layout for lit-color shader (structure rendering) */
extern const gl_vertex_attrib_t opengl_structure_attribs[3];

arcball_state_t* opengl_structure_get_arcball(void);

/* Update shared geometry buffer from global data/crnt/flags.
 * Call before reading shared geometry to ensure freshness. */
void opengl_structure_update_shared_geometry(void);

/* Return read-only pointer to shared geometry data */
const structure_overlay_data_t* opengl_structure_get_shared_geometry(void);

/* Update spin button display text without emitting value_changed signal */
void opengl_update_spin_display(GtkWidget *spin, double angle);

#endif

#endif /* OPENGL_STRUCTURE_H */
