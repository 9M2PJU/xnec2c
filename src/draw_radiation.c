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

#include "draw_radiation.h"
#include "measurements.h"
#include "shared.h"

#ifdef HAVE_OPENGL
#include "opengl_rdpattern.h"
#endif

/* Constants for display layout */
#define TEXT_GRADIENT_SPACING 8  /* Spacing between text and gradient bar in pixels */
#define LINE_WIDTH 2            /* Width of lines in pixels */

static const char *nearfield_animation_error_msg =
  N_("Animation requires near field data.\n\n"
     "E-field animation: Add NE card to NEC file\n"
     "H-field animation: Add NH card to NEC file\n"
     "Poynting vector: Add both NE and NH cards");

/* For coloring rad pattern */
static double *red = NULL, *grn = NULL, *blu = NULL;

/* Function prototypes */
void Draw_Color_Legend_Overlay( cairo_t *cr );

int need_rdpat_redraw = 1;

/* Buffered points in 3d (xyz) space
 * forming the radiation pattern */
static point_3d_t *point_3d = NULL;

/* Cached radiation pattern range values */
static double cached_r_min = 0.0;
static double cached_r_range = 1.0;

/* Generation counter for radiation pattern data
 * Incremented each time point_3d is regenerated
 * Allows renderers to track independently whether data has changed */
static unsigned int rdpat_gen_counter = 0;

/*-----------------------------------------------------------------------*/

/* Scale_Gain()
 *
 * Scales radiation pattern gain according to selected style
 * ( ARRL style, logarithmic or linear voltage/power )
 */
double Scale_Gain( double gain, int fstep, int idx )
{
  /* Scaled rad pattern gain and pol factor */
  double scaled_rad = 0.0;

  gain += Polarization_Factor( calc_data.pol_type, fstep, idx );

  switch( rc_config.gain_style )
  {
    case GS_LINP:
      scaled_rad = pow(10.0, (gain/10.0));
      break;

    case GS_LINV:
      scaled_rad = pow(10.0, (gain/20.0));
      break;

    case GS_ARRL:
      scaled_rad = exp( 0.058267 * gain );
      break;

    case GS_LOG:
      scaled_rad = gain;
      if( scaled_rad < -40 )
        scaled_rad = 0.0;
      else
        scaled_rad = scaled_rad /40.0 + 1.0;
      break;

    /* Noise temperature gain scales */
    case GS_NOISE:
    case GS_NOISE_LOG:
      {
        double t_sky, t_earth;
        if (ant_temp_resolve(save.freq[fstep], rc_config.ant_temp_env,
              &t_sky, &t_earth) < 0)
        {
          static gboolean warned = FALSE;
          if (!warned)
          {
            warned = TRUE;
            pr_warn("Scale_Gain: ant_temp_resolve failed for %.3f MHz env %d\n",
                save.freq[fstep], rc_config.ant_temp_env);
          }
          scaled_rad = 0.0;
          break;
        }

        int ith = idx % fpat.nth;
        int iph = idx / fpat.nth;
        double tht = (fpat.thets + ith * fpat.dth) * M_PI / 180.0;
        double phi = (fpat.phis  + iph * fpat.dph) * M_PI / 180.0;

        int pol = calc_data.pol_type;
        double tht_mg = rad_pattern[fstep].max_gain_tht[pol] * M_PI / 180.0;
        double phi_mg = rad_pattern[fstep].max_gain_phi[pol] * M_PI / 180.0;
        double elev_rad = rc_config.ant_temp_elevation * M_PI / 180.0;

        double z_w = ant_temp_z_world(tht, phi, tht_mg, phi_mg, elev_rad);
        double t_bright = (z_w > ANT_TEMP_Z_EPSILON) ? t_sky : t_earth;
        double g_lin = pow(10.0, gain / 10.0);

        /* Noise temperature density (K/sr): resolution-independent */
        double cell_k = g_lin * t_bright * sin(tht) / (4.0 * M_PI);

        if (rc_config.gain_style == GS_NOISE)
          scaled_rad = cell_k;
        else
          /* Log-compressed for visualization; recoverable to K */
          scaled_rad = log10(1.0 + cell_k);
      }
      break;

    default:
      scaled_rad = 0.0;
      break;

  } /* switch( rc_config.gain_style ) */

  return( scaled_rad );

} /* Scale_Gain() */

/*-----------------------------------------------------------------------*/

/* Generate_Rdpattern_Data()
 *
 * Converts spherical radiation pattern data to cartesian point_3d buffer
 * Returns generation counter (0 if no new data generated)
 * Always populates out_r_min and out_r_range from cache if non-NULL
 */
  unsigned int
Generate_Rdpattern_Data(double *out_r_min, double *out_r_range)
{
  int idx, nth, nph, pts_idx, fstep, pol;
  double theta, phi, r, r_min, r_max;
  double dth, dph;
  unsigned int gen_counter;

  gen_counter = 0;
  fstep = calc_data.freq_step;

  if( isFlagSet(ENABLE_RDPAT) && (fstep >= 0) )
  {
    pol = calc_data.pol_type;

    if( isFlagSet(DRAW_NEW_RDPAT) )
    {
      size_t mreq = ((size_t)(fpat.nth * fpat.nph)) * sizeof(point_3d_t);
      mem_realloc( (void **)&point_3d, mreq, "in draw_radiation.c" );

      idx = rad_pattern[fstep].max_gain_idx[pol];
      r_max = Scale_Gain( rad_pattern[fstep].gtot[idx], fstep, idx);

      idx = rad_pattern[fstep].min_gain_idx[pol];
      double actual_gain = rad_pattern[fstep].gtot[idx];
      double color_gain = (actual_gain < COLOR_MIN_GAIN) ? COLOR_MIN_GAIN : actual_gain;
      r_min = Scale_Gain(color_gain, fstep, idx);

      cached_r_min = r_min;
      cached_r_range = r_max - r_min;

      rdpattern_proj_params.r_max = r_max;
      New_Projection_Parameters(
          rdpattern_width,
          rdpattern_height,
          &rdpattern_proj_params );

      dth = (double)fpat.dth * (double)TORAD;
      dph = (double)fpat.dph * (double)TORAD;

      pts_idx = 0;
      phi = (double)(fpat.phis * TORAD);

      /* Noise-mode rotation parameters: rotate display coordinates so
       * the pattern visually tilts upward by the elevation angle while
       * the sky/earth boundary remains at the horizontal plane. */
      int noise_rotate = IS_NOISE_MODE(rc_config.gain_style);
      double rot_tht_mg = 0.0, rot_phi_mg = 0.0, rot_elev = 0.0;
      if (noise_rotate)
      {
        rot_tht_mg = rad_pattern[fstep].max_gain_tht[pol] * (double)TORAD;
        rot_phi_mg = rad_pattern[fstep].max_gain_phi[pol] * (double)TORAD;
        rot_elev   = rc_config.ant_temp_elevation * (double)TORAD;
      }

      for( nph = 0; nph < fpat.nph; nph++ )
      {
        theta = (double)(fpat.thets * TORAD);

        for( nth = 0; nth < fpat.nth; nth++ )
        {
          r = Scale_Gain( rad_pattern[fstep].gtot[pts_idx], fstep, pts_idx );

          point_3d[pts_idx].r = r;

          /* Place cell at rotated position in noise mode so the pattern
           * visually tilts upward; otherwise use original (θ, φ). */
          if (noise_rotate && rot_elev != 0.0)
          {
            double xr, yr, zr;
            ant_temp_rotate_point(theta, phi,
                rot_tht_mg, rot_phi_mg, rot_elev,
                &xr, &yr, &zr);
            point_3d[pts_idx].x = r * xr;
            point_3d[pts_idx].y = r * yr;
            point_3d[pts_idx].z = r * zr;
          }
          else
          {
            point_3d[pts_idx].z = r * cos(theta);
            double r_xy = r * sin(theta);
            point_3d[pts_idx].x = r_xy * cos(phi);
            point_3d[pts_idx].y = r_xy * sin(phi);
          }

          theta += dth;
          pts_idx++;
        }

        phi += dph;
      }

      /* Noise modes: scan all cells for true scaled min/max.
       * In noise mode Scale_Gain depends on gain, t_bright, and sin(θ),
       * so the dBi peak index does not determine the noise density peak. */
      if (IS_NOISE_MODE(rc_config.gain_style))
      {
        int total = fpat.nth * fpat.nph;
        double nmax = point_3d[0].r;
        double nmin = point_3d[0].r;
        for (int i = 1; i < total; i++)
        {
          if (point_3d[i].r > nmax)
            nmax = point_3d[i].r;
          if (point_3d[i].r < nmin)
            nmin = point_3d[i].r;
        }
        rad_pattern[fstep].noise_scaled_max = nmax;
        rad_pattern[fstep].noise_scaled_min = nmin;
        rdpattern_proj_params.r_max = nmax;
        cached_r_min = nmin;
        cached_r_range = nmax - nmin;

        New_Projection_Parameters(
            rdpattern_width,
            rdpattern_height,
            &rdpattern_proj_params);
      }

      ClearFlag( DRAW_NEW_RDPAT );
      rdpat_gen_counter++;
    }

    gen_counter = rdpat_gen_counter;
  }

  if( out_r_min ) *out_r_min = cached_r_min;
  if( out_r_range ) *out_r_range = cached_r_range;

  return( gen_counter );

} /* Generate_Rdpattern_Data() */

