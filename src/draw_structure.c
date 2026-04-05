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

#include "draw_structure.h"
#include "shared.h"
#include "opengl/opengl_structure.h"

/*-----------------------------------------------------------------------*/

/*  Draw_Structure()
 *
 *  Draws xyz axes, wire segments and patches
 */
/**
 * Draw_Structure_UI() - Update structure-window UI readouts
 *
 * Writes the viewer-gain entry and frequency-step label for the main
 * window.  Called under freq_data_lock by both the Cairo draw path
 * (_Draw_Structure) and the OpenGL post-render callback.
 */
  void
Draw_Structure_UI(void)
{
  Show_Viewer_Gain(
      main_window_builder,
      "main_gain_entry",
      structure_proj_params );

  if( calc_data.freq_step >= 0 )
    Display_Fstep( structure_fstep_entry, calc_data.freq_step );

} /* Draw_Structure_UI() */

/*-----------------------------------------------------------------------*/

  void
_Draw_Structure( cairo_t *cr )
{
  /* Clear drawingarea */
  cairo_set_source_rgb( cr, BLACK );
  cairo_rectangle(
      cr, 0.0, 0.0,
      (double)structure_proj_params.width,
      (double)structure_proj_params.height);
  cairo_fill( cr );

  /* Process and draw geometry if available, else clear screen */
  Process_Wire_Segments();
  Process_Surface_Patches();
  Draw_XYZ_Axes( cr, structure_proj_params );
  Draw_Surface_Patches( cr, structure_segs+data.n, data.m );
  Draw_Wire_Segments( cr, structure_segs, data.n );

  Draw_Structure_UI();

} /* _Draw_Structure() */

void Draw_Structure( cairo_t *cr )
{
	if (isFlagSet(ERROR_CONDX))
		return;

	g_mutex_lock(&freq_data_lock);
	_Draw_Structure( cr );
	g_mutex_unlock(&freq_data_lock);
}

/*-----------------------------------------------------------------------*/

/*  New_Wire_Data()
 *
 *  Calculates some projection parameters
 *  when new wire segment data is created
 */
  static void
New_Wire_Data( void )
{
  /* Abort if no wire data */
  if( data.n == 0 ) return;

  double
    r,     /* Distance of a point from XYZ origin */
    r_max; /* Maximum value of above */

  int idx;

  /* Find segment end furthest from xyz axes origin */
  r_max = 0.0;
  for( idx = 0; idx < data.n; idx++ )
  {
    r = sqrt(
        (double)data.x1[idx] * (double)data.x1[idx] +
        (double)data.y1[idx] * (double)data.y1[idx] +
        (double)data.z1[idx] * (double)data.z1[idx] );
    if( r > r_max )
      r_max = r;

    r = sqrt(
        (double)data.x2[idx] * (double)data.x2[idx] +
        (double)data.y2[idx] * (double)data.y2[idx] +
        (double)data.z2[idx] * (double)data.z2[idx] );
    if( r > r_max )
      r_max = r;

  } /* for( idx = 0; idx < data.n; idx++ ) */

  /* Max value of segment r saved if appropriate */
  if( r_max > structure_proj_params.r_max )
    structure_proj_params.r_max = r_max;

  /* Redraw structure on screen */
  New_Projection_Parameters(
      structure_width,
      structure_height,
      &structure_proj_params );

} /* New_Wire_Data() */

/*-----------------------------------------------------------------------*/

/*  New_Patch_Data()
 *
 *  Calculates some projection parameters
 *  when new surface patch data is created
 */
  void
