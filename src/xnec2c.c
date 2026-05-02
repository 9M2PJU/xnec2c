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

#include <pthread.h>

#include "xnec2c.h"
#include "callbacks.h"
#include "shared.h"
#include "measurements.h"
#include "prerender/prerender_color.h"
#include "prerender/prerender_farfield.h"
#include "prerender/prerender_nearfield.h"
#include "mathlib.h"
#include "opt_ui.h"
#include "plot_freqdata.h"
/* Only nec2_eval_signal() is called from xnec2c.c; avoid pulling
 * gsl headers (via opt_simple.h) which conflict with openblas cblas. */
extern void nec2_eval_signal(void);
/* opengl_structure_invalidate() is called here; forward-declared to avoid
 * pulling opengl_structure.h (and its GL/GLEW chain) into the NEC engine file. */
extern void opengl_structure_invalidate(void);

static pthread_t *pth_freq_loop = NULL;

/* Left-overs from fortran code :-( */
static double tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

/*-----------------------------------------------------------------------*/

/* Set the calc_data.freq_step if it matches calc_data.freq_mhz.
 * Redo radiation pattern for a new frequency.
 *
 * If it doesn't, return 0 so the caller can run New_Frequency() and
 * use the extra buffer (in rad_pattern and other structures). */
int set_freq_step(void)
{
	int fr, step;
	double freq;

	int prev_freq_step = calc_data.freq_step;

	int idx = 0;
	int found = 0;
	for (fr = 0; !found && fr < calc_data.FR_cards && save.fstep[idx]; fr++)
	{
		freq = calc_data.freq_loop_data[fr].min_freq;
		for (step = 0; !found && step < calc_data.freq_loop_data[fr].freq_steps && save.fstep[idx]; step++)
		{
			if( FREQ_EQ(calc_data.freq_mhz, freq) )
			{
				calc_data.freq_step = idx;
				found = 1;
			}
			else
			{
				if (calc_data.freq_loop_data[fr].ifreq == 1)
					freq *= calc_data.freq_loop_data[fr].delta_freq;
				else
					freq += calc_data.freq_loop_data[fr].delta_freq;

				idx++;
			}

		}
	}

	// If we didn't find the frequency, then use the "extra" frequency
	// allocated as +1 at the end of all per-frequency data indexes:
	if (!found)
	{
		calc_data.freq_step = calc_data.steps_total;

		/* save.freq[steps_total] is written only by freq_loop_dispatch;
		 * CRNT_FSTEP_AVAILABLE guards consumers until data is valid. */
	}

	if (calc_data.freq_step != prev_freq_step)
		SetFlag( DRAW_NEW_RDPAT );

	// If we found the index, then no need to re-run New_Frequency because it is
	// in the index.
	return found;
}

/* Forward declaration: defined after the freq-loop helpers below */
void freq_step_update_ui( int new_step, gboolean force );

/**
 * fetch_freq_data - retrieve frequency data from cache or dispatch computation
 *
 * Sets calc_data.freq_step to the matching sweep index when freq_mhz matches
 * a cached FR-card step.  If the extra slot already holds valid data for this
 * frequency, returns immediately.  Otherwise starts the frequency loop to
 * compute the extra slot via the child dispatch path.
 *
 * Returns: TRUE when cached data is available and the caller may redraw;
 *          FALSE when computation has been dispatched (redraws follow on
 *          completion via freq_loop_finalize/redraws).
 */
gboolean
fetch_freq_data( void )
{
  /* No data loaded yet; save.freq is unallocated */
  if( save.freq == NULL )
    return FALSE;

  g_rec_mutex_lock(&freq_data_lock);

  if( set_freq_step() )
  {
    freq_step_update_ui( calc_data.freq_step, TRUE );
    g_rec_mutex_unlock(&freq_data_lock);
    return TRUE;
  }

  if( save.fstep[calc_data.steps_total] &&
      FREQ_EQ(save.freq[calc_data.steps_total], calc_data.freq_mhz) )
  {
    freq_step_update_ui( calc_data.steps_total, TRUE );
    g_rec_mutex_unlock(&freq_data_lock);
    return TRUE;
  }

  g_rec_mutex_unlock(&freq_data_lock);
  return FALSE;
}

/**
 * freq_display_update - record user-selected frequency and refresh display
 * @fmhz: frequency in MHz selected by the user
 *
 * Sets fmhz_save and calls opt_ui_update_values.  Does not trigger NEC2
 * computation.  Called by user_set_frequency before dispatching computation,
 * and directly by the spinbutton handler when Apply is unchecked.
 */
void
freq_display_update( double fmhz )
{
  calc_data.fmhz_save = fmhz;
  opt_ui_update_values();
}

/**
 * user_set_frequency - apply a user-selected frequency and trigger computation
 * @fmhz:  frequency in MHz chosen by the user
 *
 * Updates display state via freq_display_update then dispatches NEC2
 * computation.  On a cache hit, freq_step_update_ui refreshes the UI with
 * computed data.  Apply checkbox logic is handled by the caller — this
 * function always computes.
 */
void
user_set_frequency( double fmhz )
{
  freq_display_update( fmhz );
  calc_data.freq_mhz = fmhz;
  if( !fetch_freq_data() )
    Start_Frequency_Loop_Greenline();
}

/* Frequency_Scale_Geometry()
 *
 * Scales geometric parameters to frequency
 */
  void
Frequency_Scale_Geometry(void)
{
  double fr;
  int idx;

  /* Calculate wavelength */
  data.wlam= CVEL / calc_data.freq_mhz;

  /* frequency scaling of geometric parameters */
  fr = calc_data.freq_mhz / CVEL;
  if( data.n != 0)
  {
    for( idx = 0; idx < data.n; idx++ )
    {
      data.segments[idx].x = save.xtemp[idx] * fr;
      data.segments[idx].y = save.ytemp[idx] * fr;
      data.segments[idx].z = save.ztemp[idx] * fr;
      data.segments[idx].si = save.sitemp[idx]* fr;
      data.segments[idx].bi = save.bitemp[idx]* fr;
    }
  }

  if( data.m != 0)
  {
    double fr2= fr* fr;
    for( idx = 0; idx < data.m; idx++ )
    {
      int j;

      j = idx + data.n;
      data.patches[idx].px = save.xtemp[j] * fr;
      data.patches[idx].py = save.ytemp[j] * fr;
      data.patches[idx].pz = save.ztemp[j] * fr;
      data.patches[idx].pbi = save.bitemp[j]* fr2;
    }
  }

} /* Frequency_Scale_Geometry() */