/*-----------------------------------------------------------------------*/

/* Draw_Radiation_Pattern()
 *
 * Draws the radiation pattern as a frame of line
 * segmants joining the points defined by spherical
 * co-ordinates theta, phi and r = gain(theta, phi)
 */
  static void
Draw_Radiation_Pattern( cairo_t *cr )
{
  /* Line segments to draw on Screen */
  Segment_t segm;

  int
    idx,
    nth,     /* Theta step count */
    nph,     /* Phi step count   */
    col_idx, /* Index to rad pattern color buffers */
    pts_idx; /* Index to rad pattern 3d-points buffer */

  /* Frequency step and polarization type */
  int fstep, pol;

  double r_min, r_range;

  /* Used to set text in labels */
  gchar txt[16];

  static unsigned int cairo_last_gen = 0;
  unsigned int current_gen;

  /* Abort if rad pattern cannot be drawn */
  fstep = calc_data.freq_step;
  if( isFlagClear(ENABLE_RDPAT) || (fstep < 0) )
    return;

  pol = calc_data.pol_type;

  current_gen = Generate_Rdpattern_Data(&r_min, &r_range);

  if( current_gen == 0 )
    return;

  if( current_gen != cairo_last_gen )
  {
    size_t mreq = (size_t)((fpat.nth-1) * fpat.nph + (fpat.nph-1) * fpat.nth);
    mreq *= sizeof(double);
    mem_realloc( (void **)&red, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&grn, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&blu, mreq, "in draw_radiation.c" );

    /* Calculate RGB value for rad pattern seg.
     * The average gain value of the two points
     * marking each line segment is used here */

    /* Pattern segment color in theta direction */
    col_idx = pts_idx = 0;
    for( nph = 0; nph < fpat.nph; nph++ )
    {
      for( nth = 1; nth < fpat.nth; nth++ )
      {
        Value_to_Color(
            &red[col_idx], &grn[col_idx], &blu[col_idx],
            ((point_3d[pts_idx].r+point_3d[pts_idx+1].r)/2.0-r_min)/r_range,
            1.0);
        col_idx++;
        pts_idx++;

      } /* for( nph = 0; nph < fpat.nph; nph++ ) */

      /* Needed because of "index look-ahead" above */
      pts_idx++;

    } /* for( nth = 1; nth < fpat.nth; nth++ ) */

    /* Pattern segment color in phi direction */
    for( nth = 0; nth < fpat.nth; nth++ )
    {
      pts_idx = nth;
      for( nph = 1; nph < fpat.nph; nph++ )
      {
        Value_to_Color(
            &red[col_idx], &grn[col_idx], &blu[col_idx],
            (point_3d[pts_idx].r +
             point_3d[pts_idx+fpat.nth].r)/2.0-r_min,
            r_range );
        col_idx++;

        /* Needed because of "index look-ahead" above */
        pts_idx += fpat.nth;

      } /* for( nth = 0; nth < fpat.nth; nth++ ) */
    } /* for( nph = 1; nph < fpat.nph; nph++ ) */

    /* Show max gain on color code bar */
    snprintf( txt, sizeof(txt)-1, "%.2f", rad_pattern[fstep].max_gain[pol] );
    gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
            rdpattern_window_builder, "rdpattern_colorcode_maxlabel")),
        txt );

    /* Show min gain on color code bar, clamped to COLOR_MIN_GAIN */
    double color_min_gain = rad_pattern[fstep].min_gain[pol];
    if (color_min_gain < COLOR_MIN_GAIN) color_min_gain = COLOR_MIN_GAIN;
    snprintf( txt, sizeof(txt)-1, "%.2f", color_min_gain );
    gtk_label_set_text(GTK_LABEL(Builder_Get_Object(
            rdpattern_window_builder, "rdpattern_colorcode_minlabel")),
        txt );

    cairo_last_gen = current_gen;

  } /* if( current_gen != cairo_last_gen ) */

  /* Draw xyz axes to Screen */
  Draw_XYZ_Axes( cr, rdpattern_proj_params );

  /* Overlay structure on Near Field pattern */
  if( isFlagSet(OVERLAY_STRUCT) )
  {
    /* Save structure projection params pointers */
    projection_parameters_t params = structure_proj_params;

    /* Divert structure drawing to rad pattern area */
    structure_proj_params = rdpattern_proj_params;
    structure_proj_params.r_max = params.r_max;
    structure_proj_params.xy_scale =
      params.xy_scale1 * rdpattern_proj_params.xy_zoom;

    /* Process and draw geometry if enabled */
    Process_Wire_Segments();
    Process_Surface_Patches();
    Draw_Surface_Patches( cr, structure_segs+data.n, data.m );
    Draw_Wire_Segments( cr, structure_segs, data.n );

    /* Restore structure projection params */
    structure_proj_params = params;

  } /* if( isFlagSet(OVERLAY_STRUCT) ) */

  /*** Draw rad pattern on screen ***/
  /* Draw segments along theta direction */
  col_idx = pts_idx = 0;
  for( nph = 0; nph < fpat.nph; nph++ )
  {
    for( nth = 1; nth < fpat.nth; nth++ )
    {
      /* Project line segment to Screen */
      Set_Gdk_Segment(
          &segm,
          &rdpattern_proj_params,
          point_3d[pts_idx].x,
          point_3d[pts_idx].y,
          point_3d[pts_idx].z,
          point_3d[pts_idx+1].x,
          point_3d[pts_idx+1].y,
          point_3d[pts_idx+1].z );
      pts_idx++;

      /* Draw segment */
      cairo_set_source_rgb( cr, red[col_idx], grn[col_idx], blu[col_idx] );
      Cairo_Draw_Line( cr, segm.x1, segm.y1, segm.x2, segm.y2 );
      col_idx++;

    } /* for( nth = 1; nth < fpat.nth; nth++ ) */

    pts_idx++;
  } /* for( nph = 0; nph < fpat.nph; nph++ ) */

  /* Draw segments along phi direction */
  for( nth = 0; nth < fpat.nth; nth++ )
  {
    pts_idx = nth;
    for( nph = 1; nph < fpat.nph; nph++ )
    {
      /* Project line segment to Screen */
      Set_Gdk_Segment(
          &segm,
          &rdpattern_proj_params,
          point_3d[pts_idx].x,
          point_3d[pts_idx].y,
          point_3d[pts_idx].z,
          point_3d[pts_idx+fpat.nth].x,
          point_3d[pts_idx+fpat.nth].y,
          point_3d[pts_idx+fpat.nth].z );

      /* Draw segment */
      cairo_set_source_rgb( cr, red[col_idx], grn[col_idx], blu[col_idx] );
      Cairo_Draw_Line( cr, segm.x1, segm.y1, segm.x2, segm.y2 );
      col_idx++;

      /* Needed because drawing segments "looks ahead"
       * in the 3d points buffer in the above loop */
      pts_idx += fpat.nth;

    } /* for( nph = 1; nph < fpat.nph; nph++ ) */
  } /* for( nth = 0; nth < fpat.nth; nth++ ) */

  /* Draw color legend overlay */
  Draw_Color_Legend_Overlay( cr );

  /* Show gain in direction of viewer */
  Show_Viewer_Gain(
      rdpattern_window_builder,
      "rdpattern_viewer_gain",
      rdpattern_proj_params );

  /* Update T_total readout in toolbar */
  {
    measurement_t meas = { .a = {0} };
    meas_calc(&meas, fstep);
    GtkWidget *temp_entry = Builder_Get_Object(
        rdpattern_window_builder, "rdpattern_ant_temp_entry");
    if (temp_entry)
    {
      char buf[24];
      if (meas.ant_temp_tot >= 0.0)
        snprintf(buf, sizeof(buf), "%.0f K", meas.ant_temp_tot);
      else
        snprintf(buf, sizeof(buf), "— K");
      gtk_entry_set_text(GTK_ENTRY(temp_entry), buf);
    }
  }

} /* Draw_Radiation_Pattern() */

