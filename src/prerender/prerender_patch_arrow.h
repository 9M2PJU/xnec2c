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

#ifndef PRERENDER_PATCH_ARROW_H
#define PRERENDER_PATCH_ARROW_H 1

#include "../common.h"

/*-----------------------------------------------------------------------
 * Arrow geometry in UV patch space (0..1 range), 80% of box.
 *
 * Template points to +U at angle=0.  Shaft rectangle spans
 * [TAIL_OFF..NECK_OFF] x [-SHAFT_HW..+SHAFT_HW] around center (0.5,0.5).
 * Arrowhead tip at TIP_OFF; head half-spread HEAD_HS.
 *----------------------------------------------------------------------*/

#define LINE_ARROW_TAIL_OFF    -0.24f
#define LINE_ARROW_NECK_OFF     0.04f
#define LINE_ARROW_TIP_OFF      0.28f
#define LINE_ARROW_SHAFT_HW     0.024f
#define LINE_ARROW_HEAD_HS      0.096f

#define ARROW_S0U (0.5f + LINE_ARROW_TAIL_OFF)
#define ARROW_S0V (0.5f + LINE_ARROW_SHAFT_HW)
#define ARROW_S1U (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_S1V (0.5f + LINE_ARROW_SHAFT_HW)
#define ARROW_S2U (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_S2V (0.5f - LINE_ARROW_SHAFT_HW)
#define ARROW_S3U (0.5f + LINE_ARROW_TAIL_OFF)
#define ARROW_S3V (0.5f - LINE_ARROW_SHAFT_HW)
#define ARROW_ALU (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_ALV (0.5f + LINE_ARROW_HEAD_HS)
#define ARROW_ARU (0.5f + LINE_ARROW_NECK_OFF)
#define ARROW_ARV (0.5f - LINE_ARROW_HEAD_HS)
#define ARROW_TPU (0.5f + LINE_ARROW_TIP_OFF)
#define ARROW_TPV 0.5f

#define ARROW_LINE_COUNT   7
#define ARROW_VERTEX_COUNT 14

/* Minimum flow magnitude ratio to render an arrow (avoids near-zero noise) */
#define FLOW_MAG_THRESHOLD 0.01f

/* Paired UV endpoints for one arrow line segment */
typedef struct
{
  float u1, v1, u2, v2;
} uv_segment_t;

/* Arrow template: 7 segments at angle=0 pointing +U.  Defined in prerender_patch_arrow.c. */
extern const uv_segment_t arrow_template[ARROW_LINE_COUNT];

/**
 * arrow_template_uv() - Return the u,v coordinates of one arrow vertex
 * @vertex: vertex index 0..(ARROW_VERTEX_COUNT-1)
 * @u:      output u coordinate
 * @v:      output v coordinate
 *
 * Even vertices (0,2,4,...) are segment start points (u1,v1);
 * odd vertices (1,3,5,...) are segment end points (u2,v2).
 */
static inline void
arrow_template_uv(int vertex, float *u, float *v)
{
  const uv_segment_t *seg = &arrow_template[vertex / 2];

  if( vertex & 1 )
  {
    *u = seg->u2;
    *v = seg->v2;
  }
  else
  {
    *u = seg->u1;
    *v = seg->v1;
  }
}

#endif /* PRERENDER_PATCH_ARROW_H */
