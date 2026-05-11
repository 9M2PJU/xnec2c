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

#ifndef PRERENDER_STATE_H
#define PRERENDER_STATE_H       1

#include "../common.h"
#include "prerender_color.h"

/*-----------------------------------------------------------------------
 * Shared geometry types
 *----------------------------------------------------------------------*/

/* 3D point with radial distance, used for radiation pattern vertices */
typedef struct
{
  double x, y, z, r;
} point_3d_t;

/*-----------------------------------------------------------------------
 * Tier 1 — Grid topology (frequency-independent)
 *----------------------------------------------------------------------*/

/* Far-field edge topology entry.  Indices into the per-fstep vertex array.
 * Color is stored separately in ff_pre_t.theta_rgb / phi_rgb (Tier 3). */
typedef struct
{
  uint32_t va, vb;
} ff_edge_topo_t;

/*-----------------------------------------------------------------------
 * Tier 1 — Geometry-derived (frequency-independent)
 *----------------------------------------------------------------------*/

/* Geometry-derived aggregate state, computed once at file load. */
typedef struct
{
  double  scene_radius;
  double  excitation_cx;
  double  excitation_cy;
  double  excitation_cz;
  double  nf_dr_norm;

  /* Radiation pattern grid trig tables [fpat.nth] / [fpat.nph] */
  double *sin_theta;    /* [fpat.nth] */
  double *cos_theta;    /* [fpat.nth] */
  double *sin_phi;      /* [fpat.nph] */
  double *cos_phi;      /* [fpat.nph] */
  double *solid_angle;  /* [fpat.nth] = fabs(sin(theta_i)) * dth_rad * dph_rad */

  /* Far-field edge topology (frequency-independent grid connectivity) */
  ff_edge_topo_t *theta_topo;   /* [(fpat.nth-1) * fpat.nph] */
  ff_edge_topo_t *phi_topo;     /* [fpat.nth * (fpat.nph-1)] */
  int             n_theta_edges;
  int             n_phi_edges;

  /* Precomputed patch rectangle corners [data.m] */
  patch_corners_t *patch_corners;
} geom_pre_t;

/*-----------------------------------------------------------------------
 * Tier 3 — Per-frequency prerender
 *----------------------------------------------------------------------*/

/* Per-frequency far-field prerender.
 * Grid dimensions: fpat.nth / fpat.nph (Tier 0, not stored).
 * Topology (va/vb) is Tier 1 in geom_pre.theta_topo / phi_topo.
 * Colors are parallel arrays indexed identically to those topo arrays. */
typedef struct
{
  float       pattern_radius;
  float       r_min;
  float       overlay_base_scale; /* Structure-to-pattern scale without scale_adj */
  uint32_t    generation;     /* Incremented by ff_presentation_recompute() */
  point_3d_t *vertices;       /* [fpat.nth * fpat.nph] */
  rgb_f_t    *theta_rgb;      /* [geom_pre.n_theta_edges] */
  rgb_f_t    *phi_rgb;        /* [geom_pre.n_phi_edges] */
} ff_pre_t;

/* Near-field vector displacement from sample origin.
 * Origin coordinates in near_field_fstep[fstep].points[i] (Tier 2). */
typedef struct
{
  float dx, dy, dz;
  float rgb[3];
} nf_vector_t;

/* Per-frequency near-field prerender.
 * Sample count = fpat.nrx * fpat.nry * fpat.nrz (Tier 0, not stored). */
typedef struct
{
  float        pov_max;
  nf_vector_t *e_vecs;        /* [npts] E-field, NULL if absent */
  nf_vector_t *h_vecs;        /* [npts] H-field, NULL if absent */
  nf_vector_t *pov_vecs;      /* [npts] Poynting, NULL if E or H absent */
  float       *pr_buf;        /* [npts] Poynting magnitude scratch (persistent) */
} nf_pre_t;

/*-----------------------------------------------------------------------
 * Global prerender state
 *----------------------------------------------------------------------*/

extern geom_pre_t    geom_pre;
extern ff_pre_t     *ff_pre;
extern nf_pre_t     *nf_pre;

/*-----------------------------------------------------------------------
 * Lifecycle
 *----------------------------------------------------------------------*/

/**
 * prerender_state_alloc() - Allocate per-fstep prerender arrays
 * @steps_total: number of frequency steps
 */
void prerender_state_alloc(int steps_total);

/**
 * prerender_state_free() - Release all prerender storage
 */
void prerender_state_free(void);

#endif
