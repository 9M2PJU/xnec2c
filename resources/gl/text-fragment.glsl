#version 130
uniform sampler2D tex;
centroid in vec2 v_texcoord;
out vec4 fragColor;

void main() {
  vec4 color = texture(tex, v_texcoord);

  /* Discard fully-transparent texels to prevent depth buffer writes;
   * threshold preserves anti-aliased glyph edges */
  if (color.a < 0.001) discard;

  fragColor = color;
}