/*-----------------------------------------------------------------------*/

/* Struct_Impedance_Loading()
 *
 * Calculates structure (segment) impedance loading
 */
  static void
Structure_Impedance_Loading( void )
{
  /* Calculate some loading parameters */
  if( zload.nload != 0)
    load(
        calc_data.ldtyp,  calc_data.ldtag,
        calc_data.ldtagf, calc_data.ldtagt,
        calc_data.zlr,    calc_data.zli,
        calc_data.zlc );

} /* Struct_Impedance_Loading() */

/*-----------------------------------------------------------------------*/

/* Ground_Parameters()
 *
 * Calculates ground parameters (antenna environment)
 */
  static void
Ground_Parameters( void )
{
  complex double epsc;

  if( gnd.ksymp != 1)
  {
    gnd.frati = CPLX_10;

    if( gnd.iperf != 1)
    {
      if( save.sig < 0.0 )
        save.sig = -save.sig / (59.96 * data.wlam);

      epsc = cmplx( save.epsr, -save.sig * data.wlam * 59.96 );
      gnd.zrati = 1.0 / csqrt( epsc);
      gwav.u = gnd.zrati;
      gwav.u2 = gwav.u * gwav.u;

      if( gnd.nradl > 0 )
      {
        gnd.scrwl = save.scrwlt / data.wlam;
        gnd.scrwr = save.scrwrt / data.wlam;
        gnd.t1 = CPLX_01 * 2367.067/ (double)gnd.nradl;
        gnd.t2 = gnd.scrwr * (double)gnd.nradl;
      } /* if( gnd.nradl > 0 ) */

      if( gnd.iperf == 2)
      {
        somnec( save.epsr, save.sig, calc_data.freq_mhz );
        gnd.frati =( epsc - 1.0) / ( epsc + 1.0);
        if( cabs(( ggrid.epscf - epsc) / epsc) >= 1.0e-3 )
        {
          pr_err("complex dielectric constant from file: %12.5E%+12.5Ej, requested: %12.5E%+12.5Ej\n",
                 creal(ggrid.epscf), cimag(ggrid.epscf),
				 creal(epsc), cimag(epsc));
          Stop( ERR_STOP, _("Ground_Parameters():"
                "Error in ground parameters") );
        }
      } /* if( gnd.iperf != 2) */
    } /* if( gnd.iperf != 1) */
    else
    {
      gnd.scrwl = 0.0;
      gnd.scrwr = 0.0;
      gnd.t1 = 0.0;
      gnd.t2 = 0.0;
    }
  } /* if( gnd.ksymp != 1) */

  return;
} /* Ground_Parameters() */

/*-----------------------------------------------------------------------*/

/* Set_Interaction_Matrix()
 *
 * Sets and factors the interaction matrix
 */
  static void
Set_Interaction_Matrix( void )
{
  /* Memory allocation for symmetry array */
  smat.nop = netcx.neq/netcx.npeq;
  size_t mreq = (size_t)(smat.nop * smat.nop) * sizeof( complex double);
  mem_realloc( (void **)&smat.ssx, mreq, "in xnec2c.c" );

  /* irngf is not used (NGF function not implemented) */
  int iresrv = data.np2m * (data.np + 2 * data.mp);
  if( matpar.imat == 0)
    fblock( netcx.npeq, netcx.neq, iresrv, data.ipsym);

  cmset( netcx.neq, cm, calc_data.rkh, calc_data.iexk );
  factrs( netcx.npeq, netcx.neq, cm, save.ip );
  netcx.ntsol = 0;

} /* Set_Interaction_Matrix() */

/*-----------------------------------------------------------------------*/

/* Set_Excitation()
 *
 * Sets the excitation part of the matrix
 */
  static void
Set_Excitation( void )
{
  if( (fpat.ixtyp >= 1) && (fpat.ixtyp <= 4) )
  {
    tmp4= TORAD* calc_data.xpr4;
    tmp5= TORAD* calc_data.xpr5;

    if( fpat.ixtyp == 4)
    {
      tmp1= calc_data.xpr1/ data.wlam;
      tmp2= calc_data.xpr2/ data.wlam;
      tmp3= calc_data.xpr3/ data.wlam;
      tmp6= calc_data.xpr6/( data.wlam* data.wlam);
    }
    else
    {
      tmp1= TORAD* calc_data.xpr1;
      tmp2= TORAD* calc_data.xpr2;
      tmp3= TORAD* calc_data.xpr3;
      tmp6= calc_data.xpr6;
    } /* if( fpat.ixtyp == 4) */

  } /* if( (fpat.ixtyp >= 1) && (fpat.ixtyp <= 4) ) */

  /* fills e field right-hand matrix */
  etmns( tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, fpat.ixtyp, crnt.cur );

} /* Set_Excitation() */

/*-----------------------------------------------------------------------*/

/* Set_Network_Data()
 *
 * Sets up network data and solves for currents
 */
  static void
