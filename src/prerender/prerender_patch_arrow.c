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

/*
 * prerender_patch_arrow: Arrow UV template shape constants and segment table.
 *
 * arrow_template is the single authoritative source of arrow geometry.
 * Both GL and Cairo backends reference this array; neither defines its own.
 *
 * Arrow points +U at angle=0.  Shaft rectangle spans
 * [TAIL_OFF..NECK_OFF] x [-SHAFT_HW..+SHAFT_HW] around center (0.5, 0.5).
 * Arrowhead tip at TIP_OFF; head half-spread HEAD_HS.
 *
 *   s0────────s1
 *   │     ┌────a_left
 *   │     │   ╱
 *   │     │  tip
 *   │     │   ╲
 *   │     └────a_right
 *   s3────────s2
 */
#include "prerender_patch_arrow.h"

const uv_segment_t arrow_template[ARROW_LINE_COUNT] = {
  { ARROW_S0U, ARROW_S0V, ARROW_S1U, ARROW_S1V },  /* shaft top */
  { ARROW_S3U, ARROW_S3V, ARROW_S2U, ARROW_S2V },  /* shaft bottom */
  { ARROW_S0U, ARROW_S0V, ARROW_S3U, ARROW_S3V },  /* tail cap */
  { ARROW_S1U, ARROW_S1V, ARROW_ALU, ARROW_ALV },  /* top notch */
  { ARROW_ALU, ARROW_ALV, ARROW_TPU, ARROW_TPV },  /* top arm */
  { ARROW_S2U, ARROW_S2V, ARROW_ARU, ARROW_ARV },  /* bottom notch */
  { ARROW_ARU, ARROW_ARV, ARROW_TPU, ARROW_TPV },  /* bottom arm */
};
