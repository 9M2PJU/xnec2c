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

#ifndef OPENGL_MATH_H
#define OPENGL_MATH_H 1

#include <math.h>
#include <string.h>

/* Default epsilon for floating-point comparisons */
#define GLM_EPSILON 1e-6f

/*
 * Minimal GL math types and functions replacing the cglm dependency.
 * All matrices are column-major to match OpenGL's glUniformMatrix4fv
 * convention: mat4[col][row].  Memory layout is identical to cglm.
 */

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];
typedef vec4  mat4[4];  /* mat4[col][row] — column-major for GL */


/* ------------------------------------------------------------------ */
/* Scalar helpers                                                       */
/* ------------------------------------------------------------------ */

/**
 * glm_rad() - Convert degrees to radians
 * @deg: angle in degrees
 *
 * Return: angle in radians
 */
static inline float
glm_rad(float deg)
{
  return( deg * ((float)M_PI / 180.0f) );
}

/**
 * glm_deg() - Convert radians to degrees
 * @rad: angle in radians
 *
 * Return: angle in degrees
 */
static inline float
glm_deg(float rad)
{
  return( rad * (180.0f / (float)M_PI) );
}

/**
 * glm_clamp() - Clamp a float value to [lo, hi]
 * @val: value to clamp
 * @lo:  lower bound
 * @hi:  upper bound
 *
 * Return: clamped value
 */
static inline float
glm_clamp(float val, float lo, float hi)
{
  float result;

  if( val < lo )
    result = lo;
  else if( val > hi )
    result = hi;
  else
    result = val;

  return( result );
}


/* ------------------------------------------------------------------ */
/* Vector helpers                                                       */
/* ------------------------------------------------------------------ */

/**
 * glm_vec2_zero() - Set all components of a vec2 to zero
 * @v: destination vector
 */
static inline void
glm_vec2_zero(vec2 v)
{
  v[0] = 0.0f;
  v[1] = 0.0f;
}

/**
 * glm_vec3_zero() - Set all components of a vec3 to zero
 * @v: destination vector
 */
static inline void
glm_vec3_zero(vec3 v)
{
  v[0] = 0.0f;
  v[1] = 0.0f;
  v[2] = 0.0f;
}

/**
 * glm_vec3_copy() - Copy a vec3
 * @src:  source vector
 * @dest: destination vector
 */
static inline void
glm_vec3_copy(vec3 src, vec3 dest)
{
  memcpy(dest, src, sizeof(vec3));
}


/* ------------------------------------------------------------------ */
/* mat4 helpers                                                         */
/* ------------------------------------------------------------------ */

/**
 * glm_mat4_identity() - Load a 4x4 identity matrix
 * @m: matrix to initialize
 */
static inline void
glm_mat4_identity(mat4 m)
{
  memset(m, 0, sizeof(mat4));
  m[0][0] = 1.0f;
  m[1][1] = 1.0f;
  m[2][2] = 1.0f;
  m[3][3] = 1.0f;
}

/**
 * glm_mat4_copy() - Copy a mat4
 * @src:  source matrix
 * @dest: destination matrix
 */
static inline void
glm_mat4_copy(mat4 src, mat4 dest)
{
  memcpy(dest, src, sizeof(mat4));
}

/**
 * glm_mat4_mul() - Multiply two 4x4 matrices: dest = a * b
 * @a:    left operand
 * @b:    right operand
 * @dest: result; aliasing with a or b is safe (uses internal temp)
 *
 * Matrices are column-major: a[col][row].
 */
static inline void
glm_mat4_mul(mat4 a, mat4 b, mat4 dest)
{
  mat4 tmp;
  int  col, row, k;

  for( col = 0; col < 4; col++ )
  {
    for( row = 0; row < 4; row++ )
    {
      tmp[col][row] = 0.0f;
      for( k = 0; k < 4; k++ )
        tmp[col][row] += a[k][row] * b[col][k];
    }
  }

  memcpy(dest, tmp, sizeof(mat4));
}


/* ------------------------------------------------------------------ */
/* Affine transforms                                                    */
/* ------------------------------------------------------------------ */

/**
 * glm_rotate() - Rotate matrix m by angle around axis (in-place: m = m * R)
 * @m:     matrix to rotate (modified in place)
 * @angle: rotation angle in radians
 * @axis:  rotation axis (need not be normalized)
 *
 * Builds the Rodrigues rotation matrix R and applies m = m * R.
 */