/*-----------------------------------------------------------------------*/

/* Draw_Near_Field()
 *
 * Draws near E/H fields and Poynting vector
 */
  static void
Draw_Near_Field( cairo_t *cr )
{
  int idx, npts; /* Number of points to plot */
  double
    fx, fy, fz, /* Co-ordinates of "free" end of field lines */
    fscale;     /* Scale factor for equalizing field line segments */

  /* Scale factor ref, for normalizing field strength values */
  static double dr;

  /* Co-ordinates of Poynting vectors */
  static double *pov_x = NULL, *pov_y = NULL;
  static double *pov_z = NULL, *pov_r = NULL;

  /* Range of Poynting vector values,
   * its max and min and log of max/min */
  static double pov_max = 0, max;

  /* Used to set text in labels */
  gchar txt[9];

  /* Line segments to draw on Screen */
  Segment_t segm;

  /* For coloring field lines */
  double xred = 0.0, xgrn = 0.0, xblu = 0.0;

  /* Abort if drawing a near field pattern is not possible */
  if( isFlagClear(ENABLE_NEAREH) || !near_field.valid )
    return;

  /* Initialize projection parameters */
  if( isFlagSet(DRAW_NEW_EHFIELD) )
  {
    /* Reference for scale factor used in
     * normalizing field strength values */
    if( fpat.near ) /* Spherical co-ordinates */
      dr = (double)fpat.dxnr;
    else /* Rectangular co-ordinates */
      dr = sqrt(
          (double)fpat.dxnr * (double)fpat.dxnr +
          (double)fpat.dynr * (double)fpat.dynr +
          (double)fpat.dznr * (double)fpat.dznr )/1.75;

    /* Set radiation pattern projection parametrs */
    /* Distance of field point furthest from xyz origin */
    rdpattern_proj_params.r_max = near_field.r_max + dr;
    New_Projection_Parameters(
        rdpattern_width,
        rdpattern_height,
        &rdpattern_proj_params );

    ClearFlag( DRAW_NEW_EHFIELD );

  } /* if( isFlagSet( DRAW_NEW_EHFIELD ) */

  /* Draw xyz axes to Screen */
  Draw_XYZ_Axes( cr, rdpattern_proj_params );

  /* Overlay structure on Near Field pattern */
  if( isFlagSet(OVERLAY_STRUCT) )
  {
    /* Save projection params pointers */
    projection_parameters_t params = structure_proj_params;

    /* Divert structure drawing to rad pattern area */
    structure_proj_params = rdpattern_proj_params;

    /* Process and draw geometry if enabled */
    Process_Wire_Segments();
    Process_Surface_Patches();
    Draw_Surface_Patches( cr, structure_segs+data.n, data.m );
    Draw_Wire_Segments( cr, structure_segs, data.n );

    /* Restore structure params */
    structure_proj_params = params;
  } /* if( isFlagSet(OVERLAY_STRUCT) ) */

  /* Step thru near field values */
  npts = fpat.nrx * fpat.nry * fpat.nrz;
  for( idx = 0; idx < npts; idx++ )
  {
    /*** Draw Near E Field ***/
    if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) )
    {
      /* Set gc attributes for segment */
      Value_to_Color( &xred, &xgrn, &xblu,
          near_field.er[idx], near_field.max_er );

      /* Scale factor for each field point, to make
       * near field direction lines equal-sized */
      fscale = dr / near_field.er[idx];

      /* Scaled field values are used to set one end of a
       * line segment that represents direction of field.
       * The other end is set by the field point co-ordinates */
      fx = near_field.px[idx] + near_field.erx[idx] * fscale;
      fy = near_field.py[idx] + near_field.ery[idx] * fscale;
      fz = near_field.pz[idx] + near_field.erz[idx] * fscale;

      /* Project new line segment of
       * phi chain to the Screen */
      Set_Gdk_Segment(
          &segm, &rdpattern_proj_params,
          near_field.px[idx], near_field.py[idx], near_field.pz[idx],
          fx, fy, fz );

      /* Draw segment */
      cairo_set_source_rgb( cr, xred, xgrn, xblu );
      Cairo_Draw_Line( cr, segm.x1, segm.y1, segm.x2, segm.y2 );

    } /* if( isFlagSet(DRAW_EFIELD) && (fpat.nfeh & NEAR_EFIELD) ) */

    /*** Draw Near H Field ***/
    if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) )
    {
      /* Set gc attributes for segment */
      Value_to_Color( &xred, &xgrn, &xblu,
          near_field.hr[idx], near_field.max_hr );

      /* Scale factor for each field point, to make
       * near field direction lines equal-sized */
      fscale = dr / near_field.hr[idx];

      /* Scaled field values are used to set one end of a
       * line segment that represents direction of field.
       * The other end is set by the field point co-ordinates */
      fx = near_field.px[idx] + near_field.hrx[idx] * fscale;
      fy = near_field.py[idx] + near_field.hry[idx] * fscale;
      fz = near_field.pz[idx] + near_field.hrz[idx] * fscale;

      /* Project new line segment of
       * phi chain to the Screen */
      Set_Gdk_Segment(
          &segm, &rdpattern_proj_params,
          near_field.px[idx], near_field.py[idx], near_field.pz[idx],
          fx, fy, fz );

      /* Draw segment */
      cairo_set_source_rgb( cr, xred, xgrn, xblu );
      Cairo_Draw_Line( cr, segm.x1, segm.y1, segm.x2, segm.y2 );

    } /* if( isFlagSet(DRAW_HFIELD) && (fpat.nfeh & NEAR_HFIELD) ) */

    /*** Draw Poynting Vector ***/
    if( isFlagSet(DRAW_POYNTING)  &&
        (fpat.nfeh & NEAR_EFIELD) &&
        (fpat.nfeh & NEAR_HFIELD) )
    {
      int ipv; /* Mem request and index */
      static size_t mreq = 0;

      /* Allocate on new near field matrix size */
      if( !mreq || isFlagSet(ALLOC_PNTING_BUFF) )
      {
        mreq = (size_t)npts * sizeof( double );
        mem_realloc( (void **)&pov_x, mreq, "in draw_radiation.c" );
        mem_realloc( (void **)&pov_y, mreq, "in draw_radiation.c" );
        mem_realloc( (void **)&pov_z, mreq, "in draw_radiation.c" );
        mem_realloc( (void **)&pov_r, mreq, "in draw_radiation.c" );
        ClearFlag( ALLOC_PNTING_BUFF );
      }

      /* Calculate Poynting vector and its max and min */
      pov_max = 0;
      for( ipv = 0; ipv < npts; ipv++ )
      {
        pov_x[ipv] =
          near_field.ery[ipv] * near_field.hrz[ipv] -
          near_field.hry[ipv] * near_field.erz[ipv];
        pov_y[ipv] =
          near_field.erz[ipv] * near_field.hrx[ipv] -
          near_field.hrz[ipv] * near_field.erx[ipv];
        pov_z[ipv] =
          near_field.erx[ipv] * near_field.hry[ipv] -
          near_field.hrx[ipv] * near_field.ery[ipv];
        pov_r[ipv] = sqrt(
            pov_x[ipv] * pov_x[ipv] +
            pov_y[ipv] * pov_y[ipv] +
            pov_z[ipv] * pov_z[ipv] );
        if( pov_max < pov_r[ipv] )
          pov_max = pov_r[ipv];
      } /* for( ipv = 0; ipv < npts; ipv++ ) */

      /* Set gc attributes for segment */
      Value_to_Color( &xred, &xgrn, &xblu, pov_r[idx], pov_max );

      /* Scale factor for each field point, to make
       * near field direction lines equal-sized */
      fscale = dr / pov_r[idx];

      /* Scaled field values are used to set one end of a
       * line segment that represents direction of field.
       * The other end is set by the field point co-ordinates */
      fx = near_field.px[idx] + pov_x[idx] * fscale;
      fy = near_field.py[idx] + pov_y[idx] * fscale;
      fz = near_field.pz[idx] + pov_z[idx] * fscale;

      /* Project new line segment of
       * Poynting vector to the Screen */
      Set_Gdk_Segment(
          &segm,
          &rdpattern_proj_params,
          near_field.px[idx], near_field.py[idx],
          near_field.pz[idx], fx, fy, fz );

      /* Draw segment */
      cairo_set_source_rgb( cr, xred, xgrn, xblu );
      Cairo_Draw_Line( cr, segm.x1, segm.y1, segm.x2, segm.y2 );

    } /* if( isFlagSet(DRAW_POYNTING) ) */

  } /* for( idx = 0; idx < npts; idx++ ) */

  if( isFlagSet(NEAREH_ANIMATE) )
  {
    return;
  }

  /* Draw color legend overlay */
  Draw_Color_Legend_Overlay( cr );

  /* Show max field strength on color code bar */
  if( isFlagSet(DRAW_EFIELD) )
    max = near_field.max_er;
  else if( isFlagSet(DRAW_HFIELD) )
    max = near_field.max_hr;
  else if( isFlagSet(DRAW_POYNTING) )
    max = pov_max;

  snprintf( txt, sizeof(txt), "%8.2E", max );
  gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
          rdpattern_window_builder, "rdpattern_colorcode_maxlabel")),
      txt );

  /* Show min field strength on color code bar */
  gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
          rdpattern_window_builder, "rdpattern_colorcode_minlabel")),
      "0" );

} /* Draw_Near_Field() */