New_Patch_Data( void )
{
  /* Abort if no patch data */
  if( data.m == 0 ) return;

  double
    s,          /* Side/2 of a square that will represent a patch  */
    sx, sy, sz, /* Length of components of s in the X, Y, Z axes */
    r,          /* Distance of points in patch from XYZ co-ordinates */
    r_max;      /* Maximum value of above */

  int idx, i;
  size_t mreq;

  /* Allocate memory for patch line segments */
  mreq = (size_t)(2 * data.m) * sizeof(double);
  mem_realloc( (void **)&data.px1, mreq, "in draw_structure.c" );
  mem_realloc( (void **)&data.py1, mreq, "in draw_structure.c" );
  mem_realloc( (void **)&data.pz1, mreq, "in draw_structure.c" );
  mem_realloc( (void **)&data.px2, mreq, "in draw_structure.c" );
  mem_realloc( (void **)&data.py2, mreq, "in draw_structure.c" );
  mem_realloc( (void **)&data.pz2, mreq, "in draw_structure.c" );

  /* Find point furthest from xyz axes origin */
  r_max = 0.0;
  for( idx = 0; idx < data.m; idx++ )
  {
    i = 2 * idx;

    /* Side/2 of square representing a patch (sqrt of patch area) */
    s = (double)sqrt( data.pbi[idx] ) / 2.0;

    /* Projection of s on xyz components of t1 */
    sx = s * (double)data.t1x[idx];
    sy = s * (double)data.t1y[idx];
    sz = s * (double)data.t1z[idx];

    /* End 1 of line seg parallel to t1 vector */
    data.px1[i] = (double)data.px[idx] + sx;
    data.py1[i] = (double)data.py[idx] + sy;
    data.pz1[i] = (double)data.pz[idx] + sz;

    /* Its distance from XYZ origin */
    r = sqrt(
        data.px1[i]*data.px1[i] +
        data.py1[i]*data.py1[i] +
        data.pz1[i]*data.pz1[i] );
    if( r > r_max )
      r_max = r;

    /* End 2 of line seg parallel to t1 vector */
    data.px2[i] = (double)data.px[idx] - sx;
    data.py2[i] = (double)data.py[idx] - sy;
    data.pz2[i] = (double)data.pz[idx] - sz;

    /* Its distance from XYZ origin */
    r = sqrt(
        data.px2[i]*data.px2[i] +
        data.py2[i]*data.py2[i] +
        data.pz2[i]*data.pz2[i] );
    if( r > r_max )
      r_max = r;

    i++;

    /* Projection of s on xyz components of t2 */
    sx = s * (double)data.t2x[idx];
    sy = s * (double)data.t2y[idx];
    sz = s * (double)data.t2z[idx];

    /* End 1 of line parallel to t2 vector */
    data.px1[i] = (double)data.px[idx] + sx;
    data.py1[i] = (double)data.py[idx] + sy;
    data.pz1[i] = (double)data.pz[idx] + sz;

    /* Its distance from XYZ origin */
    r = sqrt(
        data.px1[i]*data.px1[i] +
        data.py1[i]*data.py1[i] +
        data.pz1[i]*data.pz1[i] );
    if( r > r_max )
      r_max = r;

    /* End 2 of line parallel to t2 vector */
    data.px2[i] = (double)data.px[idx] - sx;
    data.py2[i] = (double)data.py[idx] - sy;
    data.pz2[i] = (double)data.pz[idx] - sz;

    /* Its distance from XYZ origin */
    r = sqrt(
        data.px2[i]*data.px2[i] +
        data.py2[i]*data.py2[i] +
        data.pz2[i]*data.pz2[i] );
    if( r > r_max )
      r_max = r;

  } /* for( idx = 0; idx < data.m; idx++ ) */

  /* Max value of patch r saved if appropriate */
  if( r_max > structure_proj_params.r_max )
    structure_proj_params.r_max = r_max;

  /* Redraw structure on screen */
  New_Projection_Parameters(
      structure_width,
      structure_height,
      &structure_proj_params );

} /* New_Patch_Data() */

/*-----------------------------------------------------------------------*/

/*  Process_Wire_Segments()
 *
 *  Processes wire segment data so they can be drawn on Screen
 */
  void
Process_Wire_Segments( void )
{
  int idx;

  /* Project all wire segs from xyz frame to screen frame */
  for( idx = 0; idx < data.n; idx++ )
    Set_Gdk_Segment(
        &structure_segs[idx],
        &structure_proj_params,
        (double)data.x1[idx],
        (double)data.y1[idx],
        (double)data.z1[idx],
        (double)data.x2[idx],
        (double)data.y2[idx],
        (double)data.z2[idx] );

} /* Process_Wire_Segments() */

/*-----------------------------------------------------------------------*/

/*  Process_Surface_Patches()
 *
 *  Processes surface patch data so they can be drawn on Screen
 */
  void
Process_Surface_Patches( void )
{
  int idx, m2;

  /* Project all patch segs from xyz frame to screen frame */
  /* Patches are represented by 2 line segs parallel to t1 */
  /* and t2 vectors. Length of segs is sqrt of patch area  */
  m2 = data.m * 2;
  for( idx = 0; idx < m2; idx++ )
    Set_Gdk_Segment(
        &structure_segs[idx+data.n],
        &structure_proj_params,
        data.px1[idx],
        data.py1[idx],
        data.pz1[idx],
        data.px2[idx],
        data.py2[idx],
        data.pz2[idx] );

} /* Process_Surface_Patches() */

