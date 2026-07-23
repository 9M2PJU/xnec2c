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
 * chroma_source: per-selected-frequency color source prepare.
 *
 * One owner for every phase-invariant per-element derivation feeding the
 * per-frame colorize: envelope, normalized phasor components, argument,
 * the family transfer of the envelope, and the physics scalars (per-run
 * SWR, far-field contribution).  Prepare runs on the fstep or channel
 * edge; apply_family runs on the family or parameter edge; both are
 * idempotent against their stored derivation inputs.
 */
#include "chroma_source.h"
#include "../prerender/prerender_color.h"
#include "../shared.h"

/* Standing-wave fit bounds: the window half-width in wavelengths, the
 * singular-basis determinant floor relative to len², and the wave-amplitude
 * noise floor below which the fit reads indeterminate. */
#define SWR_WINDOW_HALF_WL  0.5
#define RUN_FIT_DET_MIN     1e-9
#define RUN_FIT_AMP_MIN     1e-12

/* Coherent far-field power floor below which contributions read zero */
#define FFC_POWER_MIN       1e-30

static complex double z_wire_current(int fstep, int i);
static complex double z_wire_charge(int fstep, int i);
static complex double z_patch_current(int fstep, int i);
static void fill_swr(chroma_source_t *cs, int fstep);
static void fill_ffc(chroma_source_t *cs, int fstep);

/* One gather row: a phasor read plus its cmax slot, or a scalar fill.
 * Scalar physics rows derive from the wire currents, so their change
 * detector is the wire-current cmax. */
typedef struct
{
  complex double (*z_at)(int fstep, int i);          /* phasor rows */
  void (*fill_scalar)(chroma_source_t *cs, int fstep); /* physics rows */
  size_t cmax_offset;  /* offsetof into struct_colors_t: change detector */
} source_row_t;

/* Element-domain × channel gather table; an empty row marks an
 * unsupported pair (callers fall back via chroma_source_supported). */
static const source_row_t source_rows[CHROMA_ELEM_NUM][CHAN_NUM] =
{
  [CHROMA_ELEM_WIRE] = {
    [CHAN_CURRENT]   = { .z_at = z_wire_current,
      .cmax_offset = offsetof(struct_colors_t, wire_crnt_cmax) },
    [CHAN_CHARGE]    = { .z_at = z_wire_charge,
      .cmax_offset = offsetof(struct_colors_t, wire_chrg_cmax) },
    [CHAN_SWR]       = { .fill_scalar = fill_swr,
      .cmax_offset = offsetof(struct_colors_t, wire_crnt_cmax) },
    [CHAN_FFCONTRIB] = { .fill_scalar = fill_ffc,
      .cmax_offset = offsetof(struct_colors_t, wire_crnt_cmax) },
  },
  [CHROMA_ELEM_PATCH] = {
    [CHAN_CURRENT] = { .z_at = z_patch_current,
      .cmax_offset = offsetof(struct_colors_t, patch_crnt_cmax) },
  },
};

/*-----------------------------------------------------------------------*/

/**
 * z_wire_current() - Wire segment current phasor
 * @fstep: frequency step index
 * @i:     wire segment index
 */
static complex double
z_wire_current(int fstep, int i)
{
  return crnt_fstep[fstep].cur[i];
}

/**
 * z_wire_charge() - Wire charge-density phasor from the split arrays
 * @fstep: frequency step index
 * @i:     wire segment index
 */
static complex double
z_wire_charge(int fstep, int i)
{
  return crnt_fstep[fstep].bir[i] + I * crnt_fstep[fstep].bii[i];
}

/**
 * z_patch_current() - Patch normal-component current phasor
 * @fstep: frequency step index
 * @i:     patch index (0-based into data.m)
 */
static complex double
z_patch_current(int fstep, int i)
{
  return crnt_fstep[fstep].cur[data.n + 3 * i];
}

/*-----------------------------------------------------------------------*/

/**
 * chroma_seg_conn() - Decode one connection value to a 0-based neighbor index
 * @conn:     icon1/icon2 value (1-based, signed for orientation)
 * @self_num: this segment's 1-based number
 *
 * Returns -1 for a free end, a self reference, or a ground/junction
 * marker outside [1, data.n].
 */
  int