/*-----------------------------------------------------------------------*/

/* Draw_Radiation()
 *
 * Draws the radiation pattern or near E/H fields
 */
  int
_Draw_Radiation( cairo_t *cr )
{
  /* Clear drawingarea */
  cairo_set_source_rgb( cr, BLACK );
  cairo_rectangle(
      cr, 0.0, 0.0,
      (double)rdpattern_proj_params.width,
      (double)rdpattern_proj_params.height);
  cairo_fill( cr );

  /* Abort if excitation (EX card) is missing */
  if( isFlagClear(ENABLE_EXCITN) )
    return FALSE;

  // Try to hold the lock to prevent drawing the radiation pattern
  // since it could be drawing while inotify triggers a new freqloop:
  int locked = g_mutex_trylock(&global_lock);

  /* Draw rad pattern or E/H fields */
  if( isFlagSet(DRAW_GAIN) )
    Draw_Radiation_Pattern( cr );
  else if( isFlagSet(DRAW_EHFIELD) )
      Draw_Near_Field( cr );

  /* Display frequency step */
  if (calc_data.freq_step >= 0)
	  Display_Fstep( rdpattern_fstep_entry, calc_data.freq_step );

  if (locked)
	  g_mutex_unlock(&global_lock);

  return TRUE;

} /* Draw_Radiation() */

int Draw_Radiation( cairo_t *cr )
{
	int ret;

	if (isFlagSet(ERROR_CONDX))
		return FALSE;

	need_rdpat_redraw = 0;

	g_mutex_lock(&freq_data_lock);
	ret = _Draw_Radiation( cr );
	g_mutex_unlock(&freq_data_lock);

	return ret;
}

/*-----------------------------------------------------------------------*/

