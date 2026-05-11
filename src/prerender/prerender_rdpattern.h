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

#ifndef PRERENDER_RDPATTERN_H
#define PRERENDER_RDPATTERN_H   1

#include "prerender_state.h"

/**
 * compute_excitation_center() - Centroid of excitation source segments
 *
 * Reads vsorc.isant/ivqd (populated by EX-card) to compute
 * geom_pre.excitation_cx/cy/cz. Called at end of Read_Commands(),
 * parent-only, after all cards including EX have been parsed.
 */
void compute_excitation_center(void);

/**
 * compute_trig_tables() - Populate geom_pre sin/cos/solid_angle tables
 *
 * Called from input.c RP-card block after SetFlag(ENABLE_RDPAT) and
 * fpat dimensions are finalized.  fpat.nth and fpat.nph must be valid.
 */
void compute_trig_tables(void);

/**
 * compute_ff_topology() - Populate geom_pre far-field edge topology tables
 *
 * Writes theta_topo[], phi_topo[], n_theta_edges, n_phi_edges from fpat grid.
 * Called once at RP-card time alongside compute_trig_tables(), parent-only.
 */
void compute_ff_topology(void);

/**
 * compute_nf_dr_norm() - Compute near-field normalization distance
 *
 * Reads fpat.near/dxnr/dynr/dznr (set by NE/NH cards) and writes
 * geom_pre.nf_dr_norm.  Called at RP-card time after NF cards are
 * processed, outside !CHILD so both parent and child populate the field.
 */
void compute_nf_dr_norm(void);

#endif
