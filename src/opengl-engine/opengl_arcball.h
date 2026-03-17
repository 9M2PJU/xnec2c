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

#ifndef OPENGL_ARCBALL_H
#define OPENGL_ARCBALL_H 1

#include "common.h"

#ifdef HAVE_OPENGL
#include <epoxy/gl.h>
#include <cglm/cglm.h>

/* Base distance multiplier for arcball viewing (sqrt(3) * 1.25) */
#define ARCBALL_BASE_DISTANCE_FACTOR 2.165f

/* Arcball rotation mode */
typedef enum
{
  ARCBALL_DRAG_FREE = 0,
  ARCBALL_DRAG_CONSTRAINED = 1

} arcball_drag_mode_t;

#define ARCBALL_MAX_CALLBACKS 4

/* Arcball camera state */
typedef struct arcball_state arcball_state_t;

/* Arcball change notification callback */
typedef void (*arcball_callback_fn)(arcball_state_t *ab, gpointer user_data);

typedef struct
{
  arcball_callback_fn func;
  gpointer user_data;

} arcball_callback_t;

struct arcball_state
{
  mat4 rotation;
  float last_x;
  float last_y;
  int drag_button;
  arcball_callback_t callbacks[ARCBALL_MAX_CALLBACKS];
  int callback_count;
  gboolean in_notify;
  arcball_drag_mode_t drag_mode;
  float wr_deg;
  float wi_deg;
  float motion_divisor;
};

arcball_state_t* arcball_new(float motion_divisor);
void arcball_free(arcball_state_t *ab);
void arcball_set_view(arcball_state_t *ab, float wr_deg, float wi_deg);
void arcball_begin_drag(arcball_state_t *ab, int button, float x, float y);
void arcball_drag(arcball_state_t *ab, float x, float y, float viewport_height);
void arcball_end_drag(arcball_state_t *ab);
void arcball_get_mvp(arcball_state_t *ab, mat4 dest, const vec2 pan_offset,
    float distance, float model_scale, float aspect, float fov_rad,
    float near_plane, float far_plane);
void arcball_add_callback(arcball_state_t *ab, arcball_callback_fn func, gpointer user_data);
void arcball_remove_callback(arcball_state_t *ab, arcball_callback_fn func, gpointer user_data);
void arcball_notify_changed(arcball_state_t *ab);
void arcball_copy_rotation(arcball_state_t *dst, const arcball_state_t *src);
void arcball_set_drag_mode(arcball_state_t *ab, arcball_drag_mode_t mode);
arcball_drag_mode_t arcball_get_drag_mode(arcball_state_t *ab);
void arcball_get_angles(arcball_state_t *ab, float *wr, float *wi);
int arcball_get_drag_button(arcball_state_t *ab);
void arcball_get_last_pos(arcball_state_t *ab, float *x, float *y);
void arcball_update_last_pos(arcball_state_t *ab, float x, float y);
void arcball_get_rotation_col(arcball_state_t *ab, int col, float out[3]);

#endif /* HAVE_OPENGL */
#endif /* OPENGL_ARCBALL_H */