gboolean
Validate_Nearfield_Animation( void )
{
  if( ((isFlagSet(DRAW_EFIELD) || isFlagSet(DRAW_POYNTING)) && !(fpat.nfeh & NEAR_EFIELD)) ||
      ((isFlagSet(DRAW_HFIELD) || isFlagSet(DRAW_POYNTING)) && !(fpat.nfeh & NEAR_HFIELD)) )
  {
    Notice( GTK_BUTTONS_OK, _("Near Field Animation"), "%s",
        _(nearfield_animation_error_msg) );
    return( FALSE );
  }

  if( ((isFlagSet(DRAW_EFIELD) || isFlagSet(DRAW_POYNTING)) &&
       (near_field.fex == NULL || near_field.fey == NULL || near_field.fez == NULL)) ||
      ((isFlagSet(DRAW_HFIELD) || isFlagSet(DRAW_POYNTING)) &&
       (near_field.fhx == NULL || near_field.fhy == NULL || near_field.fhz == NULL)) )
  {
    Notice( GTK_BUTTONS_OK, _("Near Field Animation"), "%s",
        _(nearfield_animation_error_msg) );
    return( FALSE );
  }

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

  gboolean
Animate_Near_Field( gpointer udata )
{
  /* omega*t = 2*pi*f*t = Time-varying phase of excitation */
  static double wt = 0.0;
  int idx, npts;

  if( isFlagClear(NEAREH_ANIMATE) )
    return( FALSE );

  if( !Validate_Nearfield_Animation() )
  {
    ClearFlag( NEAREH_ANIMATE );
    if( anim_tag > 0 )
    {
      g_source_remove( anim_tag );
      anim_tag = 0;
    }
    if( animate_dialog != NULL )
      Gtk_Widget_Destroy( &animate_dialog );
    return( FALSE );
  }

  /* Number of points in near fields */
  npts = fpat.nrx * fpat.nry * fpat.nrz;
  for( idx = 0; idx < npts; idx++ )
  {
    if( isFlagSet(DRAW_EFIELD) || isFlagSet(DRAW_POYNTING) )
    {
      /* Real component of complex E field strength */
      near_field.erx[idx] = near_field.ex[idx] *
        cos( wt + near_field.fex[idx] );
      near_field.ery[idx] = near_field.ey[idx] *
        cos( wt + near_field.fey[idx] );
      near_field.erz[idx] = near_field.ez[idx] *
        cos( wt + near_field.fez[idx] );

      /* Near total electric field vector */
      near_field.er[idx]  = sqrt(
          near_field.erx[idx] * near_field.erx[idx] +
          near_field.ery[idx] * near_field.ery[idx] +
          near_field.erz[idx] * near_field.erz[idx] );
      if( near_field.max_er < near_field.er[idx] )
        near_field.max_er = near_field.er[idx];
    }

    if( isFlagSet(DRAW_HFIELD) || isFlagSet(DRAW_POYNTING) )
    {
      /* Real component of complex H field strength */
      near_field.hrx[idx] = near_field.hx[idx] *
        cos( wt + near_field.fhx[idx] );
      near_field.hry[idx] = near_field.hy[idx] *
        cos( wt + near_field.fhy[idx] );
      near_field.hrz[idx] = near_field.hz[idx] *
        cos( wt + near_field.fhz[idx] );

      /* Near total magnetic field vector*/
      near_field.hr[idx]  = sqrt(
          near_field.hrx[idx] * near_field.hrx[idx] +
          near_field.hry[idx] * near_field.hry[idx] +
          near_field.hrz[idx] * near_field.hrz[idx] );
      if( near_field.max_hr < near_field.hr[idx] )
        near_field.max_hr = near_field.hr[idx];
    }

  } /* for( idx = 0; idx < npts; idx++ ) */

  /* Increment excitation phase, keep < 2pi */
  wt += near_field.anim_step;
  if( wt >= (double)M_2PI )
    wt = 0.0;

  xnec2_widget_queue_draw( rdpattern_drawingarea );

  return( TRUE );

} /* Animate_Near_Field() */

/*-----------------------------------------------------------------------*/

/* Polarization_Factor()
 *
 * Calculates polarization factor from axial
 * ratio and tilt of polarization ellipse
 */
  double
Polarization_Factor( int pol_type, int fstep, int idx )
{
  double axrt, axrt2, tilt2, polf = 1.0;

  switch( pol_type )
  {
    case POL_TOTAL:
      polf = 1.0;
      break;

    case POL_HORIZ:
      axrt2  = rad_pattern[fstep].axrt[idx];
      axrt2 *= axrt2;
      tilt2  = sin( rad_pattern[fstep].tilt[idx] );
      tilt2 *= tilt2;
      polf = (axrt2 + (1.0 - axrt2) * tilt2) / (1.0 + axrt2);
      break;

    case POL_VERT:
      axrt2  = rad_pattern[fstep].axrt[idx];
      axrt2 *= axrt2;
      tilt2  = cos( rad_pattern[fstep].tilt[idx] );
      tilt2 *= tilt2;
      polf = (axrt2 + (1.0 - axrt2) * tilt2) / (1.0 + axrt2);
      break;

    case POL_LHCP:
      axrt  = rad_pattern[fstep].axrt[idx];
      axrt2 = axrt * axrt;
      polf  = (1.0 + 2.0 * axrt + axrt2) / 2.0 / (1.0 + axrt2);
      break;

    case POL_RHCP:
      axrt  = rad_pattern[fstep].axrt[idx];
      axrt2 = axrt * axrt;
      polf  = (1.0 - 2.0 * axrt + axrt2) / 2.0 / (1.0 + axrt2);
  }

  if( polf < 1.0E-200 ) polf = 1.0E-200;
  polf = 10.0 * log10( polf );

  return( polf );
} /* Polarization_Factor() */

/*-----------------------------------------------------------------------*/

/* Set_Polarization()
 *
 * Sets the polarization type of gain to be plotted
 */

  void
Set_Polarization( int pol )
{
  calc_data.pol_type = pol;
  Set_Window_Labels();

  /* Show gain in direction of viewer */
  if( isFlagSet(INPUT_OPENED) )
    Show_Viewer_Gain( main_window_builder, "main_gain_entry", structure_proj_params );

  /* Enable redraw of rad pattern */
  SetFlag( DRAW_NEW_RDPAT );

  /* Trigger a redraw of drawingareas */
  if( isFlagSet(DRAW_ENABLED) )
  {
    xnec2_widget_queue_draw( rdpattern_drawingarea );
  }

  if( isFlagSet(PLOT_ENABLED) )
  {
    xnec2_widget_queue_draw( freqplots_drawingarea );
  }

} /* Set_Polarization() */

/*-----------------------------------------------------------------------*/

/* Set_Gain_Style()
 *
 * Sets the radiation pattern Gain scaling style
 */
  void
Set_Gain_Style( int gs )
{
  static char *scale_widget_names[NUM_SCALES] = {
	  "rdpattern_linear_power",
	  "rdpattern_linear_voltage",
	  "rdpattern_arrl_style",
	  "rdpattern_logarithmic",
	  "rdpattern_noise_temp",
	  "rdpattern_noise_temp_log",
	  };

  GtkWidget *widget;

  // This should never happen:
  if (gs >= NUM_SCALES)
	  return;

  rc_config.gain_style = gs;

  widget = Builder_Get_Object( rdpattern_window_builder, scale_widget_names[rc_config.gain_style] );
  gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(widget), TRUE );

  /* Update units label and row label for noise vs gain modes */
  widget = Builder_Get_Object( rdpattern_window_builder, "rdpattern_gain_units_label" );
  if (widget)
    gtk_label_set_text( GTK_LABEL(widget),
        IS_NOISE_MODE(gs) ? "K/sr" : "dB" );

  widget = Builder_Get_Object( rdpattern_window_builder, "rdpattern_gain_row_label" );
  if (widget)
    gtk_label_set_text( GTK_LABEL(widget),
        IS_NOISE_MODE(gs) ? "Noise" : "Gain" );

  /* Desensitize noise-specific controls when not in noise mode */
  gboolean noise = IS_NOISE_MODE(gs);

  static const struct {
    const char *widget_id;
    const char *tooltip;
    const char *disabled_tooltip;
  } noise_widgets[] = {
    { "rdpattern_elevation_spinbutton",
      N_("Observation elevation angle. Shifts sky/earth "
         "boundary for antenna temperature evaluation. "
         "0° = horizon."),
      N_("Observation elevation for antenna temperature.\n"
         "Select Gain Style → Noise Temperature to enable.") },
    { "rdpattern_elevation_label",
      N_("Observation elevation angle. Shifts sky/earth "
         "boundary for antenna temperature evaluation. "
         "0° = horizon."),
      N_("Observation elevation for antenna temperature.\n"
         "Select Gain Style → Noise Temperature to enable.") },
    { "rdpattern_noise_env_menu",
      N_("Select brightness temperature model "
         "for T_sky and T_earth."),
      N_("Brightness temperature model for T_sky and T_earth.\n"
         "Select Gain Style → Noise Temperature to enable.") },
  };

  int n_noise_widgets = sizeof(noise_widgets) / sizeof(noise_widgets[0]);
  for (int i = 0; i < n_noise_widgets; i++)
  {
    widget = Builder_Get_Object(
        rdpattern_window_builder, (gchar *)noise_widgets[i].widget_id);
    if (widget)
    {
      gtk_widget_set_sensitive(widget, noise);
      gtk_widget_set_tooltip_text(widget,
          noise ? _(noise_widgets[i].tooltip) : _(noise_widgets[i].disabled_tooltip));
    }
  }

  /* Check for model compatibility warnings when entering noise mode */
  if (noise)
    Check_Noise_Warnings(calc_data.freq_step);

  Set_Window_Labels();

  /* Trigger a redraw of drawingarea */
  if( isFlagSet(DRAW_ENABLED) )
  {
    /* Enable redraw of rad pattern */
    SetFlag( DRAW_NEW_RDPAT );

    xnec2_widget_queue_draw( rdpattern_drawingarea );
  }

} /* Set_Gain_Style() */

/*-----------------------------------------------------------------------*/

/*  New_Radiation_Projection_Angle()
 *
 *  Calculates new projection parameters when a
 *  structure projection angle (Wr or Wi) changes
 */
  void
