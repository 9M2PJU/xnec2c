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
#include "../render/render_dispatch.h"

#ifdef HAVE_OPENGL
#include "../shared.h"

/* Generate line geometry from dispatch-resolved near-field vector sets.
 * Iterates fields[0..n_fields-1], converting nf_vector_t to lit_color_point_t pairs.
 * Returns total line count, or -1 on failure. */
int opengl_rdpattern_generate_nf_field_lines(
    const near_field_point_t *origins, int npts,
    const nf_field_set_t *fields, int n_fields,
    double dr);

/* Tessellate point_3d buffer into colored triangles.
 * Returns triangle count, or -1 on invalid input. */
int opengl_rdpattern_generate_triangles(
    point_3d_t *points, int nth, int nph,
    double r_min, double r_range);

/* Return near-field line vertex buffer and count */
lit_color_point_t* opengl_rdpattern_get_nf_lines(int *count);

/* Return far-field triangle buffer and count */
lit_color_triangle_t* opengl_rdpattern_get_triangles(int *count);

/* Tessellate point_3d buffer into colored line pairs for wireframe.
 * Returns line count, or -1 on invalid input. */
int opengl_rdpattern_generate_lines(
    point_3d_t *points, int nth, int nph,
    double r_min, double r_range);

/* Return far-field wireframe line buffer and count */
lit_color_point_t* opengl_rdpattern_get_lines(int *count);

/* Return current far-field generation counter */
unsigned int opengl_rdpattern_get_ff_generation(void);

/* Return current near-field generation counter */
unsigned int opengl_rdpattern_get_nf_generation(void);

/* Free all geometry buffers and reset state */
void opengl_rdpattern_geometry_cleanup(void);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_RDPATTERN_GEOMETRY_H */