Set_Network_Data( void )
{
  if( netcx.nonet != 0 )
  {
    int i, j, itmp1, itmp2, itmp3;

    itmp3=0;
    itmp1= netcx.ntyp[0];
    for( i = 0; i < 2; i++ )
    {
      if( itmp1 == 3) itmp1=2;

      for( j = 0; j < netcx.nonet; j++)
      {
        itmp2= netcx.ntyp[j];

        if( (itmp2/itmp1) != 1 ) itmp3 = itmp2;
        else if( (itmp2 >= 2) && (netcx.x11i[j] <= 0.0) )
        {
          double xx, yy, zz;
          int idx4, idx5;

          idx4 = netcx.iseg1[j]-1;
          idx5 = netcx.iseg2[j]-1;
          xx = data.segments[idx5].x - data.segments[idx4].x;
          yy = data.segments[idx5].y - data.segments[idx4].y;
          zz = data.segments[idx5].z - data.segments[idx4].z;
          netcx.x11i[j] = data.wlam* sqrt( xx*xx + yy*yy + zz*zz );
        }

      } /* for( j = 0; j < netcx.nonet; j++) */

      if( itmp3 == 0) break;

      itmp1= itmp3;

    } /* for( i = 0; i < 2; i++ ) */

  } /* if( netcx.nonet != 0 ) */

  /* Set network data */
  netwk( cm, save.ip, crnt.cur );
  netcx.ntsol = 1;

  /* Save impedance data for normalization */
  int fstep = calc_data.freq_step;
  if (fstep < 0 || fstep > calc_data.steps_total)
	return;

  if( ((calc_data.steps_total > 1) &&
        isFlagSet(FREQ_LOOP_RUNNING)) ||
		CHILD ||
		fstep == calc_data.steps_total)
  {

    impedance_data.zreal[fstep] = (double)creal( netcx.zped);
    impedance_data.zimag[fstep] = (double)cimag( netcx.zped);
    impedance_data.zmagn[fstep] = (double)cabs( netcx.zped);
    impedance_data.zphase[fstep]= (double)cang( netcx.zped);

    if( (calc_data.iped == 1) &&
        ((double)impedance_data.zmagn[fstep] > calc_data.zpnorm) )
      calc_data.zpnorm = (double)impedance_data.zmagn[fstep];
  }

} /* Set_Network_Data() */

/*-----------------------------------------------------------------------*/

/* Power_Loss()
 *
 * Calculate power loss due to segment loading
 */
  static void
Power_Loss( void )
{
  int i;
  double cmg;
  complex double curi;


  /* No wire/segments in structure */
  if( data.n == 0) return;

  fpat.ploss = 0.0;
  /* Loop over all wire segs */
  for( i = 0; i < data.n; i++ )
  {
    /* Calculate segment current (mag/phase) */
    curi= crnt.cur[i]* data.wlam;
    cmg= cabs( curi);

    /* Calculate power loss in segment */
    if( (zload.nload != 0) &&
        (fabs(creal(zload.zarray[i])) >= 1.0e-20) )
      fpat.ploss += 0.5* cmg* cmg* creal( zload.zarray[i])* data.segments[i].si;

  } /* for( i = 0; i < n; i++ ) */

} /* Power_Loss() */

/*-----------------------------------------------------------------------*/

/* Radiation_Pattern()
 *
 * Calculates far field (radiation) pattern
 */
  static void
Radiation_Pattern( void )
{
  if( (gnd.ifar != 1) && isFlagSet(ENABLE_RDPAT) )
  {
    fpat.pinr= netcx.pin;
    fpat.pnlr= netcx.pnls;
    rdpat();

    /* Store radiation efficiency per frequency step */
    int fstep = calc_data.freq_step;
    if (fstep >= 0 && fpat.pinr > 0.0)
    {
      rad_pattern[fstep].efficiency =
        (fpat.pinr - fpat.ploss - fpat.pnlr) / fpat.pinr;
    }
  }

} /* Radiation_Pattern() */

/*-----------------------------------------------------------------------*/

/* Near_Field_Pattern()
 *
 * Calculates near field pattern if enabled
 */
  void
Near_Field_Pattern( void )
{
  if( isFlagClear(ENABLE_NEAREH) )
    return;

  if( fpat.nfeh & NEAR_EFIELD )
    nfpat(0);

  if( fpat.nfeh & NEAR_HFIELD )
    nfpat(1);

} /* Near_Field_Pattern() */

/*-----------------------------------------------------------------------*/

/* New_Frequency_Reset_Prev()
 *
 * Resets the previous frequency state to force New_Frequency() to recalculate if the
 * same frequency is called.
 *
 * save.last_freq variable stores the previous MHz value that was used when
 * calling New_Frequency() so it can exit early if that frequency
 * has already been calculated.  Reset_Prev_New_Frequency() needs
 * to be called to reset this when a file is opened or when a benchmark
 * is being run.
 */
void New_Frequency_Reset_Prev(void)
{
	save.last_freq = 0;
}

/* New_Frequency()
 *
 * (Re)calculates all frequency-dependent parameters
 */
  void
New_Frequency( void )
{
  struct timespec start, end;
  double elapsed;

  /* Abort if freq has not really changed, as when changing
   * between current or charge density structure coloring */
  if( (save.last_freq == calc_data.freq_mhz) ||
      isFlagClear(ENABLE_EXCITN) )
    return;

  g_rec_mutex_lock(&freq_data_lock);

  save.last_freq = calc_data.freq_mhz;

  // Only show this if you manually change frequencies:
  clock_gettime(CLOCK_MONOTONIC, &start);

  /* Frequency scaling of geometric parameters */
  Frequency_Scale_Geometry();

  /* Structure segment loading */
  Structure_Impedance_Loading();

  /* Calculate ground parameters */
  Ground_Parameters();

  /* Fill and factor primary interaction matrix */
  Set_Interaction_Matrix();

  /* Fill excitation part of matrix */
  Set_Excitation();

  /* Matrix solving (netwk calls solves) */
  Set_Network_Data();

  /* Calculate power loss */
  Power_Loss();

  /* Calculate radiation pattern */
  Radiation_Pattern();

  /* Near field calculation */
  Near_Field_Pattern();

  /* Per-fstep noise temperature table: frequency is fixed here, so all
   * sky/earth model × method combinations are deterministic and hoistable. */
  ant_temp_fill_fstep( calc_data.freq_step );

  /* Save per-step results before prerender so struct_colors_fill_fstep and
   * Prerender_Near_Field read current crnt_fstep / near_field_fstep data. */
  Save_Crnt_Data( calc_data.freq_step );
  Save_Nearfield_Data( calc_data.freq_step );

  /* Child-deterministic per-fstep prerender: no user-mutable inputs enter
   * these functions. */
  struct_colors_fill_fstep( calc_data.freq_step );

  if( isFlagSet(ENABLE_NEAREH) )
    Prerender_Near_Field( calc_data.freq_step );

  if( !CHILD )
  {
    if( save.fstep != NULL && calc_data.freq_step >= 0 )
      save.fstep[calc_data.freq_step] = 1;
  }

  g_rec_mutex_unlock(&freq_data_lock);

  // Calculate elapsed time
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  elapsed = (end.tv_sec + (double)end.tv_nsec/1e9) - (start.tv_sec + (double)start.tv_nsec/1e9);
  pr_info("%.6f MHz: %f seconds. (%s)\n",
			calc_data.freq_mhz, elapsed, current_mathlib->name);

} /* New_Frequency()  */

