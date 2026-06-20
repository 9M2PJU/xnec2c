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

#include "prerender_state.h"
#include "../shared.h"

/* Singleton geometry-derived state (Tier 1) */
geom_pre_t    geom_pre;

/* Per-fstep prerender arrays (Tier 3) */
ff_pre_t     *ff_pre     = NULL;
nf_pre_t     *nf_pre     = NULL;

/*-----------------------------------------------------------------------*/

/**
 * free_ff_pre_step() - Release one ff_pre_t entry
 * @elem: pointer to one ff_pre_t element
 */
static void
free_ff_pre_step(void *elem)
{
  ff_pre_t *fp = elem;
  mem_free((void **)&fp->vertices);
  mem_free((void **)&fp->theta_rgb);
  mem_free((void **)&fp->phi_rgb);
  mem_free((void **)&fp->vertex_rgb);
}

/**
 * free_nf_pre_step() - Release one nf_pre_t entry
 * @elem: pointer to one nf_pre_t element
 */
static void
free_nf_pre_step(void *elem)
{
  nf_pre_t *np = elem;
  mem_free((void **)&np->e_vecs);
  mem_free((void **)&np->h_vecs);
  mem_free((void **)&np->pov_vecs);
  mem_free((void **)&np->pr_buf);
}

/*-----------------------------------------------------------------------*/

  void
prerender_state_alloc(int steps_total)
{
  if( steps_total <= 0 )
    return;

  size_t mreq;

  /* Resize each array, freeing only the shrink tail; surviving entries
   * keep their sub-buffers for reuse by the inner alloc loop. */
  mem_array_resize((void **)&ff_pre, sizeof(ff_pre_t),
      steps_total, free_ff_pre_step);
  mem_array_resize((void **)&nf_pre, sizeof(nf_pre_t),
      steps_total, free_nf_pre_step);

  /* Pre-allocate near-field inner arrays as pipe-read destinations.
   * PRead_Pipe writes directly into these pointers; they must be
   * allocated before Get_Freq_Data() is called on the parent side. */
  int npts = fpat.nrx * fpat.nry * fpat.nrz;
  int i;
  for( i = 0; i < steps_total; i++ )
  {
    if( npts > 0 )
    {
      mreq = (size_t)npts * sizeof(nf_vector_t);
      if( fpat.nfeh & NEAR_EFIELD )
        mem_alloc((void **)&nf_pre[i].e_vecs, mreq);
      if( fpat.nfeh & NEAR_HFIELD )
        mem_alloc((void **)&nf_pre[i].h_vecs, mreq);
      if( (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) )
        mem_alloc((void **)&nf_pre[i].pov_vecs, mreq);
    }
  }
}

/*-----------------------------------------------------------------------*/

  void
prerender_state_free(void)
{
  int i;

  if( ff_pre != NULL )
  {
    int n = mem_array_count(ff_pre, sizeof(ff_pre_t));
    for( i = 0; i < n; i++ )
      free_ff_pre_step(&ff_pre[i]);
    mem_free((void **)&ff_pre);
  }

  if( nf_pre != NULL )
  {
    int n = mem_array_count(nf_pre, sizeof(nf_pre_t));
    for( i = 0; i < n; i++ )
      free_nf_pre_step(&nf_pre[i]);
    mem_free((void **)&nf_pre);
  }

  mem_free((void **)&geom_pre.sin_theta);
  mem_free((void **)&geom_pre.cos_theta);
  mem_free((void **)&geom_pre.sin_phi);
  mem_free((void **)&geom_pre.cos_phi);
  mem_free((void **)&geom_pre.solid_angle);
  mem_free((void **)&geom_pre.theta_topo);
  mem_free((void **)&geom_pre.phi_topo);
  geom_pre.n_theta_edges = 0;
  geom_pre.n_phi_edges   = 0;

  /* patch_corners holds geometry computed at GE-card time. It is only
   * freed and reallocated by New_Patch_Data() on file reload; unlike the
   * RP/EX-card fields above, it must survive frequency-sweep restarts. */
}

/*-----------------------------------------------------------------------*/
