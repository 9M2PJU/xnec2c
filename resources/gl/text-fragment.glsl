#version 120
uniform sampler2D tex;
varying vec2 v_texcoord;

void main() {
  vec4 color = texture2D(tex, v_texcoord);

  /* Discard fully-transparent texels to prevent depth buffer writes;
   * threshold preserves anti-aliased glyph edges */
  if (color.a < 0.001) discard;

  gl_FragColor = color;
}
