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

#ifndef CHROMA_SOURCE_H
#define CHROMA_SOURCE_H  1

#include "../common.h"
#include "../color/color_edge.h"
#include "../color/color_tone.h"

/*-----------------------------------------------------------------------
 * Physical channel axis: which per-element quantity feeds a color
 * carrier.  Phasor channels fill re_n/im_n/arg beside env; the physics
 * scalars (SWR, far-field contribution) are phase-invariant and land in
 * env alone.
 *----------------------------------------------------------------------*/
typedef enum
{
  CHAN_CURRENT = 0,   /* wire segment or patch normal-component current */
  CHAN_CHARGE,        /* wire charge density (bir + j·bii) */
  CHAN_SWR,           /* per-region standing-wave ratio, [0,1] scalar */
  CHAN_FFCONTRIB,     /* per-element far-field contribution, [0,1] scalar */
  CHAN_NUM
} chroma_channel_t;

/* Element domain axis: wire segments or surface patches. */
typedef enum
{
  CHROMA_ELEM_WIRE = 0,
  CHROMA_ELEM_PATCH,
  CHROMA_ELEM_NUM
} chroma_elem_t;

/*-----------------------------------------------------------------------
 * Per-selected-frequency prepared source (SoA, single-buffered).
 *
 * All arrays are phase-invariant and normalized against the selected
 * step's peak envelope cmax; the per-frame combine reads them and
 * applies the animation phase.  The derivation-input edge snapshots
 * make prepare and family-apply idempotent per edge: a call with
 * matching inputs is a no-op, so no caller tracks staleness.  The
 * content generation gen advances on every refill, family reapply, and
 * release; frame producers key their own edges on it instead of copying
 * source determinants.
 *----------------------------------------------------------------------*/
typedef struct
{
  int      n;            /* element count: data.n wires or data.m patches */
  float   *env;          /* [n] channel scalar in [0,1]: |z|/cmax or physics value */
  float   *re_n;         /* [n] Re(z)/cmax; zero for scalar channels */
  float   *im_n;         /* [n] Im(z)/cmax; zero for scalar channels */
  float   *arg;          /* [n] arg z in radians; zero for scalar channels */
  float   *s_env;        /* [n] family transfer s(env), filled by apply_family */

  uint32_t     gen;      /* content generation; bumps on refill or reapply */
  color_edge_t prep;     /* prepare determinants; fstep -1 when empty */
  color_edge_t fam;      /* family-apply determinants; fam -1 marks s_env stale */
} chroma_source_t;

/**
 * chroma_source_supported() - Whether a channel exists for an element domain
 * @elem: element domain
 * @chan: physical channel
 *
 * Patches carry no charge or wire-run physics, so their only channel is
 * CHAN_CURRENT; callers fall back to CHAN_CURRENT for unsupported pairs.
 */
gboolean chroma_source_supported(chroma_elem_t elem, chroma_channel_t chan);

/**
 * chroma_source_prepare() - Fill the phase-invariant arrays for one channel
 * @cs:    source to fill; arrays are mem_array managed and resized here
 * @elem:  element domain selecting data.n or data.m sizing
 * @chan:  physical channel to gather
 * @fstep: selected frequency step
 *
 * No-op when the derivation inputs (fstep, freq_mhz, elem, chan, cmax,
 * count) match the prepared snapshot.  A refill marks s_env stale so the
 * next chroma_source_apply_family() recomputes it.
 */
void chroma_source_prepare(chroma_source_t *cs, chroma_elem_t elem,
    chroma_channel_t chan, int fstep);

/**
 * chroma_source_apply_family() - Fill s_env via the family transfer
 * @cs:  prepared source
 * @fam: scale family selection
 *
 * Applies color_tones[fam].transfer to every env entry.  No-op when
 * the family and its natural parameter match the applied snapshot and no
 * prepare intervened.
 */
void chroma_source_apply_family(chroma_source_t *cs, color_tone_t fam);

/**
 * chroma_seg_conn() - Decode one connection value to a 0-based neighbor index
 * @conn:     icon1/icon2 value (1-based, signed for orientation)
 * @self_num: this segment's 1-based number
 *
 * Returns -1 for a free end, a self reference, or a ground/junction
 * marker outside [1, data.n].  Shared with the glyph node marker.
 */
int chroma_seg_conn(int conn, int self_num);

/**
 * chroma_seg_linked() - Whether segment b's connectivity references segment a
 * @b:     0-based index of the candidate neighbor
 * @a_num: 1-based number of the referring segment
 *
 * Mutual-link confirmation: at a junction of three or more segments only
 * one neighbor pair is mutually referenced, so chains terminate there.
 */
gboolean chroma_seg_linked(int b, int a_num);

/**
 * chroma_source_free() - Release a source's managed arrays
 * @cs: source to release; safe on an empty source
 */
void chroma_source_free(chroma_source_t *cs);

#endif
