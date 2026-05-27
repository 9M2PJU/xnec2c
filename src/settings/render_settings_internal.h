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

#ifndef RENDER_SETTINGS_INTERNAL_H
#define RENDER_SETTINGS_INTERNAL_H 1

#include "render_settings_common.h"

/* Internal interface between render_settings.c and its per-tab modules.
 * Not part of the public API declared in render_settings.h. */

/* Shared window widget; defined in render_settings.c */
extern GtkWidget *render_settings_window;

/* Per-tab dispatch table instances */
extern const config_tab_defaults_t general_tab_defaults;
extern const config_tab_defaults_t opengl_tab_defaults;
extern const config_tab_defaults_t cairo_tab_defaults;

/* General tab sync: applies GL-availability sensitivity to widgets */
void general_tab_sync(void);

/* Available in both HAVE_OPENGL and non-OpenGL builds */
void render_settings_sync_from_config(void);

#endif /* RENDER_SETTINGS_INTERNAL_H */
