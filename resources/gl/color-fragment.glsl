#version 120
uniform float u_alpha;
varying vec4 vertexColor;

void main() {
  gl_FragColor = vec4(vertexColor.rgb, vertexColor.a * u_alpha);
}
