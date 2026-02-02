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

#ifndef OPENGL_CYLINDER_H
#define OPENGL_CYLINDER_H 1

#include "common.h"

#ifdef HAVE_OPENGL

/* Default number of sides for cylinder approximation */
#define CYLINDER_DEFAULT_SEGMENTS 12

/* Minimum radius to use when segment radius is zero or very small */
#define CYLINDER_MIN_RADIUS 0.001

/* Lit cylinder mesh data (with normals for lighting) */
typedef struct
{
  lit_color_point_t *vertices;
  int vertex_count;
  int triangle_count;

} lit_cylinder_mesh_t;

/* Calculate required vertex count for a cylinder with given segments */
int opengl_cylinder_vertex_count(int segments);

/* Append lit cylinder to existing mesh buffer with computed normals
 * mesh->vertices must be pre-allocated with sufficient space
 * Returns updated vertex count, or -1 on error */
int opengl_lit_cylinder_append(
    lit_cylinder_mesh_t *mesh,
    int start_vertex,
    double x1, double y1, double z1,
    double x2, double y2, double z2,
    double radius,
    int segments,
    float r, float g, float b, float a);

/* Free lit cylinder mesh data */
void opengl_lit_cylinder_free(lit_cylinder_mesh_t *mesh);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_CYLINDER_H */