New_Radiation_Projection_Angle(void)
{
  /* sin and cos of rad pattern rotation and inclination angles */
  rdpattern_proj_params.sin_wr = sin(rdpattern_proj_params.Wr/(double)TODEG);
  rdpattern_proj_params.cos_wr = cos(rdpattern_proj_params.Wr/(double)TODEG);
  rdpattern_proj_params.sin_wi = sin(rdpattern_proj_params.Wi/(double)TODEG);
  rdpattern_proj_params.cos_wi = cos(rdpattern_proj_params.Wi/(double)TODEG);

  need_rdpat_redraw = 1;

  /* Noise modes require full recomputation of scaled min/max */
  if (IS_NOISE_MODE(rc_config.gain_style))
  {
    SetFlag( DRAW_NEW_RDPAT );
  }

  /* Trigger a redraw of radiation drawingarea */
  if( isFlagSet(DRAW_ENABLED) )
  {
    xnec2_widget_queue_draw( rdpattern_drawingarea );
  }

  /* Trigger a redraw of plots drawingarea if doing "viewer" gain
   * or antenna temperature (noise env / elevation changes affect T_ant) */
  if( isFlagSet(PLOT_ENABLED) &&
      (isFlagSet(PLOT_GVIEWER) || isFlagSet(PLOT_ANT_TEMP)) &&
      isFlagClear(SUPPRESS_INTERMEDIATE_REDRAWS) )
  {
    xnec2_widget_queue_draw( freqplots_drawingarea );
  }

} /* New_Radiation_Projection_Angle() */

/*-----------------------------------------------------------------------*/

/* Redo_Radiation_Pattern()
 *
 * Refreshes radiation pattern on new frequency in spinbutton
 */
  gboolean
Redo_Radiation_Pattern( gpointer udata )
{
  // Only re-calculate if the set_freq_step() determines that
  // the selected frequency has not been calculated.
  if (!set_freq_step())
	  New_Frequency();

  /* Redraw radiation pattern on screen */
  if( isFlagSet(DRAW_ENABLED) )
  {
    xnec2_widget_queue_draw( rdpattern_drawingarea );
  }

  return FALSE;

} /* Redo_Radiation_Pattern() */

/*-----------------------------------------------------------------------*/

/* Viewer_Gain()
 *
 * Calculate gain in direction of viewer
 * (e.g. Perpenticular to the Screen)
 */
  double
Viewer_Gain( projection_parameters_t proj_parameters, int fstep )
{
  double phi, gain;
  int nth, nph, idx;

  /* Calculate theta step from proj params */
  phi = proj_parameters.Wr;
  if( fpat.dth == 0.0 ) nth = 0;
  else
  {
    double theta;
    theta = fabs( 90.0 - proj_parameters.Wi );
    if( theta > 180.0 )
    {
      theta = 360.0 - theta;
      phi  -= 180.0;
    }

    if( (gnd.ksymp == 2) &&
        (theta > 90.01)  &&
        (gnd.ifar != 1) )
      return( -999.99 );

    nth = (int)( (theta - fpat.thets) / fpat.dth + 0.5 );
    if( (nth >= fpat.nth) || (nth < 0) )
      nth = fpat.nth-1;
  }

  /* Calculate phi step from proj params */
  if( fpat.dph == 0.0 ) nph = 0;
  else
  {
    while( phi < 0.0 ) phi += 360.0;
    nph = (int)( (phi - fpat.phis) / fpat.dph + 0.5 );
    if( (nph >= fpat.nph) || (nph < 0) )
      nph = fpat.nph-1;
  }

  idx = nth + nph * fpat.nth;
  gain = rad_pattern[fstep].gtot[idx] +
    Polarization_Factor(calc_data.pol_type, fstep, idx);
  if( gain < -999.99 ) gain = -999.99;

  return( gain );

} /* Viewer_Gain() */

/*-----------------------------------------------------------------------*/

/* Rdpattern_Window_Killed()
 *
 * Cleans up after the rad pattern window is closed
 */
  void
Rdpattern_Window_Killed( void )
{
  rc_config.rdpattern_width  = 0;
  rc_config.rdpattern_height = 0;

  if( animate_dialog != NULL )
  {
    Gtk_Widget_Destroy( &animate_dialog );
    ClearFlag( NEAREH_ANIMATE );
    if( anim_tag ) g_source_remove( anim_tag );
    anim_tag = 0;
  }

  if( isFlagSet(DRAW_ENABLED) )
  {
    ClearFlag( DRAW_FLAGS );
    rdpattern_drawingarea = NULL;
    g_object_unref( rdpattern_window_builder );
    rdpattern_window_builder = NULL;
    Free_Draw_Buffers();

    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(
          Builder_Get_Object( main_window_builder, "main_rdpattern")), FALSE );
  }
  rdpattern_window = NULL;
  kill_window = NULL;

} /* Rdpattern_Window_Killed() */

/*-----------------------------------------------------------------------*/

/* Set_Window_Labels()
 *
 * Sets radiation pattern window labels
 * according to what is being drawn.
 */
  void
Set_Window_Labels( void )
{
  char *pol_type[NUM_POL] =
  {
    _("Total Gain"),
    _("Horizontal Polarization"),
    _("Vertical Polarization"),
    _("RH Circular Polarization"),
    _("LH Circular Polarization")
  };

  char *scale[NUM_SCALES] =
  {
    _("Linear Power"),
    _("Linear Voltage"),
    _("ARRL Scale"),
    _("Logarithmic Scale"),
    _("Noise Temperature"),
    _("Noise Temp (log scale)"),
  };

  char txt[64];
  size_t s = sizeof( txt );

  if( isFlagSet(DRAW_ENABLED) )
  {
    /* Set window labels */
    Strlcpy( txt, _("Radiation Patterns"), s );
    if( isFlagSet(DRAW_GAIN) )
    {
      Strlcpy( txt, _("Radiation Pattern: - "), s );
      Strlcat( txt, pol_type[calc_data.pol_type], s );
      Strlcat( txt, " - ", s );
      Strlcat( txt, scale[rc_config.gain_style], s );
    }
    else if( isFlagSet(DRAW_EHFIELD) )
    {
      Strlcpy( txt, _("Near Fields:"), s );
      if( isFlagSet(DRAW_EFIELD) )
        Strlcat( txt, _(" - E Field"), s );
      if( isFlagSet(DRAW_HFIELD) )
        Strlcat( txt, _(" - H Field"), s );
      if( isFlagSet(DRAW_POYNTING) )
        Strlcat( txt, _(" - Poynting Vector"), s );
    }

    gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
            rdpattern_window_builder, "rdpattern_label")), txt );

  } /* if( isFlagSet(DRAW_ENABLED) ) */

  if( isFlagSet(PLOT_ENABLED) )
  {
    Strlcpy( txt, _("Frequency Data Plots - "), s );
    Strlcat( txt, pol_type[calc_data.pol_type], s );
    gtk_label_set_text( GTK_LABEL(Builder_Get_Object(
            freqplots_window_builder, "freqplots_label")), txt );
  }

} /* Set_Window_Labels() */

/*-----------------------------------------------------------------------*/

/* Alloc_Rdpattern_Buffers
 *
 * Allocates memory to the radiation pattern buffers
 */
  void
