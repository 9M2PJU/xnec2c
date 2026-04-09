#version 150
uniform float u_alpha;
uniform float u_color_dim;
centroid in vec4 vertexColor;
out vec4 fragColor;

void main() {
  fragColor = vec4(vertexColor.rgb * u_color_dim, vertexColor.a * u_alpha);
}
