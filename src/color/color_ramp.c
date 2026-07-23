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
 * color_ramp: legacy magenta-blue-cyan-green-yellow-red rainbow ramp.
 *
 * The single point of truth for the historical Value_to_Color sweep; the
 * theme palette LUTs reproduce this ramp for the amplitude projection.
 */
#include "color_ramp.h"

/*-----------------------------------------------------------------------*/

/**
 * Value_to_Color() - Map a scalar to an RGB rainbow ramp
 * @red: output red channel [0..1]
 * @grn: output green channel [0..1]
 * @blu: output blue channel [0..1]
 * @val: input value
 * @max: normalization maximum
 *
 * Produces magenta->blue->cyan->green->yellow->red for val in [0..max].
 */
  void
Value_to_Color(double *red, double *grn, double *blu, double val, double max)
{
  int ival;

  /* Scale val so that normalized ival is 0-1279 */
  ival = (int)(1279.0 * val / max);

  switch( ival / 256 )
  {
    case 0: /* 0-255 : magenta to blue */
      *red = 255.0 - (double)ival;
      *grn = 0.0;
      *blu = 255.0;
      break;

    case 1: /* 256-511 : blue to cyan */
      *red = 0.0;
      *grn = (double)ival - 256.0;
      *blu = 255.0;
      break;

    case 2: /* 512-767 : cyan to green */
      *red = 0.0;
      *grn = 255.0;
      *blu = 767.0 - (double)ival;
      break;

    case 3: /* 768-1023 : green to yellow */
      *red = (double)ival - 768.0;
      *grn = 255.0;
      *blu = 0.0;
      break;

    case 4: /* 1024-1279 : yellow to red */
      *red = 255.0;
      *grn = 1279.0 - (double)ival;
      *blu = 0.0;
      break;

    default: /* Clamp out-of-range to endpoints */
      if( ival < 0 )
      {
        *red = 255.0;
        *grn = 0.0;
        *blu = 255.0;
      }
      else
      {
        *red = 255.0;
        *grn = 0.0;
        *blu = 0.0;
      }
      break;
  }

  /* Scale channels to [0..1] */
  *red /= 255.0;
  *grn /= 255.0;
  *blu /= 255.0;
}

/*-----------------------------------------------------------------------*/
