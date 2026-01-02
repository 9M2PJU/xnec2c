#version 120
attribute vec3 position;
attribute vec4 color;
uniform mat4 mvp;
varying vec4 vertexColor;

void main() {
  gl_Position = mvp * vec4(position, 1.0);
  vertexColor = color;
}
