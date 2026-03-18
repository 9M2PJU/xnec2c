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

#include "opengl_view_msaa.h"
#include "../shared.h"

#ifdef HAVE_OPENGL

/*-----------------------------------------------------------------------*/

/** gl_view_msaa_free() - Release MSAA framebuffer, color, and depth renderbuffer resources
 * @state: view state containing MSAA resources to free
 */
  void
gl_view_msaa_free(gl_view_state_t *state)
{
  if( !state )
    return;

  if( state->msaa_fbo )
  {
    glDeleteFramebuffers(1, &state->msaa_fbo);
    state->msaa_fbo = 0;
  }

  if( state->msaa_color_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_color_rbo);
    state->msaa_color_rbo = 0;
  }

  if( state->msaa_depth_rbo )
  {
    glDeleteRenderbuffers(1, &state->msaa_depth_rbo);
    state->msaa_depth_rbo = 0;
  }

  state->msaa_samples = 0;

} /* gl_view_msaa_free() */

/*-----------------------------------------------------------------------*/

/** gl_view_recreate_msaa() - Recreate MSAA resources without destroying GL widget
 * @state: view state to update with new MSAA resources
 * @requested_samples: number of MSAA samples to use (clamped to hardware limit)
 */
  void
gl_view_recreate_msaa(gl_view_state_t *state, int requested_samples)
{
  GLint max_samples;
  int width, height;

  if( !state || !state->initialized )
    return;

  /* Release existing MSAA resources */
  gl_view_msaa_free(state);

  if( requested_samples == 0 )
    return;

  /* Query hardware limit and clamp */
  glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
  state->msaa_samples = (requested_samples > max_samples) ? max_samples : requested_samples;

  if( state->msaa_samples < 2 )
  {
    state->msaa_samples = 0;
    return;
  }

  /* Get current viewport dimensions */
  width = state->msaa_width;
  height = state->msaa_height;

  if( width == 0 || height == 0 )
    return;

  /* Create multisampled color renderbuffer */
  glGenRenderbuffers(1, &state->msaa_color_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, state->msaa_color_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, state->msaa_samples,
      GL_RGBA8, width, height);

  /* Create multisampled depth renderbuffer */
  glGenRenderbuffers(1, &state->msaa_depth_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, state->msaa_depth_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, state->msaa_samples,
      GL_DEPTH_COMPONENT24, width, height);

  /* Create FBO and attach renderbuffers */
  glGenFramebuffers(1, &state->msaa_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, state->msaa_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER, state->msaa_color_rbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
      GL_RENDERBUFFER, state->msaa_depth_rbo);

  if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
  {
    pr_err("MSAA framebuffer incomplete\n");
    gl_view_msaa_free(state);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

} /* gl_view_recreate_msaa() */

/*-----------------------------------------------------------------------*/

#endif /* HAVE_OPENGL */
