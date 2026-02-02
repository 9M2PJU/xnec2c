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

#ifndef DRAW_H
#define DRAW_H      1

#include "common.h"

/* Segment color classification for structure rendering */
typedef enum
{
  SEG_COLOR_NORMAL = 0,     /* Unloaded wire - BLUE */
  SEG_COLOR_LOADED,         /* Any load type - YELLOW */
  SEG_COLOR_EXCITATION      /* Voltage/current source - RED */

} segment_color_type_t;

/* Determine color type for segment (1-indexed segment number) */
segment_color_type_t get_segment_color_type(int seg_num);

/* Convert segment color type to RGB values */
void segment_type_to_rgb(segment_color_type_t type, float *r, float *g, float *b);

#endif

