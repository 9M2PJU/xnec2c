#version 130
uniform float u_alpha;
uniform float u_color_dim;
centroid in vec4 vertexColor;
out vec4 fragColor;

void main() {
  /* Default to full brightness when u_color_dim is unset (GL defaults to 0.0) */
  float dim = (u_color_dim > 0.0) ? u_color_dim : 1.0;
  fragColor = vec4(vertexColor.rgb * dim, vertexColor.a * u_alpha);
}