/*-----------------------------------------------------------------------*/

typedef struct
{
  int              max_step;     /* Highest dispatchable index (steps_total or steps_total-1) */
  int              next_scan;    /* Resume point for dispatch step scan */
  struct timespec  t0;           /* Wall-clock start time */
  child_proc_t   **idle_stack;   /* LIFO stack of idle child pointers */
  int              idle_top;     /* Index of top entry; -1 = empty */
} freq_loop_state_t;

/* Per-sweep state; allocated by Start_Frequency_Loop(), freed by thread/Stop */
static freq_loop_state_t *floop_state = NULL;

/**
 * freq_loop_display_step - determine highest valid step to show in the UI
 *
 * Scans save.fstep[] for the highest completed sweep entry.  When the green-
 * line slot is active and computed, it takes priority over sweep steps.
 * Returns the step index, or -1 if no data is available yet.
 */
static int
freq_loop_display_step( void )
{
  int idx, step = -1;

  g_rec_mutex_lock(&freq_data_lock);

  for( idx = 0; idx < calc_data.steps_total; idx++ )
    if( save.fstep[idx] )
      step = idx;

  /* Green-line slot takes priority when active and computed */
  if( calc_data.fmhz_save > 0.0 && save.fstep[calc_data.steps_total] )
    step = calc_data.steps_total;

  g_rec_mutex_unlock(&freq_data_lock);
  return step;
}

/**
 * freq_step_update_ui - apply a frequency step change to all UI consumers
 * @new_step: frequency step index to activate (0..steps_total)
 *
 * Sets calc_data.freq_step and all derived display state, then queues
 * redraws for every frequency-dependent window.  Unconditional and
 * idempotent.  Must be called on the GTK main thread.
 */
void
freq_step_update_ui( int new_step, gboolean force )
{
  char txt[16];

  g_rec_mutex_lock(&freq_data_lock);

  calc_data.freq_step = new_step;
  calc_data.freq_mhz  = save.freq[new_step];
  SetFlag( DRAW_NEW_RDPAT );
  SetFlag( FREQ_LOOP_READY );

  /* Block value-changed callbacks during programmatic spinbutton updates;
   * only user interaction sets fmhz_save via those callbacks. */
  SIGNAL_BLOCK(mainwin_frequency, on_freq_spinbutton_value_changed);
  gtk_spin_button_set_value( mainwin_frequency, calc_data.freq_mhz );
  SIGNAL_UNBLOCK(mainwin_frequency, on_freq_spinbutton_value_changed);

  if( isFlagSet(DRAW_ENABLED) && rdpattern_frequency != NULL )
  {
    SIGNAL_BLOCK(rdpattern_frequency, on_freq_spinbutton_value_changed);
    gtk_spin_button_set_value( rdpattern_frequency, calc_data.freq_mhz );
    SIGNAL_UNBLOCK(rdpattern_frequency, on_freq_spinbutton_value_changed);
  }

  if( isFlagSet(PLOT_ENABLED) )
  {
    snprintf( txt, sizeof(txt), "%.3f", calc_data.freq_mhz );
    gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(
        freqplots_window_builder, "freqplots_fmhz_entry")), txt );

    xnec2_widget_queue_draw( freqplots_drawingarea, force );
  }

  /* Vertex colors are baked per freq_step; invalidate so the next render
   * rebuilds them from crnt_fstep[new_step] rather than cached stale data. */
#ifdef HAVE_OPENGL
  opengl_structure_invalidate();
#endif

  xnec2_widget_queue_draw( structure_drawingarea, force );

  if( isFlagSet(DRAW_ENABLED) )
  {
    xnec2_widget_queue_draw( rdpattern_drawingarea, force );
    xnec2_widget_queue_draw( rdpattern_gl_area, force );
  }

  opt_ui_update_values();

  g_rec_mutex_unlock(&freq_data_lock);
}

/* Idle wrapper: intermediate loop step — draws suppressed during optimizer */
static gboolean
freq_step_update_ui_idle( gpointer p )
{
  freq_step_update_ui( GPOINTER_TO_INT(p), FALSE );
  return G_SOURCE_REMOVE;
}

/* Idle wrapper: terminal step — draws forced through SUPPRESS gate.
 * Do not flush GTK events here — _callback_g_idle_add_once flushes
 * after we return for sync paths; flushing here would process arbitrary
 * idle sources (e.g. eval_apply_and_reload) and deadlock on pthread_join. */
static gboolean
freq_step_update_ui_idle_force( gpointer p )
{
  freq_step_update_ui( GPOINTER_TO_INT(p), TRUE );
  return G_SOURCE_REMOVE;
}

/**
 * freq_populate_steps - pre-compute save.freq[] from FR card parameters
 *
 * Fills save.freq[0..steps_total-1] with frequencies derived from each
 * FR card's min_freq, delta_freq, and ifreq (linear vs multiplicative).
 * If fmhz_save is valid, also sets save.freq[steps_total] for the green
 * line slot.  Returns the highest dispatchable step index (max_step).
 */
static int
freq_populate_steps( void )
{
  int step = 0, fr, card_start;
  double freq;
  freq_loop_data_t *fld;

  for( fr = 0; fr < calc_data.FR_cards; fr++ )
  {
    fld = &calc_data.freq_loop_data[fr];
    freq = fld->min_freq;
    card_start = step;

    for( ; step < card_start + fld->freq_steps && step < calc_data.steps_total; step++ )
    {
      if( step > card_start )
      {
        freq = (fld->ifreq == 1)
            ? freq * fld->delta_freq
            : freq + fld->delta_freq;
      }
      save.freq[step] = freq;
    }
  }

  /* Include the extra green-line slot in the scan range when the user
   * has selected a green-line frequency.  Assign it here so the dispatch
   * loop treats it identically to sweep slots. */
  if( calc_data.fmhz_save > 0.0 )
  {
    save.freq[calc_data.steps_total] = calc_data.fmhz_save;
    return calc_data.steps_total;
  }

  return calc_data.steps_total - 1;
}