chroma_seg_conn(int conn, int self_num)
{
  int j = abs(conn);

  if( j == 0 || j == self_num || j > data.n )
    return -1;

  return j - 1;
}

/**
 * chroma_seg_linked() - Whether segment b's connectivity references segment a
 * @b:     0-based index of the candidate neighbor
 * @a_num: 1-based number of the referring segment
 *
 * Mutual-link confirmation: at a junction of three or more segments only
 * one neighbor pair is mutually referenced, so chains terminate there.
 */
  gboolean
chroma_seg_linked(int b, int a_num)
{
  return abs(data.segments[b].icon1) == a_num
      || abs(data.segments[b].icon2) == a_num;
}

/**
 * run_rho() - Fit forward/backward waves over one run, return |Γ|
 * @run: 0-based segment indices along the run, in chain order
 * @len: run length
 * @cur: wire current phasors for the selected step
 *
 * Least-squares fit of I(s) = A·e^{-jks} + B·e^{+jks} with s the
 * cumulative midpoint position in wavelengths (k = 2π; NEC geometry is
 * wavelength-normalized).  Returns min(|A|,|B|) / max(|A|,|B|) in [0,1]:
 * 0 traveling, 1 standing.  Returns NAN when the fit cannot resolve a
 * wave pair: fewer than four segments, a singular basis, or currents
 * below the noise floor.  The s origin is arbitrary: shifting it only
 * rotates A and B in phase, leaving the magnitude ratio unchanged.
 */
static double
run_rho(const int *run, int len, const complex double *cur)
{
  complex double suv = 0.0;  /* Σ e^{+2jks}: basis cross term */
  complex double rui = 0.0;  /* Σ e^{+jks}·I */
  complex double rvi = 0.0;  /* Σ e^{-jks}·I */
  complex double a, b;
  double s = 0.0;
  double det, af, ab_, hi, lo;
  int m;

  if( len < 4 )
    return NAN;

  for( m = 0; m < len; m++ )
  {
    double half = 0.5 * data.segments[run[m]].si;
    double ks;
    complex double u;

    s += half;
    ks = M_2PI * s;
    s += half;

    u = cexp(I * ks);
    suv += u * u;
    rui += u * cur[run[m]];
    rvi += conj(u) * cur[run[m]];
  }

  det = (double)len * (double)len - creal(suv * conj(suv));
  if( det <= RUN_FIT_DET_MIN * (double)len * (double)len )
    return NAN;

  a = ((double)len * rui - suv * rvi) / det;
  b = ((double)len * rvi - conj(suv) * rui) / det;

  af  = cabs(a);
  ab_ = cabs(b);
  hi  = fmax(af, ab_);
  lo  = fmin(af, ab_);

  if( hi <= RUN_FIT_AMP_MIN )
    return NAN;

  return lo / hi;
}

/**
 * fill_swr() - Fill env with a windowed per-segment standing-wave scalar
 * @cs:    source whose env array is sized [data.n]
 * @fstep: frequency step index
 *
 * Runs are maximal mutually-linked chains through icon1/icon2,
 * terminated at free ends, grounds, and junctions of three or more
 * segments; a closed loop walks once around, so its window clamps at
 * the walk seam.  Each segment fits the wave pair over a ±0.5λ
 * wire-distance window centered on it, so long runs (a helix winding)
 * resolve spatial variation instead of one whole-run average.  An
 * unresolvable window (short grid stubs between junctions, singular
 * fits) reads mid-scale as indeterminate, never as standing.
 */
