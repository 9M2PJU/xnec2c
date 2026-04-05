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

/* Frequency spinbutton value-changed callbacks; referenced by
 * SIGNAL_BLOCK/SIGNAL_UNBLOCK in programmatic update sites. */
void on_main_freq_spinbutton_value_changed(GtkSpinButton *spinbutton, gpointer user_data);
void on_rdpattern_freq_spinbutton_value_changed(GtkSpinButton *spinbutton, gpointer user_data);

/* Suppress -Wpedantic warnings from the g_signal_handlers_block_by_func macro,
 * which internally casts a function pointer to gpointer (forbidden by ISO C).
 * This cast is intentional and correct for GLib signal handling. */
#define SIGNAL_BLOCK(widget, func) \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
  g_signal_handlers_block_by_func((widget), (func), NULL); \
  _Pragma("GCC diagnostic pop") \
  G_GNUC_END_IGNORE_DEPRECATIONS

#define SIGNAL_UNBLOCK(widget, func) \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
  g_signal_handlers_unblock_by_func((widget), (func), NULL); \
  _Pragma("GCC diagnostic pop") \
  G_GNUC_END_IGNORE_DEPRECATIONS

#endif