/*
 * freq_loop_collect - save per-step results into shared arrays
 * @fstep: step index to store results at
 *
 * Called under freq_data_lock.  For the forked path the caller has already
 * read raw data via Get_Freq_Data(); for the non-forked path the data is
 * already in memory from New_Frequency().
 */
static void
freq_loop_collect( int fstep )
{
  Save_Crnt_Data( fstep );
  Save_Nearfield_Data( fstep );
  save.fstep[fstep] = 1;
}

static inline gboolean
idle_stack_empty( const freq_loop_state_t *state )
{ return state->idle_top < 0; }

static inline gboolean
idle_stack_full( const freq_loop_state_t *state )
{ return state->idle_top == calc_data.num_jobs - 1; }

/* Returns TRUE if any child has an assigned step not yet collected */
static gboolean
children_dispatched( void )
{
  int i;

  for( i = 0; i < calc_data.num_jobs; i++ )
    if( child_procs[i]->assigned_step != -1 )
      return TRUE;

  return FALSE;
}

/* Returns TRUE if step is already dispatched to a child (in-flight) */
static gboolean
step_in_flight( int step )
{
  int i;

  for( i = 0; i < calc_data.num_jobs; i++ )
    if( child_procs[i]->assigned_step == step )
      return TRUE;

  return FALSE;
}

static inline child_proc_t *
idle_stack_pop( freq_loop_state_t *state )
{ return state->idle_stack[state->idle_top--]; }

static inline void
idle_stack_push( freq_loop_state_t *state, child_proc_t *child )
{ state->idle_stack[++state->idle_top] = child; }

/**
 * freq_loop_validate_result - check if a child's result is still current
 * @state: loop state; child pushed back to idle stack on stale result
 * @child: child that finished computing
 *
 * Compares the frequency dispatched to the child against the current
 * value in save.freq[].  If they differ (external invalidation raced
 * the computation), logs a notice, discards the result, and returns
 * the child to the idle stack.
 *
 * Returns TRUE if the result is valid; FALSE if stale (caller skips collect).
 * Called under freq_data_lock.
 */
static gboolean
freq_loop_validate_result( freq_loop_state_t *state, child_proc_t *child )
{
  if( FREQ_EQ(save.freq[child->assigned_step], child->assigned_freq) )
    return TRUE;

  pr_notice("step %d raced: dispatched %.6f MHz, current %.6f MHz; will recalculate\n",
      child->assigned_step, child->assigned_freq,
      save.freq[child->assigned_step]);
  child->assigned_step = -1;
  idle_stack_push( state, child );
  return FALSE;
}

/*
 * freq_loop_dispatch - send one frequency step to a child or compute inline
 * @state: loop state; idle_stack updated for non-forked path
 * @child: child process descriptor
 * @fstep: step index being dispatched
 * @freq:  frequency in MHz to compute
 * @batch: TRUE for batch mathlib, FALSE for interactive mathlib
 *
 * Forked: sets child->assigned_step and writes MATHLIB+FRQDATA to pipe.
 * Non-forked: computes inline under freq_data_lock, collects, resets
 * child->assigned_step to -1, and pushes child back onto the idle stack.
 * The COMPUTE loop detects synchronous completion via child->assigned_step == -1.
 * Non-forked path: New_Frequency acquires freq_data_lock internally.
 */
static void
freq_loop_dispatch( freq_loop_state_t *state, child_proc_t *child,
                    int fstep, double freq, gboolean batch )
{
  char   *buff;
  size_t  len;

  child->assigned_step = fstep;
  child->assigned_freq = freq;

  /* Record dispatched frequency so freq_loop_validate_result and the
   * dispatch scan can compare stored vs desired for the extra slot. */
  save.freq[fstep] = freq;

  if( FORKED )
  {
    const char *mathlib_id = batch
        ? rc_config.mathlib_batch_id
        : current_mathlib->id;

    if( batch )
      mathlib_lock_intel_batch( mathlib_id );
    else
      mathlib_lock_intel_interactive( mathlib_id );

    len = strlen( fork_commands[MATHLIB] );
    Write_Pipe( child->idx, fork_commands[MATHLIB], (ssize_t)len, TRUE );
    Write_Pipe( child->idx, (char *)mathlib_id, (ssize_t)MATHLIB_ID_LEN, TRUE );

    len = strlen( fork_commands[FRQDATA] );
    Write_Pipe( child->idx, fork_commands[FRQDATA], (ssize_t)len, TRUE );
    buff = (char *)&freq;
    Write_Pipe( child->idx, buff, (ssize_t)sizeof(double), TRUE );
    return;
  }

  /* Non-forked: write freq_mhz and freq_step for the NEC engine here;
   * freq_step_update_ui overwrites both on the GTK thread for display.
   * New_Frequency acquires freq_data_lock internally. */
  calc_data.freq_mhz  = freq;
  calc_data.freq_step = fstep;
  New_Frequency();

  /* Non-forked: computation is synchronous.  Child stays off the idle stack
   * with assigned_step set; freq_loop_collect_pending() handles collect
   * and push-back so the COMPUTE loop needs no forked/non-forked branch. */
}

/*
 * freq_loop_collect_pending - collect one round of finished forked children
 * @state: loop state; idle_stack updated in place
 *
 * Blocks in select() until at least one child pipe is readable, then
 * processes all ready children.  Forked path acquires freq_data_lock
 * internally; non-forked path is lock-free (saves done in New_Frequency).
 *
 * Returns FALSE and sets FREQ_LOOP_STOP if a pipe read fails; TRUE otherwise.
 */
