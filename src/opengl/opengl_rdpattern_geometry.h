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

#ifndef OPENGL_RDPATTERN_GEOMETRY_H
#define OPENGL_RDPATTERN_GEOMETRY_H 1

#include "common.h"
#include "../draw_radiation.h"

#ifdef HAVE_OPENGL
#include "../shared.h"

/* Generate line geometry for near-field vectors.
 * Returns line count, or -1 if near-field is inactive or invalid. */
int opengl_rdpattern_generate_nf_lines(void);

/* Tessellate point_3d buffer into colored triangles.
 * Returns triangle count, or -1 on invalid input. */
int opengl_rdpattern_generate_triangles(
    point_3d_t *points, int nth, int nph,
    double r_min, double r_range);

/* Return near-field line vertex buffer and count */
lit_color_point_t* opengl_rdpattern_get_nf_lines(int *count);

/* Return far-field triangle buffer and count */
lit_color_triangle_t* opengl_rdpattern_get_triangles(int *count);

/* Return current far-field generation counter */
unsigned int opengl_rdpattern_get_ff_generation(void);

/* Return current near-field generation counter */
unsigned int opengl_rdpattern_get_nf_generation(void);

/* Free all geometry buffers and reset state */
void opengl_rdpattern_geometry_cleanup(void);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_RDPATTERN_GEOMETRY_H */
