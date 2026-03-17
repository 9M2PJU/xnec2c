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

#ifndef OPENGL_VIEW_TOOLTIP_H
#define OPENGL_VIEW_TOOLTIP_H 1

#include "common.h"

#ifdef HAVE_OPENGL
#include "opengl_view.h"

/* gl_view_show_tooltip()
 *
 * Display a tooltip message that holds for duration_ms then fades over 500ms.
 */
void gl_view_show_tooltip(GtkWidget *widget, const char *text, int duration_ms);

/* gl_view_render_tooltip()
 *
 * Render tooltip overlay with fade animation. Called from the render loop
 * when state->tooltip_active is set.
 */
void gl_view_render_tooltip(gl_view_state_t *state, int surf_width, int surf_height);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_VIEW_TOOLTIP_H */
