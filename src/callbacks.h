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

#ifndef CALLBACKS_H
#define CALLBACKS_H     1

#include "common.h"
#include "interface.h"
#include "fork.h"
#include "xnec2c.h"
#include "editors.h"
#include "nec2_model.h"

#include <gdk/gdkkeysyms.h>

/* OpenGL common projection sync */
void opengl_common_projection_sync(void);

/* Frequency spinbutton value-changed callback; referenced by
 * SIGNAL_BLOCK/SIGNAL_UNBLOCK in programmatic update sites. */
void on_freq_spinbutton_value_changed(GtkSpinButton *spinbutton, gpointer user_data);

#ifdef HAVE_OPENGL
/* Ortho toolbar projection sync (defined in callbacks.c) */
void sync_ortho_toolbar_button(void);
void on_ortho_toggled(GtkToggleButton *button, gpointer user_data);
#endif /* HAVE_OPENGL */

/* ISO C forbids casting a function pointer to gpointer (void *) directly.
 * A union reinterpret is defined behavior under C99/C11 and avoids that cast.
 * g_signal_handlers_block_by_func is bypassed in favor of the underlying
 * g_signal_handlers_block_matched to keep the cast site visible and explicit. */
#define SIGNAL_BLOCK(widget, func) \
  do { \
    union { GCallback cb; gpointer p; } _u = { .cb = (GCallback)(func) }; \
    g_signal_handlers_block_matched((widget), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, _u.p, NULL); \
  } while(0)

#define SIGNAL_UNBLOCK(widget, func) \
  do { \
    union { GCallback cb; gpointer p; } _u = { .cb = (GCallback)(func) }; \
    g_signal_handlers_unblock_matched((widget), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, _u.p, NULL); \
  } while(0)

#endif

