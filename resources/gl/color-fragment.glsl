#version 130
uniform float u_alpha;
in vec4 vertexColor;
out vec4 fragColor;

void main() {
  fragColor = vec4(vertexColor.rgb, vertexColor.a * u_alpha);
}
