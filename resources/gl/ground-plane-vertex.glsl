#version 120
attribute vec3 position;
attribute vec3 normal;
attribute vec4 color;
uniform mat4 mvp;
varying vec4 vertexColor;
varying vec3 viewNormal;
varying vec3 viewPos;
varying vec3 worldPos;

void main() {
  vec4 pos = mvp * vec4(position, 1.0);
  gl_Position = pos;
  viewPos = pos.xyz / pos.w;
  viewNormal = normalize(mat3(mvp) * normal);
  vertexColor = color;
  worldPos = position;
}
