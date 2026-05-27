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

#ifndef OPENGL_RDPATTERN_H
#define OPENGL_RDPATTERN_H 1

#include "common.h"

/* Public API - always available, stubs when no OpenGL */
GtkWidget* opengl_rdpattern_create_widget(void);
void opengl_rdpattern_cleanup(void);
GtkWidget* opengl_rdpattern_get_widget(void);
#ifdef HAVE_OPENGL
#include "../opengl-engine/opengl_renderer.h"
#include "../render/render_dispatch.h"
#include "../view/view_core.h"

/* GL rdpattern leaf renderers; exported for unified gl_ops vtable */
gboolean gl_rdpat_draw_farfield(void *ctx, int fstep, const ff_draw_params_t *ff);
gboolean gl_rdpat_draw_nearfield(void *ctx,
    const near_field_point_t *origins, int npts,
    const nf_field_set_t *fields, int n_fields,
    double dr, double r_max);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_RDPATTERN_H */