/*-----------------------------------------------------------------------*/

/*  Draw_Wire_Segments()
 *
 *  Draws all wire segments of the input structure
 */

  void
Draw_Wire_Segments( cairo_t *cr, Segment_t *segm, gint nseg )
{
  /* Abort if no wire segs or new input pending */
  if( !nseg || isFlagSet(INPUT_PENDING) )
    return;

  int idx, i;

  /* Draw networks */
  for( idx = 0; idx < netcx.nonet; idx++ )
  {
    int i1, i2;

    i1 = netcx.iseg1[idx]-1;
    i2 = netcx.iseg2[idx]-1;

    switch( netcx.ntyp[idx] )
    {
      case 1: /* Two-port network */
        {
          GdkPoint points[4];

          /* Draw a box between segs to represent two-port network */
          points[0].x = segm[i1].x1 + (segm[i2].x1 - segm[i1].x1)/3;
          points[0].y = segm[i1].y1 + (segm[i2].y1 - segm[i1].y1)/3;
          points[1].x = segm[i2].x1 + (segm[i1].x1 - segm[i2].x1)/3;
          points[1].y = segm[i2].y1 + (segm[i1].y1 - segm[i2].y1)/3;
          points[2].x = segm[i2].x2 + (segm[i1].x2 - segm[i2].x2)/3;
          points[2].y = segm[i2].y2 + (segm[i1].y2 - segm[i2].y2)/3;
          points[3].x = segm[i1].x2 + (segm[i2].x2 - segm[i1].x2)/3;
          points[3].y = segm[i1].y2 + (segm[i2].y2 - segm[i1].y2)/3;

          cairo_set_source_rgb( cr, MAGENTA );
          Cairo_Draw_Polygon( cr, points, 4 );
          cairo_fill( cr );

          /* Draw connecting lines */
          Cairo_Draw_Line( cr,
              segm[i1].x1, segm[i1].y1,
              segm[i2].x1, segm[i2].y1 );
          Cairo_Draw_Line( cr,
              segm[i1].x2, segm[i1].y2,
              segm[i2].x2, segm[i2].y2 );
        }
        break;

      case 2: /* Straight transmission line */
        /* Set cr attributes for transmission line */
        cairo_set_source_rgb( cr, CYAN );

        Cairo_Draw_Line( cr,
            segm[i1].x1, segm[i1].y1,
            segm[i2].x1, segm[i2].y1 );
        Cairo_Draw_Line( cr,
            segm[i1].x2, segm[i1].y2,
            segm[i2].x2, segm[i2].y2 );
        break;

      case 3: /* Crossed transmisson line */
        /* Set cr attributes for transmission line */
        cairo_set_source_rgb( cr, CYAN );

        Cairo_Draw_Line( cr,
            segm[i1].x1, segm[i1].y1,
            segm[i2].x2, segm[i2].y2 );
        Cairo_Draw_Line( cr,
            segm[i1].x2, segm[i1].y2,
            segm[i2].x1, segm[i2].y1 );

    } /* switch( netcx.ntyp ) */

  } /* for( idx = 0; idx < netcx.nonet; idx++ ) */

  /* Draw currents/charges if enabled, return */
  /* Current or charge calculations do not contain wavelength */
  /* factors, since they are drawn normalized to their max value */
  int fstep = calc_data.freq_step;
  if( (isFlagSet(DRAW_CURRENTS) || isFlagSet(DRAW_CHARGES))
      && CRNT_FSTEP_AVAILABLE(fstep) )
  {
    crnt_t *cf = &crnt_fstep[fstep];
    static double cmax; /* Max of seg current/charge */
    /* To color structure segs */
    double red = 0.0, grn = 0.0, blu = 0.0;
    char label[16];
    size_t s = sizeof( label )-1;

    /* Loop over all wire segs, find max current/charge */
    cmax = 0.0;
    for( idx = 0; idx < nseg; idx++ )
    {
      if( isFlagSet(DRAW_CURRENTS) )
        /* Calculate segment current magnitude */
        cmag[idx] = (double)cabs( cf->cur[idx] );
      else
        /* Calculate segment charge density */
        cmag[idx] = (double)cabs( cmplx(cf->bir[idx], cf->bii[idx]) );

      /* Find max current/charge magnitude */
      if( cmag[idx] > cmax )
        cmax = cmag[idx];
    }

    /* Show max value in color code label */
    if( isFlagSet(DRAW_CURRENTS) )
      snprintf( label, s, "%8.2E", cmax * (double)data.wlam );
    else
      snprintf( label, s, "%8.2E", cmax * 1.0E-6 / (double)calc_data.freq_mhz );
    gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
            main_window_builder, "main_colorcode_maxlabel")), label );

    /* Draw segments in color code according to current */
    for( idx = 0; idx < nseg; idx++ )
    {
      /* Calculate RGB value for seg current */
      Value_to_Color( &red, &grn, &blu, cmag[idx], cmax );

      /* Set cr attributes for segment */
      cairo_set_source_rgb( cr, red, grn, blu );

      /* Draw segment */
      Cairo_Draw_Line( cr,
          segm[idx].x1, segm[idx].y1,
          segm[idx].x2, segm[idx].y2 );
    } /* for( idx = 0; idx < nseg; idx++ ) */

    return;
  } /* if( isFlagSet(DRAW_CURRENTS) || isFlagSet(DRAW_CHARGES) ) */

  /* Draw segs if not all loaded */
  cairo_set_line_width( cr, 2.0 );
  if( zload.nldseg != nseg )
  {
    /* Set gc attributes for segments */
    if( isFlagSet(OVERLAY_STRUCT) &&
        (structure_proj_params.type == RDPATTERN_DRAWINGAREA) )
      cairo_set_source_rgb( cr, WHITE );
    else
      cairo_set_source_rgb( cr, BLUE );

    /* Draw wire segments */
    Cairo_Draw_Segments( cr, segm, nseg );
  }

  /* Draw lumped loaded segments */
  cairo_set_source_rgb( cr, YELLOW );
  cairo_set_line_width( cr, 9.0 );
  for( idx = 0; idx < zload.nldseg; idx++ )
  {
    if( zload.ldtype[idx] != 5 )
    {
      i = zload.ldsegn[idx]-1;
      Cairo_Draw_Line( cr,
          segm[i].x1, segm[i].y1,
          segm[i].x2, segm[i].y2 );
    }
  }

  /* Set gc attributes for excitation */
  cairo_set_source_rgb( cr, RED );
  cairo_set_line_width( cr, 5.0 );

  /* Draw excitation sources */
  for( idx = 0; idx < vsorc.nsant; idx++ )
  {
    i = vsorc.isant[idx]-1;
    Cairo_Draw_Line( cr,
        segm[i].x1, segm[i].y1,
        segm[i].x2, segm[i].y2 );
  }

  for( idx = 0; idx < vsorc.nvqd; idx++ )
  {
    i = vsorc.ivqd[idx]-1;
    Cairo_Draw_Line( cr,
        segm[i].x1, segm[i].y1,
        segm[i].x2, segm[i].y2 );
  }

  /* Draw resistivity loaded segments */
  cairo_set_source_rgb( cr, YELLOW );
  cairo_set_line_width( cr, 2.0 );
  for( idx = 0; idx < zload.nldseg; idx++ )
  {
    if( zload.ldtype[idx] == 5 )
    {
      i = zload.ldsegn[idx]-1;
      Cairo_Draw_Line( cr,
          segm[i].x1, segm[i].y1,
          segm[i].x2, segm[i].y2 );
    }
  }

} /* Draw_Wire_Segments() */

