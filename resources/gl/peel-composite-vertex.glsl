#version 120
/* Fullscreen triangle via VBO-supplied positions.
 * Three vertices at (-1,-1), (3,-1), (-1,3) cover the screen. */
attribute vec2 position;
varying vec2 vUV;

void main() {
  vUV = position * 0.5 + 0.5;
  gl_Position = vec4(position, 0.0, 1.0);
}