static gboolean
freq_loop_collect_pending( freq_loop_state_t *state )
{
  fd_set read_fds;
  int    n = 0, sel_ret, idx;

  FD_ZERO( &read_fds );
  for( idx = 0; idx < calc_data.num_jobs; idx++ )
  {
    if( child_procs[idx]->assigned_step == -1 )
      continue;

    int rfd = child_procs[idx]->from_child[READ];
    if( rfd < 0 )
    {
      if( FORKED )
        pr_warn("child %d has invalid pipe fd during forked collect\n", idx);
      continue;
    }

    FD_SET( rfd, &read_fds );
    if( n < rfd )
      n = rfd;
  }

  /* Non-forked path: no pipe fds (n==0); dispatch() computed synchronously
   * but left the child off the idle stack with assigned_step set.
   * Collect results and return child to the idle stack. */
  /* Non-forked path: New_Frequency already saved under freq_data_lock;
   * collect results and return children to the idle stack. */
  if( n == 0 )
  {
    for( idx = 0; idx < calc_data.num_jobs; idx++ )
    {
      if( child_procs[idx]->assigned_step == -1 )
        continue;

      if( !freq_loop_validate_result( state, child_procs[idx] ) )
        continue;

      freq_loop_collect( child_procs[idx]->assigned_step );
      child_procs[idx]->assigned_step = -1;
      idle_stack_push( state, child_procs[idx] );
    }
    return TRUE;
  }

  do
  {
    sel_ret = select( n + 1, &read_fds, NULL, NULL, NULL );
  } while( sel_ret == -1 && errno == EINTR );

  if( sel_ret == -1 )
  {
    perror( "select()" );
    _exit(0);
  }

  g_rec_mutex_lock(&freq_data_lock);
  for( idx = 0; idx < num_child_procs; idx++ )
  {
    if( child_procs[idx]->assigned_step == -1 )
      continue;

    if( !FD_ISSET(child_procs[idx]->from_child[READ], &read_fds) )
      continue;

    int child_fstep = child_procs[idx]->assigned_step;

    if( !Get_Freq_Data( idx, child_fstep ) )
    {
      pr_err("Failed to read data from forked child\n");
      SetFlag(FREQ_LOOP_STOP);
      g_rec_mutex_unlock(&freq_data_lock);
      return FALSE;
    }

    /* Invalidate parent dedup cache; Get_Freq_Data overwrote local EM arrays */
    New_Frequency_Reset_Prev();

    if( !freq_loop_validate_result( state, child_procs[idx] ) )
      continue;

    freq_loop_collect( child_fstep );
    child_procs[idx]->assigned_step = -1;
    idle_stack_push( state, child_procs[idx] );
  }
  g_rec_mutex_unlock(&freq_data_lock);

  return TRUE;
}


/**
 * fmhz_save_apply_idle - GTK idle callback to apply reset fmhz_save via
 *     user_set_frequency (the single point of truth for frequency selection).
 */
static void
fmhz_save_apply_idle(gpointer data)
{
  (void)data;
  user_set_frequency(calc_data.fmhz_save);
}

/**
 * fmhz_save_reset_if_stale - reset stale green-line frequency to best-VSWR step
 *
 * When fmhz_save falls outside every FR card range (e.g. after loading a
 * different NEC2 file while the config retains the old frequency), replaces
 * it with the sweep frequency that has the lowest VSWR.
 *
 * Called from freq_loop_finalize after all sweep data is valid.
 *
 * Returns TRUE when fmhz_save was reset, signalling the caller to skip
 * its normal display-step logic (user_set_frequency handles the UI update).
 */
static gboolean
fmhz_save_reset_if_stale(void)
{
  if (calc_data.fmhz_save <= 0.0 || isnan(calc_data.fmhz_save) || calc_data.FR_cards < 1)
    return FALSE;

  /* Check whether fmhz_save falls within any FR card range */
  for (int fr = 0; fr < calc_data.FR_cards; fr++)
  {
    freq_loop_data_t *fld = &calc_data.freq_loop_data[fr];
    if (calc_data.fmhz_save >= fld->min_freq - 1e-6
        && calc_data.fmhz_save <= fld->max_freq + 1e-6)
      return FALSE;
  }

  /* fmhz_save is visible in a plot panel whose display range exceeds the FR
   * card data range (e.g. single-frequency card expanded by Fit_to_Scale) */
  if (fmhz_within_display_range(calc_data.fmhz_save))
    return FALSE;

  /* fmhz_save is outside all FR card ranges; find lowest-VSWR step */
  double best_vswr = 1e30;
  int best_step = 0;
  measurement_t meas;

  for (int idx = 0; idx < calc_data.steps_total; idx++)
  {
    if (!save.fstep[idx])
      continue;

    meas_calc(&meas, idx);

    if (!isnan(meas.vswr) && meas.vswr >= 0.0 && meas.vswr < best_vswr)
    {
      best_vswr = meas.vswr;
      best_step = idx;
    }
  }

  /* No valid steps computed; leave fmhz_save unchanged */
  if (best_vswr >= 1e30)
    return FALSE;

  pr_notice("fmhz_save %.4f MHz outside FR card ranges; reset to %.4f MHz (VSWR %.2f)\n",
    calc_data.fmhz_save, save.freq[best_step], best_vswr);

  calc_data.fmhz_save = save.freq[best_step];
  /* Queue the full UI update on the GTK main thread via the single
   * point of truth for user-selected frequency changes. */
  g_idle_add_once((GSourceOnceFunc)fmhz_save_apply_idle, NULL);
  return TRUE;
}

/*
 * freq_loop_finalize - complete a finished sweep
 * @state: loop state (for elapsed-time calculation)
 *
 * Sets FREQ_LOOP_DONE, logs elapsed time, wakes the optimizer, and
 * queues final UI updates.  No locks held on entry.
 */
static void
freq_loop_finalize( freq_loop_state_t *state )
{
  struct timespec end;

  SetFlag( FREQ_LOOP_DONE );

  clock_gettime(CLOCK_MONOTONIC, &end);
  pr_notice("Frequency loop elapsed time: %f seconds. (%s)\n",
    (end.tv_sec  + (double)end.tv_nsec  / 1e9) -
    (state->t0.tv_sec + (double)state->t0.tv_nsec / 1e9),
    (FORKED ? get_mathlib_by_id(rc_config.mathlib_batch_id)->name
            : current_mathlib->name));

  /* Wake optimizer thread waiting on eval_cond */
  nec2_eval_signal();

  /* Reset stale green-line frequency to best-VSWR sweep step;
   * when reset occurs, fmhz_save_reset_if_stale queues user_set_frequency
   * on the GTK main thread which handles all UI updates. */
  if( !fmhz_save_reset_if_stale() )
  {
    int display = freq_loop_display_step();
    if( display >= 0 )
      g_idle_add_once( (GSourceOnceFunc)freq_step_update_ui_idle_force,
                            GINT_TO_POINTER(display) );
  }

  if( (rc_config.batch_mode || isFlagSet(SUPPRESS_INTERMEDIATE_REDRAWS)) &&
      opt_have_files_to_save() )
    /* Async: sync would deadlock if the optimizer queues
     * eval_apply_and_reload before this idle source is processed. */
    g_idle_add_once((GSourceOnceFunc)Write_Optimizer_Data, NULL);
}