/*-----------------------------------------------------------------------*/

/*  Draw_Surface_Patches()
 *
 *  Draws the line segments that represent surface patches
 */
  void
Draw_Surface_Patches( cairo_t *cr, Segment_t *segm, gint npatch )
{
  /* Abort if no patches */
  if( ! npatch ) return;

  /* Draw currents if enabled, return */
  int fstep = calc_data.freq_step;
  if( isFlagSet(DRAW_CURRENTS)
      && CRNT_FSTEP_AVAILABLE(fstep) )
  {
    crnt_t *cf = &crnt_fstep[fstep];
    /* Buffers for t1,t2 currents below */
    static double cmax;

    /* Current along x,y,z and t1,t2 vector directions */
    complex double cx, cy, cz, ct1, ct2;

    double red = 0.0, grn = 0.0, blu = 0.0;

    int i, j;

    /* Find max value of patch current magnitude */
    j= data.n;
    cmax = 0.0;

    for( i = 0; i < npatch; i++ )
    {
      /* Calculate current along x,y,z vectors */
      cx = cf->cur[j];
      cy = cf->cur[j+1];
      cz = cf->cur[j+2];

      /* Calculate current along t1 and t2 tangent vectors */
      ct1 = cx*(double)data.t1x[i] +
        cy*(double)data.t1y[i] +
        cz*(double)data.t1z[i];
      ct2 = cx*(double)data.t2x[i] +
        cy*(double)data.t2y[i] +
        cz*(double)data.t2z[i];

      /* Save current magnitudes */
      ct1m[i] = (double)cabs( ct1 );
      ct2m[i] = (double)cabs( ct2 );

      /* Find current magnitude max */
      if( ct1m[i] > cmax ) cmax = ct1m[i];
      if( ct2m[i] > cmax ) cmax = ct2m[i];

      j += 3;

    } /* for( i = 0; i < npatch; i++ ) */

    /* Draw patches in color code according to current */
    for( i = 0; i < npatch; i++ )
    {
      j = 2 * i;

      /* Calculate RGB value for patch t1 current */
      Value_to_Color( &red, &grn, &blu, ct1m[i], cmax );

      /* Set cr attributes for patch t1 */
      cairo_set_source_rgb( cr, red, grn, blu );

      /* Draw patch t1 */
      Cairo_Draw_Line( cr,
          segm[j].x1, segm[j].y1,
          segm[j].x2, segm[j].y2 );

      /* Calculate RGB value for patch t2 current */
      Value_to_Color( &red, &grn, &blu, ct2m[i], cmax );

      /* Set cr attributes for patch t2 */
      cairo_set_source_rgb( cr, red, grn, blu );

      /* Draw patch t2 */
      j++;
      Cairo_Draw_Line( cr,
          segm[j].x1, segm[j].y1,
          segm[j].x2, segm[j].y2 );

    } /* for( idx = 0; idx < npatch; idx++ ) */

  } /* if( isFlagSet(DRAW_CURRENTS) ) */
  else
  {
    int idx;

    /* Set gc attributes for patches */
    if( isFlagSet(OVERLAY_STRUCT) &&
        (structure_proj_params.type == RDPATTERN_DRAWINGAREA) )
      cairo_set_source_rgb( cr, WHITE );
    else cairo_set_source_rgb( cr, BLUE );

    /* Draw patch segments */
    int nsg = 2 * npatch;
    for( idx = 0; idx < nsg; idx++ )
    {
      Cairo_Draw_Line( cr,
          segm[idx].x1, segm[idx].y1,
          segm[idx].x2, segm[idx].y2 );
    }
  }

} /* Draw_Surface_Patches() */

