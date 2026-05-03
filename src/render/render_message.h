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

#ifndef RENDER_MESSAGE_H
#define RENDER_MESSAGE_H        1

#include "../common.h"

/* Status messages emitted when rendering preconditions are not met.
 * Single source of truth: both Cairo and OpenGL use these constants. */
#define STATUS_MSG_OPEN_FILE        "File ▸ Open to load an NEC file"
#define STATUS_MSG_NO_RP_CARD       "Gain pattern requires an RP card in the NEC file"
#define STATUS_MSG_NO_NEAREH_CARDS  "Near field requires NE or NH cards in the NEC file"
#define STATUS_MSG_NO_RP_NO_NEAREH  "No RP or NE/NH cards in the NEC file"
#define STATUS_MSG_SELECT_NEARFIELD "Select Near Field (no RP card for gain pattern)"
#define STATUS_MSG_SELECT_GAINPAT   "Select Gain Pattern (no NE/NH cards for near field)"
#define STATUS_MSG_SELECT_MODE      "Select Gain Pattern or Near Field"
#define STATUS_MSG_START_FREQLOOP   "Press ▶ to start the frequency loop"
#define STATUS_MSG_NOT_READY        "Radiation pattern data not ready"

/**
 * Draw_Centered_Message() - Render a centered status message on a Cairo surface
 * @cr:     Cairo context
 * @width:  drawable width in pixels
 * @height: drawable height in pixels
 * @msg:    UTF-8 message string
 */
void Draw_Centered_Message(cairo_t *cr, int width, int height, const char *msg);

#endif
