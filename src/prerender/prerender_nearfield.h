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

#ifndef PRERENDER_NEARFIELD_H
#define PRERENDER_NEARFIELD_H   1

#include "prerender_state.h"

/**
 * Prerender_Near_Field() - Tier 3 per-frequency near-field prerender
 * @fstep: frequency step index
 *
 * Populates nf_pre[fstep]: E-field, H-field, and Poynting vector
 * displacements with baked RGB color.  Poynting cross-product runs
 * as a single O(N) pass.
 */
void Prerender_Near_Field(int fstep);

#endif
