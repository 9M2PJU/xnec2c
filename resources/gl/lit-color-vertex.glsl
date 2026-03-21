#version 120
attribute vec3 position;
attribute vec3 normal;
attribute vec4 color;
attribute vec2 uv;
attribute vec4 flow_data;
attribute float depth_bias;
uniform mat4 mvp;
uniform mat4 u_mv;
varying vec4 vertexColor;
varying vec3 viewNormal;
varying vec3 viewPos;
varying vec2 vUV;
varying vec4 vFlowData;

void main() {
  gl_Position = mvp * vec4(position, 1.0);

  /* Clip-space depth offset for coplanar z-fighting resolution.
   * Pulls biased patches toward the camera so distant geometry
   * is never pushed past the far clip plane.
   * Multiplying by w keeps the bias constant in NDC. */
  gl_Position.z -= depth_bias * gl_Position.w;

  viewPos = (u_mv * vec4(position, 1.0)).xyz;
  viewNormal = normalize(mat3(u_mv) * normal);
  vertexColor = color;
  vUV = uv;
  vFlowData = flow_data;
}