static void
fill_swr(chroma_source_t *cs, int fstep)
{
  const complex double *cur = crnt_fstep[fstep].cur;
  char   *visited = NULL;
  int    *run = NULL;
  double *pos = NULL;
  int i;

  mem_alloc(&visited, (size_t)data.n);
  mem_array_alloc(&run, data.n);
  mem_array_alloc(&pos, data.n);

  for( i = 0; i < data.n; i++ )
  {
    int head = i;
    int steps, len, m;
    double s;

    if( visited[i] )
      continue;

    /* Walk backward to the run head; a loop returns to i and stops */
    for( steps = 0; steps < data.n; steps++ )
    {
      int prev = chroma_seg_conn(data.segments[head].icon1, head + 1);

      if( prev < 0 || visited[prev] || !chroma_seg_linked(prev, head + 1)
          || prev == i )
        break;

      head = prev;
    }

    /* Walk forward from the head, collecting the run */
    len = 0;
    for( steps = 0; steps < data.n; steps++ )
    {
      int next;

      run[len++] = head;
      visited[head] = 1;

      next = chroma_seg_conn(data.segments[head].icon2, head + 1);
      if( next < 0 || visited[next] || !chroma_seg_linked(next, head + 1) )
        break;

      head = next;
    }

    /* Cumulative midpoint positions along the run in wavelengths */
    s = 0.0;
    for( m = 0; m < len; m++ )
    {
      double half = 0.5 * data.segments[run[m]].si;

      s += half;
      pos[m] = s;
      s += half;
    }

    for( m = 0; m < len; m++ )
    {
      int lo = m, hi = m;
      double rho;

      /* ±0.5λ wire-distance window, clamped at the run ends */
      while( lo > 0 && pos[m] - pos[lo - 1] <= SWR_WINDOW_HALF_WL )
        lo--;
      while( hi < len - 1 && pos[hi + 1] - pos[m] <= SWR_WINDOW_HALF_WL )
        hi++;

      rho = run_rho(run + lo, hi - lo + 1, cur);
      cs->env[run[m]] = isnan(rho) ? 0.5f : (float)rho;
    }
  }

  mem_free(&visited);
  mem_array_free(&run);
  mem_array_free(&pos);
}

/**
 * fill_ffc() - Fill env with the per-segment far-field contribution
 * @cs:    source whose env array is sized [data.n]
 * @fstep: frequency step index
 *
 * Coherent decomposition toward the selected step's main-lobe direction:
 * e_i = I_i·L_i·sinθ_i·e^{j·2π·(r̂·p_i)} with θ_i the angle between the
 * segment axis and r̂; contribution_i = Re(e_i·conj(E)) / |E|² clamped
 * to [0,1] and normalized by its peak.  Without radiation-pattern data
 * the channel reads cold (all zero).
 */
static void
fill_ffc(chroma_source_t *cs, int fstep)
{
  const complex double *cur = crnt_fstep[fstep].cur;
  complex double e_tot = 0.0;
  double tht, phi, rx, ry, rz;
  double e_pw, peak;
  int pol = calc_data.pol_type;
  int i;

  mem_array_zero(cs->env);

  if( rad_pattern == NULL || rad_pattern[fstep].max_gain_tht == NULL )
  {
    pr_notice("far-field contribution needs radiation pattern data\n");
    return;
  }

  tht = rad_pattern[fstep].max_gain_tht[pol] * TORAD;
  phi = rad_pattern[fstep].max_gain_phi[pol] * TORAD;
  rx  = sin(tht) * cos(phi);
  ry  = sin(tht) * sin(phi);
  rz  = cos(tht);

  for( i = 0; i < data.n; i++ )
  {
    const wire_segment_t *sg = &data.segments[i];
    double dot = sg->cab * rx + sg->sab * ry + sg->salp * rz;
    double sin_t = sqrt(fmax(0.0, 1.0 - dot * dot));
    double ks = M_2PI * (rx * sg->x + ry * sg->y + rz * sg->z);
    complex double e_i = cur[i] * sg->si * sin_t * cexp(I * ks);

    /* Stage the element phasor in re_n/im_n until the second pass */
    cs->re_n[i] = (float)creal(e_i);
    cs->im_n[i] = (float)cimag(e_i);
    e_tot += e_i;
  }

  e_pw = creal(e_tot * conj(e_tot));
  if( e_pw <= FFC_POWER_MIN )
    return;

  peak = 0.0;
  for( i = 0; i < data.n; i++ )
  {
    complex double e_i = (double)cs->re_n[i] + I * (double)cs->im_n[i];
    double c = creal(e_i * conj(e_tot)) / e_pw;

    c = fmin(fmax(c, 0.0), 1.0);
    cs->env[i] = (float)c;
    if( c > peak )
      peak = c;
  }

  if( peak > 0.0 )
  {
    for( i = 0; i < data.n; i++ )
      cs->env[i] = (float)((double)cs->env[i] / peak);
  }
}