_Alloc_Rdpattern_Buffers( int nfrq, int nth, int nph )
{
  int idx;
  size_t mreq;
  static int last_nfrq = 0;

  /* Free old gain buffers first */
  for( idx = 0; idx < last_nfrq; idx++ )
  {
    free_ptr( (void **)&rad_pattern[idx].gtot );
    free_ptr( (void **)&rad_pattern[idx].max_gain );
    free_ptr( (void **)&rad_pattern[idx].min_gain );
    free_ptr( (void **)&rad_pattern[idx].max_gain_tht );
    free_ptr( (void **)&rad_pattern[idx].max_gain_phi );
    free_ptr( (void **)&rad_pattern[idx].max_gain_idx );
    free_ptr( (void **)&rad_pattern[idx].min_gain_idx );
    free_ptr( (void **)&rad_pattern[idx].axrt );
    free_ptr( (void **)&rad_pattern[idx].tilt );
    free_ptr( (void **)&rad_pattern[idx].sens );
  }
  last_nfrq = nfrq;

  /* Allocate rad pattern buffers */
  mreq = (size_t)nfrq * sizeof(rad_pattern_t);
  mem_realloc( (void **)&rad_pattern, mreq, "in draw_radiation.c" );
  for( idx = 0; idx < nfrq; idx++ )
  {
    /* Memory request for allocs */
    mreq = (size_t)(nph * nth) * sizeof(double);
    rad_pattern[idx].gtot = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].gtot), mreq, "in draw_radiation.c" );
    rad_pattern[idx].axrt = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].axrt), mreq, "in draw_radiation.c" );
    rad_pattern[idx].tilt = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].tilt), mreq, "in draw_radiation.c" );

    mreq = NUM_POL * sizeof(double);
    rad_pattern[idx].max_gain = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].max_gain), mreq, "in draw_radiation.c" );
    rad_pattern[idx].min_gain = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].min_gain), mreq, "in draw_radiation.c" );
    rad_pattern[idx].max_gain_tht = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].max_gain_tht), mreq, "in draw_radiation.c" );
    rad_pattern[idx].max_gain_phi = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].max_gain_phi), mreq, "in draw_radiation.c" );

    mreq = NUM_POL * sizeof(int);
    rad_pattern[idx].max_gain_idx = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].max_gain_idx), mreq, "in draw_radiation.c" );
    rad_pattern[idx].min_gain_idx = NULL;
    mem_alloc( (void **)&(rad_pattern[idx].min_gain_idx), mreq, "in draw_radiation.c" );

    rad_pattern[idx].sens = NULL;
    mreq = (size_t)(nph * nth) * sizeof(int);
    mem_alloc( (void **)&(rad_pattern[idx].sens), mreq, "in draw_radiation.c" );
  }

} /* Alloc_Rdpattern_Buffers() */

void Alloc_Rdpattern_Buffers( int nfrq, int nth, int nph )
{
	g_mutex_lock(&freq_data_lock);
	_Alloc_Rdpattern_Buffers(nfrq, nth, nph);
	g_mutex_unlock(&freq_data_lock);
}

/*-----------------------------------------------------------------------*/

/* Alloc_Nearfield_Buffers
 *
 * Allocates memory to the radiation pattern buffers
 */
  void
Alloc_Nearfield_Buffers( int n1, int n2, int n3 )
{
  size_t mreq;

  if( isFlagClear(ALLOC_NEAREH_BUFF) ) return;
  ClearFlag( ALLOC_NEAREH_BUFF );

  /* Memory request for allocations */
  mreq = (size_t)(n1 * n2 * n3) * sizeof( double );

  /* Allocate near field buffers */
  if( fpat.nfeh & NEAR_EFIELD )
  {
    mem_realloc( (void **)&near_field.ex,  mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.ey,  mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.ez,  mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.fex, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.fey, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.fez, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.erx, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.ery, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.erz, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.er,  mreq, "in draw_radiation.c" );
  }

  if( fpat.nfeh & NEAR_HFIELD )
  {
    mem_realloc( (void **)&near_field.hx,  mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.hy,  mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.hz,  mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.fhx, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.fhy, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.fhz, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.hrx, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.hry, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.hrz, mreq, "in draw_radiation.c" );
    mem_realloc( (void **)&near_field.hr,  mreq, "in draw_radiation.c" );
  }

  mem_realloc( (void **)&near_field.px, mreq, "in draw_radiation.c" );
  mem_realloc( (void **)&near_field.py, mreq, "in draw_radiation.c" );
  mem_realloc( (void **)&near_field.pz, mreq, "in draw_radiation.c" );

} /* Alloc_Nearfield_Buffers() */

/*-----------------------------------------------------------------------*/

/* Inverse_Scale_Gain()
 *
 * Calculates the actual dB value from a scaled gain value
 * This is the inverse of Scale_Gain()
 */
static double
Inverse_Scale_Gain(double scaled_val)
{
  double db_val = 0.0;

  switch(rc_config.gain_style)
  {
    case GS_LINP:
      if (scaled_val > 0.0)
        db_val = 10.0 * log10(scaled_val);
      else
        db_val = -999.99;
      break;

    case GS_LINV:
      if (scaled_val > 0.0)
        db_val = 20.0 * log10(scaled_val);
      else
        db_val = -999.99;
      break;

    case GS_ARRL:
      if (scaled_val > 0.0)
        db_val = log(scaled_val) / 0.058267;
      else
        db_val = -999.99;
      break;

    case GS_LOG:
      db_val = (scaled_val - 1.0) * 40.0;
      break;

    /* Gain-weighted brightness temperature in Kelvin */
    case GS_NOISE:
      db_val = scaled_val;
      break;

    /* Log-compressed: recover Kelvin from log10(1 + G_lin*T_bright) */
    case GS_NOISE_LOG:
      db_val = pow(10.0, scaled_val) - 1.0;
      break;
  }

  return db_val;
}

/* Draw_Color_Legend_Overlay()
 *
 * Draws a color gradient legend in the bottom right corner
 * showing gain values and corresponding colors
 */