/**
 * Frequency_Loop - single iteration of the frequency sweep state machine
 * @udata: per-sweep freq_loop_state_t* allocated by Start_Frequency_Loop()
 *
 * States: INIT -> COMPUTE -> PUBLISH -> [FINALIZE | COMPUTE]
 *
 * Returns: TRUE to request another call; FALSE when the sweep is complete
 * or has been stopped and all pending children have been drained.
 */
gboolean
Frequency_Loop( gpointer udata )
{
  freq_loop_state_t *state = (freq_loop_state_t *)udata;
  int idx;

  /* INIT: reset all iteration state for a new sweep */
  if( isFlagSet(FREQ_LOOP_INIT) )
  {
    ClearFlag( FREQ_LOOP_INIT | FREQ_LOOP_DONE );

    state->idle_top     = -1;
    state->next_scan    = 0;
    state->max_step     = freq_populate_steps();

    /* Per-step validity is managed by Start_Frequency_Loop;
     * INIT resets the dedup cache and loop infrastructure. */

    /* Reset parent dedup cache so every sweep recomputes */
    New_Frequency_Reset_Prev();

    /* Push all children onto the idle stack; mark each as idle */
    for( idx = 0; idx < calc_data.num_jobs; idx++ )
    {
      child_procs[idx]->assigned_step = -1;
      idle_stack_push( state, child_procs[idx] );
    }

    /* freq_step is a display field owned by the GTK thread; do not reset
     * it here — stale data remains visible during the recomputation cycle
     * and freq_step_update_ui sets it on completion. */
    if( calc_data.zpnorm > 0.0 ) calc_data.iped = 2;

    clock_gettime(CLOCK_MONOTONIC, &state->t0);

    return TRUE;
  }

  /* COMPUTE: producer-consumer loop.
   *
   * Dispatch to all idle children, then block for one collect round when
   * all workers are busy.  Repeat until every step is dispatched and every
   * result collected, or until FREQ_LOOP_STOP is set.
   *
   * Non-forked path: dispatch() is synchronous; one step per Frequency_Loop()
   * call so async redraws can reach the GTK main thread between steps.
   */
  /* Dispatch phase: scan for invalid steps and dispatch to idle children */
  gboolean found_work = FALSE;
  while( !idle_stack_empty(state) && !isFlagSet(FREQ_LOOP_STOP) )
  {
    int next = -1;
    for( idx = state->next_scan; idx <= state->max_step; idx++ )
    {
      if( save.fstep[idx] != 0 || step_in_flight(idx) )
        continue;
      next = idx;
      break;
    }

    /* Wrap to catch externally invalidated steps behind next_scan */
    if( next == -1 && state->next_scan > 0 )
    {
      state->next_scan = 0;
      continue;
    }

    if( next == -1 )
      break;

    found_work = TRUE;
    state->next_scan = next + 1;
    child_proc_t *child = idle_stack_pop( state );
    gboolean batch = (next < calc_data.steps_total);
    freq_loop_dispatch( state, child, next, save.freq[next], batch );
  }


  /* Collect phase: always invoked; handles forked (select+reap) and
   * non-forked (n==0: scan dispatched children, collect, push back). */
  if( !freq_loop_collect_pending(state) )
    return FALSE;

  /* STOP: drain remaining children before exiting */
  if( isFlagSet(FREQ_LOOP_STOP) )
  {
    while( children_dispatched() )
    {
      if( !freq_loop_collect_pending( state ) )
        break;
    }
    return FALSE;
  }

  /* PUBLISH: expose highest completed step to all UI consumers */
  SetFlag( FREQ_LOOP_READY );

  /* Dispatch found nothing and all children have returned */
  if( !found_work && idle_stack_full(state) )
  {
    freq_loop_finalize( state );
    return FALSE;
  }

  if( isFlagClear(SUPPRESS_INTERMEDIATE_REDRAWS) )
  {
    int display = freq_loop_display_step();
    if( display >= 0 )
      g_idle_add_once( (GSourceOnceFunc)freq_step_update_ui_idle,
                       GINT_TO_POINTER(display) );
  }
  return TRUE;
} /* Frequency_Loop() */

/*-----------------------------------------------------------------------*/


void *Frequency_Loop_Thread(void *p)
{
	freq_loop_state_t *state = (freq_loop_state_t *)p;

	// Don't draw the green line if in batch mode
	if (rc_config.batch_mode)
		calc_data.fmhz_save = 0.0;

	// Run the loop; Frequency_Loop() returns FALSE when done or stopped
	while( Frequency_Loop(state) );

	if (isFlagSet(FREQ_LOOP_STOP))
		goto out;

	// Exit if in batch mode
	if (rc_config.batch_mode)
	{
		g_idle_add_once_sync((GSourceOnceFunc)Gtk_Quit, NULL);
		goto out;
	}

	/*
		Prevent deadlock waiting for Stop_Frequency_Loop()=>pthread_join()
		in Open_Input_File() triggered by Optimizer_Output() because
		g_idle_add_once_sync won't allow this Frequency_Loop_Thread()
		thread to exit until Open_Input_File() returns for GTK to make
		progress, but Open_Input_File() is waiting for pthread_join()
		to return when this thread exits.
	*/
	if ( isFlagSet(INPUT_PENDING) )
		goto out;

out:
	ClearFlag(FREQ_LOOP_RUNNING);
	return NULL;
}


/**
 * freq_loop_start_internal - allocate state and launch the loop thread
 *
 * Caller has already invalidated the desired save.fstep[] entries.
 * Returns TRUE on success, FALSE if preconditions are not met.
 */
