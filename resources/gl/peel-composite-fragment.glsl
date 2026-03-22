#version 130
uniform sampler2D u_layer;
noperspective in vec2 vUV;
out vec4 fragColor;

void main() {
  /* Mode 0: under-accumulate — output layer color for GL blend unit.
   *   Blend state: glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE)
   *   Computes: dst = src * (1 - dst.a) + dst
   *
   * Mode 1: final composite — output accumulated transparent color.
   *   Blend state: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
   *   Premultiplied alpha over opaque background. */
  fragColor = texture(u_layer, vUV);
}
