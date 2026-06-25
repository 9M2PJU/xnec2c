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
 * prerender_nearfield: Tier 3 per-frequency near-field prerender.
 *
 * Computes E-field, H-field, and Poynting vector displacements with
 * baked RGB color.  The Poynting cross-product runs as a single O(N) pass.
 */
#include "prerender_nearfield.h"
#include "prerender_color.h"
#include "prerender_aggregate.h"
#include "../shared.h"

/*-----------------------------------------------------------------------*/

/**
 * prerender_field_vectors() - Compute scaled displacement and color per sample
 * @vecs:   output array [npts], allocated by caller
 * @nf:     near-field data for this fstep
 * @npts:   sample count
 * @dr:     normalization distance (geom_pre.nf_dr_norm)
 * @e_mode: TRUE for E-field, FALSE for H-field
 */
static void
prerender_field_vectors(nf_vector_t *vecs, near_field_t *nf,
    int npts, double dr, gboolean e_mode)
{
  int idx;
  double max_val = e_mode ? nf->max_er : nf->max_hr;

  for( idx = 0; idx < npts; idx++ )
  {
    double mag, rx, ry, rz, fscale;
    double rd, gn, bl;

    if( e_mode )
    {
      mag = nf->points[idx].er;
      rx  = nf->points[idx].erx;
      ry  = nf->points[idx].ery;
      rz  = nf->points[idx].erz;
    }
    else
    {
      mag = nf->points[idx].hr;
      rx  = nf->points[idx].hrx;
      ry  = nf->points[idx].hry;
      rz  = nf->points[idx].hrz;
    }

    /* Scale factor to equalize line segment lengths */
    fscale = dr / mag;

    vecs[idx].dx = (float)(rx * fscale);
    vecs[idx].dy = (float)(ry * fscale);
    vecs[idx].dz = (float)(rz * fscale);

    Value_to_Color(&rd, &gn, &bl, mag, max_val);
    vecs[idx].rgb[0] = (float)rd;
    vecs[idx].rgb[1] = (float)gn;
    vecs[idx].rgb[2] = (float)bl;
  }
}

/*-----------------------------------------------------------------------*/

  void
Prerender_Near_Field(int fstep)
{
  int idx, npts;
  nf_pre_t *np = &nf_pre[fstep];

  if( near_field_fstep == NULL || near_field_fstep[fstep].points == NULL )
    return;

  near_field_t *nf = &near_field_fstep[fstep];
  npts = fpat.nrx * fpat.nry * fpat.nrz;
  double dr = geom_pre.nf_dr_norm;

  /* E-field vectors */
  if( fpat.nfeh & NEAR_EFIELD )
  {
    mem_array_realloc(&np->e_vecs, npts);
    prerender_field_vectors(np->e_vecs, nf, npts, dr, TRUE);
  }

  /* H-field vectors */
  if( fpat.nfeh & NEAR_HFIELD )
  {
    mem_array_realloc(&np->h_vecs, npts);
    prerender_field_vectors(np->h_vecs, nf, npts, dr, FALSE);
  }

  /* Poynting vectors (E×H cross product). Requires both E and H field data.
   * Pass 1: compute cross-product magnitudes and displacements, scan pov_max.
   * Pass 2: bake colors from magnitude buffer. */
  if( (fpat.nfeh & NEAR_EFIELD) && (fpat.nfeh & NEAR_HFIELD) )
  {
    mem_array_realloc(&np->pov_vecs, npts);
    mem_array_realloc(&np->pr_buf, npts);

    np->pov_max = 0.0f;

    for( idx = 0; idx < npts; idx++ )
    {
      double px = nf->points[idx].ery * nf->points[idx].hrz -
                  nf->points[idx].hry * nf->points[idx].erz;
      double py = nf->points[idx].erz * nf->points[idx].hrx -
                  nf->points[idx].hrz * nf->points[idx].erx;
      double pz = nf->points[idx].erx * nf->points[idx].hry -
                  nf->points[idx].hrx * nf->points[idx].ery;
      double pr = sqrt(px * px + py * py + pz * pz);
      double fscale = dr / pr;

      np->pov_vecs[idx].dx = (float)(px * fscale);
      np->pov_vecs[idx].dy = (float)(py * fscale);
      np->pov_vecs[idx].dz = (float)(pz * fscale);

      np->pr_buf[idx] = (float)pr;
      if( np->pr_buf[idx] > np->pov_max )
        np->pov_max = np->pr_buf[idx];
    }

    for( idx = 0; idx < npts; idx++ )
    {
      double rd, gn, bl;
      Value_to_Color(&rd, &gn, &bl, (double)np->pr_buf[idx], (double)np->pov_max);
      np->pov_vecs[idx].rgb[0] = (float)rd;
      np->pov_vecs[idx].rgb[1] = (float)gn;
      np->pov_vecs[idx].rgb[2] = (float)bl;
    }

  }
}

/*-----------------------------------------------------------------------*/