static gboolean
freq_loop_start_internal( void )
{
  if( calc_data.freq_loop_data == NULL )
    return FALSE;

  if( isFlagSet(FREQ_LOOP_RUNNING) ||
      calc_data.FR_cards < 1       ||
      calc_data.steps_total < 1 )
    return FALSE;

  /* Join previous thread if it exited naturally but was never joined.
   * Stop_Frequency_Loop is idempotent and no-ops when pth_freq_loop is NULL. */
  Stop_Frequency_Loop();

  /* Re-check: the GTK event flush inside Stop_Frequency_Loop may have
   * re-entrantly started a new sweep via eval_apply_and_reload. */
  if( isFlagSet(FREQ_LOOP_RUNNING) )
    return FALSE;

  mem_alloc((void**)&floop_state, sizeof(freq_loop_state_t), __LOCATION__);
  floop_state->idle_top = -1;
  mem_alloc((void**)&floop_state->idle_stack,
            (size_t)calc_data.num_jobs * sizeof(child_proc_t *), __LOCATION__);

  SetFlag( FREQ_LOOP_INIT );
  SetFlag( FREQ_LOOP_RUNNING );

  /* Intermediate-step draws use force=FALSE and are gated by
   * SUPPRESS_INTERMEDIATE_REDRAWS inside xnec2_widget_queue_draw. */

  if( !rc_config.disable_pthread_freqloop )
  {
    mem_alloc((void**)&pth_freq_loop, sizeof(pthread_t), __LOCATION__);
    int ret = pthread_create( pth_freq_loop, NULL, Frequency_Loop_Thread, floop_state );
    if( ret != 0 )
    {
      free_ptr((void**)&pth_freq_loop);
      pr_crit("failed to start Frequency_Loop_Thread\n");
      perror( "pthread_create()" );
      exit( -1 );
    }
  }
  else
  {
    floop_tag = g_idle_add( Frequency_Loop, floop_state );
  }

  return TRUE;
}

/**
 * Start_Frequency_Loop - invalidate all steps and start a full sweep
 */
gboolean
Start_Frequency_Loop( void )
{
  if( save.fstep == NULL || calc_data.steps_total < 1 )
    return FALSE;

  g_rec_mutex_lock(&freq_data_lock);
  for( int i = 0; i <= calc_data.steps_total; i++ )
    save.fstep[i] = 0;
  g_rec_mutex_unlock(&freq_data_lock);

  return freq_loop_start_internal();
}

/**
 * Start_Frequency_Loop_Greenline - recompute only the green-line step
 *
 * Invalidates save.fstep[steps_total] so the dispatch loop recomputes
 * that slot.  Sweep steps 0..steps_total-1 remain valid.
 */
gboolean
Start_Frequency_Loop_Greenline( void )
{
  if( calc_data.fmhz_save <= 0.0 || save.fstep == NULL || calc_data.steps_total < 1 )
    return FALSE;

  g_rec_mutex_lock(&freq_data_lock);
  save.fstep[calc_data.steps_total] = 0;
  g_rec_mutex_unlock(&freq_data_lock);

  return freq_loop_start_internal();
}

/*-----------------------------------------------------------------------*/

/* Stop_Frequency_Loop()
 *
 * Stops and resets freq loop
 */
  void
Stop_Frequency_Loop( void )
{
  // Clearing this flag will cause the Frequency_Loop pthread to exit when it is done:
  ClearFlag( FREQ_LOOP_RUNNING );
  SetFlag(FREQ_LOOP_STOP);

  if (!rc_config.disable_pthread_freqloop)
  {
	  // Wait for the thread to exit:
	  if (pth_freq_loop != NULL)
	  {
		  pthread_join(*pth_freq_loop, NULL);
		  free_ptr((void**)&pth_freq_loop);

		  if( floop_state != NULL )
		  {
		    free_ptr((void**)&floop_state->idle_stack);
		    free_ptr((void**)&floop_state);
		  }
	  }

	  ClearFlag(FREQ_LOOP_STOP);

	  // Flush any pending GTK events. This is critical because any pending
	  // events that may work upon GtkWidget's that change (or close) upon exit
	  // from this function will fail.
	  while( g_main_context_iteration(NULL, FALSE) ) {}
  }
  else if( floop_tag > 0 )
  {
	g_source_remove( floop_tag );
	floop_tag = 0;
	ClearFlag(FREQ_LOOP_STOP);

	/* Both paths free state here; g_idle source was removed above */
	if( floop_state != NULL )
	{
	  free_ptr((void**)&floop_state->idle_stack);
	  free_ptr((void**)&floop_state);
	}
  }
} /* Stop_Frequency_Loop() */

/*-----------------------------------------------------------------------*/

/* Incident_Field_Loop()
 *
 * Loops over incident field directions if
 * receiving pattern calculations are requested
 */
  void
Incident_Field_Loop( void )
{
  int phi_step, theta_step;

  /* Frequency scaling of geometric parameters */
  Frequency_Scale_Geometry();

  /* Structure segment loading */
  Structure_Impedance_Loading();

  /* Calculate ground parameters */
  Ground_Parameters();

  /* Fill and factor primary interaction matrix */
  Set_Interaction_Matrix();

  /* Loop over incident field angles */
  netcx.nprint=0;
  /* Loop over phi */
  for( phi_step = 0; phi_step < calc_data.nphi; phi_step++ )
  {
    /* Loop over theta */
    for( theta_step = 0; theta_step < calc_data.nthi; theta_step++ )
    {
      /* Fill excitation part of matrix */
      Set_Excitation();

      /* Matrix solving (netwk calls solves) */
      Set_Network_Data();

      /* Calculate power loss */
      Power_Loss();

      calc_data.xpr1 += calc_data.xpr4;

    } /* for( theta_step = 0; theta_step < calc_data.nthi.. */

    calc_data.xpr1= calc_data.thetis;
    calc_data.xpr2= calc_data.xpr2+ calc_data.xpr5;

  } /* for( phi_step = 0; phi_step < calc_data.nphi.. */

  calc_data.xpr2  = calc_data.phiss;

} /* Incident_Field_Loop() */

/*-----------------------------------------------------------------------*/

