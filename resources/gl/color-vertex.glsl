#version 130
in vec3 position;
in vec4 color;
uniform mat4 mvp;
out vec4 vertexColor;

void main() {
  gl_Position = mvp * vec4(position, 1.0);
  vertexColor = color;
}
