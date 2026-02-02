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

#ifdef HAVE_OPENGL
#include "opengl_renderer.h"
#include "opengl_axes.h"
#include "opengl_cylinder.h"

/* Structure OpenGL state */
typedef struct
{
  gl_instance_t *gl;
  GLint position_location;
  GLint normal_location;
  GLint color_location;
  int vertex_count;
  opengl_axes_t *axes;
  gboolean initialized;
  int geometry_generation;

} structure_gl_state_t;

/* Get GL state from widget */
structure_gl_state_t* opengl_structure_get_state(GtkWidget *widget);

#endif /* HAVE_OPENGL */

/* Public API - always available, stubs when no OpenGL */
GtkWidget* opengl_structure_create_widget(void);
void opengl_structure_queue_redraw(void);
void opengl_structure_cleanup(void);

#endif /* OPENGL_STRUCTURE_H */