void
Draw_Color_Legend_Overlay( cairo_t *cr )
{
  /* Declare all variables at start */
  int i, x, y;
  double red, grn, blu;
  char txt[16];
  int fstep = calc_data.freq_step;
  int pol = calc_data.pol_type;

  double max_gain, min_gain, color_min;
  double grad_pos, pos, db_val;
  const int width = 20;
  const int height = 200;
  const int margin = 10;
  const int text_width = 80; /* Increased for "dB" suffix */
  const int num_graduations = 5; /* Number of intermediate graduations */

  /* Validate parameters */
  if (!cr || !rad_pattern) return;


  /* Validate frequency step. Note fstep == calc_data.steps_total is valid
   * because an extra index is used the custom frequency when clicking on 
   * the graph at an arbitrary point.
   */
  if (fstep < 0 || fstep > calc_data.steps_total) {
    pr_err("fstep=%d is out of bounds. calc_data.steps_total=%d", fstep, calc_data.steps_total);
    return;
  }

  /* Validate polarization type */
  if (pol < 0 || pol >= NUM_POL) return;

  /* Validate rad_pattern members */
  if (!rad_pattern[fstep].max_gain || !rad_pattern[fstep].min_gain) return;
  
  /* Position in bottom right corner */
  x = rdpattern_proj_params.width - width - text_width - margin;
  y = rdpattern_proj_params.height - height - margin;

  /* Get actual min/max gains and scale them */
  max_gain = rad_pattern[fstep].max_gain[pol];
  min_gain = rad_pattern[fstep].min_gain[pol];
  color_min = (min_gain < COLOR_MIN_GAIN) ? COLOR_MIN_GAIN : min_gain;

  /* Scale the gains for color mapping */
  int noise_legend = IS_NOISE_MODE(rc_config.gain_style);
  double scaled_max, scaled_min, scaled_range;

  if (noise_legend)
  {
    /* Use true noise extremes computed during cell scan */
    scaled_max = rad_pattern[fstep].noise_scaled_max;
    scaled_min = rad_pattern[fstep].noise_scaled_min;
  }
  else
  {
    scaled_max = Scale_Gain(max_gain, fstep, rad_pattern[fstep].max_gain_idx[pol]);
    scaled_min = Scale_Gain(color_min, fstep, rad_pattern[fstep].min_gain_idx[pol]);
  }
  scaled_range = scaled_max - scaled_min;

  /* Calculate relative gain points starting at -3dB and stepping by -6dB */
  int num_rel_marks = 0;
  double rel_gains[10]; // Large enough for all possible marks
  double scaled_vals[10];
  double positions[10];
  
  // Get text height for spacing calculation
  cairo_text_extents_t text_extents;
  cairo_text_extents(cr, "-88 dB", &text_extents);
  double text_height = text_extents.height;
  double min_mark_spacing = text_height * 1.5; // Minimum pixels between marks
  
  // Start with -3dB
  rel_gains[num_rel_marks++] = -3.0;
  
  // Add -6dB steps and -10dB multiples
  double current_gain = -6.0;
  double last_y_pos = 0;
  
  while (current_gain > COLOR_MIN_GAIN) { // Reasonable lower limit
    double gain_val = max_gain + current_gain;
    double scaled_val = Scale_Gain(gain_val, fstep, rad_pattern[fstep].max_gain_idx[pol]);
    double pos = (scaled_val - scaled_min) / scaled_range;
    double y_pos = (1.0 - pos) * (height - 1);
    
    // Only add mark if it has enough spacing from previous mark
    if (num_rel_marks == 1 || (y_pos - last_y_pos) >= min_mark_spacing) {
      rel_gains[num_rel_marks++] = current_gain;
      last_y_pos = y_pos;
    }
    current_gain -= 6.0;
  }

  // Calculate scaled values and positions
  for (i = 0; i < num_rel_marks; i++) {
    double gain_val = max_gain + rel_gains[i];
    scaled_vals[i] = Scale_Gain(gain_val, fstep, rad_pattern[fstep].max_gain_idx[pol]);
    positions[i] = (scaled_vals[i] - scaled_min) / scaled_range;
  }

  /* Draw color gradient bar */
  if (x >= 0 && y >= 0 &&
      x + width <= rdpattern_proj_params.width &&
      y + height <= rdpattern_proj_params.height)
  {
    /* Draw headings */
    int noise_mode = IS_NOISE_MODE(rc_config.gain_style);
    cairo_set_source_rgb(cr, WHITE);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    if (!noise_mode)
    {
      /* Right-justify "Rel." */
      cairo_text_extents_t rel_extents;
      cairo_text_extents(cr, "Rel.", &rel_extents);
      cairo_move_to(cr, x - 5 - rel_extents.width, y - 7);  /* Moved up 2px */
      cairo_show_text(cr, "Rel.");
    }

    /* Left-justify heading: "K" for noise, "Abs." for gain */
    cairo_move_to(cr, x + width + 5, y - 7);  /* Moved up 2px */
    cairo_show_text(cr, noise_mode ? "K/sr" : "Abs.");

    /* Draw color gradient with proper scaling (min at bottom, max at top) */
    for (i = 0; i < height; i++)
    {
      /* Map position in gradient (i=0 is top, i=height-1 is bottom) */
      pos = 1.0 - ((double)i / (double)(height - 1));  // Invert position
      
      /* Get scaled value at this position */
      double scaled_val = scaled_min + (pos * scaled_range);
      
      /* Convert to color */
      Value_to_Color(&red, &grn, &blu, (scaled_val - scaled_min) / scaled_range, 1.0);
      cairo_set_source_rgb(cr, red, grn, blu);
      cairo_rectangle(cr, x, y + i, width, 1);
      cairo_fill(cr);
    }

    /* Draw border around gradient */
    cairo_set_source_rgb(cr, WHITE);
    cairo_set_line_width(cr, LINE_WIDTH);
    cairo_rectangle(cr, x, y-(LINE_WIDTH/2), width, height+2*(LINE_WIDTH/2));
    cairo_stroke(cr);

    /* Draw graduation marks with matching colors and values (min at bottom, max at top) */
    for (i = 0; i < num_graduations; i++)
    {
      /* Calculate scaled value at this graduation (reverse order) */
      grad_pos = (double)(num_graduations - 1 - i) / (double)(num_graduations - 1);
      
      /* Get scaled value at this position */
      double scaled_val = scaled_min + (grad_pos * scaled_range);
      
      /* Convert back to dB for display */
      db_val = Inverse_Scale_Gain(scaled_val);
      
      /* Draw graduation mark with matching color */
      Value_to_Color(&red, &grn, &blu, grad_pos, 1.0);
      cairo_set_source_rgb(cr, red, grn, blu);
      
      int grad_y = y + (i * height) / (num_graduations - 1);
      cairo_move_to(cr, x + width - (LINE_WIDTH/2), grad_y);
      cairo_line_to(cr, x + width + TEXT_GRADIENT_SPACING, grad_y);
      cairo_stroke(cr);

      /* Show value in text */
      if (db_val < -999.99) db_val = -999.99;
      /* First measure width of numeric part */
      char num_txt[16];
      snprintf(num_txt, sizeof(num_txt)-1, "%.2f", db_val);
      cairo_text_extents_t num_extents;
      cairo_text_extents(cr, num_txt, &num_extents);

      /* Format with appropriate units */
      if (noise_mode)
        snprintf(txt, sizeof(txt)-1, "%.0f", db_val);
      else
        snprintf(txt, sizeof(txt)-1, "%.2f dBi", db_val);
      cairo_set_source_rgb(cr, WHITE);
      /* Right-justify absolute gain values with normal weight */
      cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      
      /* Position text so numeric part aligns with gradient */
      cairo_move_to(cr, x + width + TEXT_GRADIENT_SPACING, grad_y + 4);
      cairo_show_text(cr, txt);
    }

    /* Draw relative gain marks on left side (gain modes only) */
    for (i = 0; !noise_mode && i < num_rel_marks; i++) {
      int mark_y = y + (int)((1.0 - positions[i]) * (height - 1));
      
      /* Draw mark */
      Value_to_Color(&red, &grn, &blu, positions[i], 1.0);
      cairo_set_source_rgb(cr, red, grn, blu);
      cairo_move_to(cr, x - TEXT_GRADIENT_SPACING, mark_y);
      cairo_line_to(cr, x, mark_y);
      cairo_stroke(cr);

      /* Show value */
      cairo_set_source_rgb(cr, WHITE);
      if (i == 0) {  /* -3dB point */
        /* Ensure -3dB is bold */
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        snprintf(txt, sizeof(txt)-1, "-3 dB");
      } else {
        /* Other relative gain values in normal weight */
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        snprintf(txt, sizeof(txt)-1, "%d dB", (int)rel_gains[i]);
      }
      /* Right-justify relative gain values */
      cairo_text_extents_t extents;
      cairo_text_extents(cr, txt, &extents);
      cairo_move_to(cr, x - TEXT_GRADIENT_SPACING - extents.width, mark_y + 4);
      cairo_show_text(cr, txt);
    }
  }
}

/* Free_Draw_Buffers()
 *
 * Frees the buffers used for drawing
 */
  void
Free_Draw_Buffers( void )
{
  free_ptr( (void **)&point_3d );
  free_ptr( (void **)&red );
  free_ptr( (void **)&grn );
  free_ptr( (void **)&blu );
}

/*-----------------------------------------------------------------------*/

/* Get_Radiation_Pattern_Data()
 *
 * Returns current radiation pattern data for OpenGL rendering
 */
gboolean
Get_Radiation_Pattern_Data(rdpattern_data_t *data)
{
  int fstep;

  if( !data )
    return( FALSE );

  fstep = calc_data.freq_step;

  if( fstep < 0 || !point_3d || fpat.nth <= 0 || fpat.nph <= 0 )
  {
    data->valid = FALSE;
    return( FALSE );
  }

  data->points = point_3d;
  data->nth = fpat.nth;
  data->nph = fpat.nph;
  data->r_min = cached_r_min;
  data->r_range = cached_r_range;
  data->valid = TRUE;

  return( TRUE );
}

/*-----------------------------------------------------------------------*/