/*-----------------------------------------------------------------------*/

/**
 * Alloc_Crnt_Fstep_Buffers() - Allocate per-frequency-step crnt storage
 *
 * @nfrq: Number of frequency steps (steps_total + 1)
 *
 * Allocates crnt_fstep[] array so each frequency step can store
 * its computed current/charge data for later restoration.
 */
  void
Alloc_Crnt_Fstep_Buffers( int nfrq )
{
  size_t mreq;

  /* Outer array — zero so realloc of sub-fields receives NULL, not garbage */
  mreq = (size_t)nfrq * sizeof(crnt_t);
  mem_realloc( (void **)&crnt_fstep, mreq, "in draw_structure.c" );
  memset( crnt_fstep, 0, mreq );

  for( int i = 0; i < nfrq; i++ )
  {
    /* Basis function coefficients: npm elements each */
    mreq = (size_t)data.npm * sizeof(double);
    mem_realloc( (void **)&crnt_fstep[i].air, mreq, "in draw_structure.c" );
    mem_realloc( (void **)&crnt_fstep[i].aii, mreq, "in draw_structure.c" );
    mem_realloc( (void **)&crnt_fstep[i].bir, mreq, "in draw_structure.c" );
    mem_realloc( (void **)&crnt_fstep[i].bii, mreq, "in draw_structure.c" );
    mem_realloc( (void **)&crnt_fstep[i].cir, mreq, "in draw_structure.c" );
    mem_realloc( (void **)&crnt_fstep[i].cii, mreq, "in draw_structure.c" );

    /* Solved current amplitudes: np3m elements */
    mreq = (size_t)data.np3m * sizeof(complex double);
    mem_realloc( (void **)&crnt_fstep[i].cur, mreq, "in draw_structure.c" );

  }

} /* Alloc_Crnt_Fstep_Buffers() */

/*-----------------------------------------------------------------------*/

/**
 * Free_Crnt_Fstep_Buffers() - Free per-frequency-step crnt storage
 */
  void