/*-----------------------------------------------------------------------*/

  gboolean
chroma_source_supported(chroma_elem_t elem, chroma_channel_t chan)
{
  const source_row_t *row = &source_rows[elem][chan];

  return row->z_at != NULL || row->fill_scalar != NULL;
}

/*-----------------------------------------------------------------------*/

  void
chroma_source_prepare(chroma_source_t *cs, chroma_elem_t elem,
    chroma_channel_t chan, int fstep)
{
  const source_row_t *row = &source_rows[elem][chan];
  int n = (elem == CHROMA_ELEM_WIRE) ? data.n : data.m;
  double cmax;
  color_edge_t want;
  int i;

  if( !chroma_source_supported(elem, chan) )
  {
    BUG("unsupported color source elem %d chan %d\n", elem, chan);
    return;
  }

  if( n <= 0 || struct_colors == NULL || !CRNT_FSTEP_AVAILABLE(fstep) )
  {
    cs->n = 0;
    cs->prep = (color_edge_t){ .fstep = -1, .fam = -1 };
    cs->gen++;
    return;
  }

  cmax = (double)*(const float *)
      ((const char *)&struct_colors[fstep] + row->cmax_offset);

  /* Prepare edge: refill only when a derivation input changed */
  want = (color_edge_t){ .fstep = fstep, .elem = (int)elem,
      .chan = (int)chan, .fam = -1,
      .freq_mhz = calc_data.freq_mhz, .cmax = cmax };
  if( cs->n == n && color_edge_eq(&cs->prep, &want) )
    return;

  mem_array_realloc(&cs->env, n);
  mem_array_realloc(&cs->re_n, n);
  mem_array_realloc(&cs->im_n, n);
  mem_array_realloc(&cs->arg, n);
  mem_array_realloc(&cs->s_env, n);

  if( row->z_at != NULL )
  {
    double scale = (cmax > 0.0) ? 1.0 / cmax : 0.0;

    for( i = 0; i < n; i++ )
    {
      complex double z = row->z_at(fstep, i);
      double m = cabs(z);

      cs->env[i]  = (float)(m * scale);
      cs->re_n[i] = (float)(creal(z) * scale);
      cs->im_n[i] = (float)(cimag(z) * scale);
      cs->arg[i]  = (m > 0.0) ? (float)carg(z) : 0.0f;
    }
  }
  else
  {
    /* Physics scalars are phase-invariant: no phasor components */
    row->fill_scalar(cs, fstep);
    mem_array_zero(cs->re_n);
    mem_array_zero(cs->im_n);
    mem_array_zero(cs->arg);
  }

  cs->n    = n;
  cs->prep = want;

  /* New envelope data invalidates the applied family transfer */
  cs->fam = (color_edge_t){ .fstep = -1, .fam = -1 };
  cs->gen++;
}

/*-----------------------------------------------------------------------*/

  void
chroma_source_apply_family(chroma_source_t *cs, color_tone_t fam)
{
  const color_tone_row_t *row = &color_tones[fam];
  color_edge_t want;
  tone_param_t tp;
  int i;

  if( cs->n <= 0 )
    return;

  tone_param_init(&tp, fam);

  /* Apply edge: recompute only when family, parameter, or env changed */
  want = (color_edge_t){ .fstep = -1, .fam = (int)fam, .param = tp.param };
  if( color_edge_eq(&cs->fam, &want) )
    return;

  for( i = 0; i < cs->n; i++ )
    cs->s_env[i] = (float)row->transfer((double)cs->env[i], &tp);

  cs->fam = want;
  cs->gen++;
}

/*-----------------------------------------------------------------------*/

  void
chroma_source_free(chroma_source_t *cs)
{
  mem_array_free(&cs->env);
  mem_array_free(&cs->re_n);
  mem_array_free(&cs->im_n);
  mem_array_free(&cs->arg);
  mem_array_free(&cs->s_env);

  cs->n = 0;
  cs->prep = (color_edge_t){ .fstep = -1, .fam = -1 };
  cs->fam  = (color_edge_t){ .fstep = -1, .fam = -1 };
  cs->gen++;
}

/*-----------------------------------------------------------------------*/