static inline void
glm_rotate(mat4 m, float angle, vec3 axis)
{
  float  len, x, y, z, c, s, t;
  mat4   rot;

  /* normalize axis */
  len = sqrtf(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
  if( len < GLM_EPSILON )
    return;
  x = axis[0] / len;
  y = axis[1] / len;
  z = axis[2] / len;

  c = cosf(angle);
  s = sinf(angle);
  t = 1.0f - c;

  /* column-major Rodrigues rotation matrix */
  rot[0][0] = c + x*x*t;
  rot[0][1] = y*x*t + z*s;
  rot[0][2] = z*x*t - y*s;
  rot[0][3] = 0.0f;

  rot[1][0] = x*y*t - z*s;
  rot[1][1] = c + y*y*t;
  rot[1][2] = z*y*t + x*s;
  rot[1][3] = 0.0f;

  rot[2][0] = x*z*t + y*s;
  rot[2][1] = y*z*t - x*s;
  rot[2][2] = c + z*z*t;
  rot[2][3] = 0.0f;

  rot[3][0] = 0.0f;
  rot[3][1] = 0.0f;
  rot[3][2] = 0.0f;
  rot[3][3] = 1.0f;

  glm_mat4_mul(m, rot, m);
}

/**
 * glm_scale() - Scale columns 0-2 of m by components of v (in-place)
 * @m: matrix to scale (modified in place)
 * @v: per-axis scale factors: v[0]=X, v[1]=Y, v[2]=Z
 */
static inline void
glm_scale(mat4 m, vec3 v)
{
  int row;

  for( row = 0; row < 4; row++ )
  {
    m[0][row] *= v[0];
    m[1][row] *= v[1];
    m[2][row] *= v[2];
  }
}

/**
 * glm_translate() - Apply translation v to matrix m (in-place: m[3] += m*v)
 * @m: matrix to translate (modified in place)
 * @v: translation vector
 */
static inline void
glm_translate(mat4 m, vec3 v)
{
  int row;

  for( row = 0; row < 4; row++ )
    m[3][row] += m[0][row]*v[0] + m[1][row]*v[1] + m[2][row]*v[2];
}


/* ------------------------------------------------------------------ */
/* Camera / projection                                                  */
/* ------------------------------------------------------------------ */

/**
 * glm_lookat() - Build a right-handed look-at view matrix
 * @eye:    camera position
 * @center: point the camera looks at
 * @up:     world up vector
 * @dest:   output view matrix
 */
static inline void
glm_lookat(vec3 eye, vec3 center, vec3 up, mat4 dest)
{
  float fx, fy, fz, flen;
  float sx, sy, sz, slen;
  float ux, uy, uz;

  /* forward = normalize(center - eye) */
  fx = center[0] - eye[0];
  fy = center[1] - eye[1];
  fz = center[2] - eye[2];
  flen = sqrtf(fx*fx + fy*fy + fz*fz);
  fx /= flen;  fy /= flen;  fz /= flen;

  /* right = normalize(cross(forward, up)) */
  sx = fy*up[2] - fz*up[1];
  sy = fz*up[0] - fx*up[2];
  sz = fx*up[1] - fy*up[0];
  slen = sqrtf(sx*sx + sy*sy + sz*sz);
  sx /= slen;  sy /= slen;  sz /= slen;

  /* up_corrected = cross(right, forward) — already unit-length */
  ux = sy*fz - sz*fy;
  uy = sz*fx - sx*fz;
  uz = sx*fy - sy*fx;

  /* column-major view matrix */
  dest[0][0] =  sx;  dest[0][1] =  ux;  dest[0][2] = -fx;  dest[0][3] = 0.0f;
  dest[1][0] =  sy;  dest[1][1] =  uy;  dest[1][2] = -fy;  dest[1][3] = 0.0f;
  dest[2][0] =  sz;  dest[2][1] =  uz;  dest[2][2] = -fz;  dest[2][3] = 0.0f;
  dest[3][0] = -(sx*eye[0] + sy*eye[1] + sz*eye[2]);
  dest[3][1] = -(ux*eye[0] + uy*eye[1] + uz*eye[2]);
  dest[3][2] =   fx*eye[0] + fy*eye[1] + fz*eye[2];
  dest[3][3] = 1.0f;
}

/**
 * glm_perspective() - Build a right-handed perspective projection (NDC depth -1..1)
 * @fov:    vertical field of view in radians
 * @aspect: viewport width / height
 * @near:   near clip plane distance (positive)
 * @far:    far clip plane distance (positive)
 * @dest:   output projection matrix
 */
static inline void
glm_perspective(float fov, float aspect, float near, float far, mat4 dest)
{
  float f;

  memset(dest, 0, sizeof(mat4));

  f = 1.0f / tanf(fov * 0.5f);

  dest[0][0] =  f / aspect;
  dest[1][1] =  f;
  dest[2][2] = -(far + near) / (far - near);
  dest[2][3] = -1.0f;
  dest[3][2] = -(2.0f * far * near) / (far - near);
}

/**
 * glm_ortho() - Build a right-handed orthographic projection (NDC depth -1..1)
 * @left:   left clip plane
 * @right:  right clip plane
 * @bottom: bottom clip plane
 * @top:    top clip plane
 * @near:   near clip plane
 * @far:    far clip plane
 * @dest:   output projection matrix
 */
static inline void
glm_ortho(float left, float right, float bottom, float top,
          float near, float far, mat4 dest)
{
  memset(dest, 0, sizeof(mat4));

  dest[0][0] =  2.0f / (right - left);
  dest[1][1] =  2.0f / (top - bottom);
  dest[2][2] = -2.0f / (far - near);
  dest[3][0] = -(right + left)  / (right - left);
  dest[3][1] = -(top   + bottom) / (top   - bottom);
  dest[3][2] = -(far   + near)  / (far   - near);
  dest[3][3] =  1.0f;
}

#endif /* OPENGL_MATH_H */
