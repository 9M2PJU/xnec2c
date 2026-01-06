#version 120
attribute vec3 position;
attribute vec2 texcoord;
uniform mat4 mvp;
varying vec2 v_texcoord;

void main() {
  gl_Position = mvp * vec4(position, 1.0);
  v_texcoord = texcoord;
}
