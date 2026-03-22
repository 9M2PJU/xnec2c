#version 130
in vec3 position;
in vec3 normal;
in vec4 color;
in vec2 uv;
in vec4 flow_data;
in float depth_bias;
uniform mat4 mvp;
uniform mat4 u_mv;
centroid out vec4 vertexColor;
centroid out vec3 viewNormal;
centroid out vec3 viewPos;
centroid out vec2 vUV;
centroid out vec4 vFlowData;
flat out float vDepthBias;

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