Free_Crnt_Fstep_Buffers( void )
{
  if( crnt_fstep == NULL )
    return;

  int nfrq = calc_data.steps_total + 1;
  for( int i = 0; i < nfrq; i++ )
  {
    free_ptr( (void **)&crnt_fstep[i].air );
    free_ptr( (void **)&crnt_fstep[i].aii );
    free_ptr( (void **)&crnt_fstep[i].bir );
    free_ptr( (void **)&crnt_fstep[i].bii );
    free_ptr( (void **)&crnt_fstep[i].cir );
    free_ptr( (void **)&crnt_fstep[i].cii );
    free_ptr( (void **)&crnt_fstep[i].cur );
  }

  free_ptr( (void **)&crnt_fstep );

} /* Free_Crnt_Fstep_Buffers() */

/*-----------------------------------------------------------------------*/

/**
 * Save_Crnt_Data() - Save current crnt data for a frequency step
 *
 * @fstep: Frequency step index
 *
 * Copies the global crnt struct arrays into crnt_fstep[fstep].
 */
  void
Save_Crnt_Data( int fstep )
{
  if( crnt_fstep == NULL
      || fstep < 0 || fstep > calc_data.steps_total
      || crnt_fstep[fstep].cur == NULL )
    return;

  size_t npm = (size_t)data.npm;
  size_t np3m = (size_t)data.np3m;

  memcpy( crnt_fstep[fstep].air, crnt.air, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].aii, crnt.aii, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].bir, crnt.bir, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].bii, crnt.bii, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].cir, crnt.cir, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].cii, crnt.cii, npm * sizeof(double) );
  memcpy( crnt_fstep[fstep].cur, crnt.cur, np3m * sizeof(complex double) );

} /* Save_Crnt_Data() */

/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

/*  New_Structure_Projection_Angle()
 *
 *  Calculates new projection parameters when a
 *  structure projection angle (Wr or Wi) changes
 */
  void
New_Structure_Projection_Angle(void)
{
  /* sin and cos of structure rotation and inclination angles */
  structure_proj_params.sin_wr = sin(structure_proj_params.Wr/(double)TODEG);
  structure_proj_params.cos_wr = cos(structure_proj_params.Wr/(double)TODEG);
  structure_proj_params.sin_wi = sin(structure_proj_params.Wi/(double)TODEG);
  structure_proj_params.cos_wi = cos(structure_proj_params.Wi/(double)TODEG);

  /* Trigger a redraw of structure drawingarea */
  if( structure_drawingarea && isFlagClear(INPUT_PENDING) )
  {
    xnec2_widget_queue_draw( structure_drawingarea, TRUE );
  }

  /* Trigger a redraw of plots drawingarea */
  if( isFlagSet(PLOT_ENABLED) &&
      isFlagSet(PLOT_GVIEWER) &&
      isFlagClear(SUPPRESS_INTERMEDIATE_REDRAWS) )
  {
    xnec2_widget_queue_draw( freqplots_drawingarea, TRUE );
  }

} /* New_Structure_Projection_Angle() */

/*-----------------------------------------------------------------------*/

/*  Init_Struct_Drawing()
 *
 *  Initializes drawing parameters after geometry input
 */
  void
Init_Struct_Drawing( void )
{
  /* We need n segs for wires + 2m for patces */
  size_t mreq = (size_t)(data.n + 2*data.m) * sizeof(Segment_t);
  mem_realloc( (void **)&structure_segs, mreq, "in draw_structure.c" );
  New_Wire_Data();
  New_Patch_Data();
}

/*-----------------------------------------------------------------------*/

/* Show_Viewer_Gain()
 *
 * Shows gain in direction of viewer
 */
  void
Show_Viewer_Gain(
    GtkBuilder *builder,
    gchar *widget,
    projection_parameters_t proj_params )
{
  if( isFlagSet(DRAW_CURRENTS) ||
      isFlagSet(DRAW_CHARGES)  ||
      isFlagSet(DRAW_GAIN)     ||
      isFlagSet(FREQ_LOOP_RUNNING) )
  {
    char txt[16];
    if( isFlagSet(ENABLE_RDPAT) && (calc_data.freq_step >= 0) )
    {
      snprintf( txt, sizeof(txt)-1, "%.2f", Viewer_Gain(proj_params, calc_data.freq_step) );
      gtk_entry_set_text( GTK_ENTRY(Builder_Get_Object(builder, widget)), txt );
    }
  }

} /* Show_Viewer_Gain() */

/*-----------------------------------------------------------------------*/

