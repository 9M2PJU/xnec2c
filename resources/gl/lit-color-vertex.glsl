#version 130
in vec3 position;
in vec3 normal;
in vec4 color;
in vec2 uv;
in vec4 flow_data;
in float depth_bias;
uniform mat4 mvp;
uniform mat4 u_mv;
out vec4 vertexColor;
out vec3 viewNormal;
out vec3 viewPos;
out vec2 vUV;
out vec4 vFlowData;
out float vDepthBias;

void main() {
  gl_Position = mvp * vec4(position, 1.0);

  /* Pass per-vertex depth bias to fragment shader for gl_FragDepth.
   * Wires carry 0; patches carry per-index positive bias. */
  vDepthBias = depth_bias;

  viewPos = (u_mv * vec4(position, 1.0)).xyz;
  viewNormal = normalize(mat3(u_mv) * normal);
  vertexColor = color;
  vUV = uv;
  vFlowData = flow_data;
}
