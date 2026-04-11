#version 150
in vec3 position;
in vec3 normal;
in vec4 color;
in vec2 uv;
in vec4 flow_data;
in vec3 tangent1;
in vec3 tangent2;
uniform mat4 mvp;
uniform mat4 u_mv;
uniform float u_cos_phase;
uniform float u_sin_phase;
centroid out vec4 vertexColor;
centroid out vec3 viewNormal;
centroid out vec3 viewPos;
centroid out vec2 vUV;
centroid out vec4 vFlowData;

void main() {
  vec3 world_pos = position;
  vec2 out_uv = uv;
  vec4 out_flow = flow_data;

  /* GPU-driven arrow rotation: when tangent frame is non-zero,
   * position holds the patch center and uv holds the arrow
   * template coordinate.  Rotate the template UV by the
   * phase-derived flow angle, then transform to world space
   * via the tangent frame. */
  if (dot(tangent1, tangent1) > 0.0)
  {
    /* Instantaneous flow angle at current phase:
     * Re(ct * e^(j*phi)) = Re(ct)*cos(phi) - Im(ct)*sin(phi)
     * Parity: identical formula to fragment shader default case. */
    float re1 = flow_data.x * u_cos_phase - flow_data.y * u_sin_phase;
    float re2 = flow_data.z * u_cos_phase - flow_data.w * u_sin_phase;
    float angle = atan(re2, re1);

    float ca = cos(angle);
    float sa = sin(angle);

    /* Rotate template UV around (0.5, 0.5) center */
    vec2 centered = uv - vec2(0.5);
    vec2 rotated = vec2(
        centered.x * ca - centered.y * sa,
        centered.x * sa + centered.y * ca);
    rotated += vec2(0.5);

    /* Transform rotated UV to world space via tangent frame:
     * world = center + (2u-1)*tangent1 + (2v-1)*tangent2 */
    world_pos = position
      + (2.0 * rotated.x - 1.0) * tangent1
      + (2.0 * rotated.y - 1.0) * tangent2;

    /* Pass zero UV/flow to fragment shader — arrow geometry
     * is the visualization; no chevron pattern needed */
    out_uv = vec2(0.0);
    out_flow = vec4(0.0);
  }

  gl_Position = mvp * vec4(world_pos, 1.0);

  viewPos = (u_mv * vec4(world_pos, 1.0)).xyz;
  viewNormal = normalize(mat3(u_mv) * normal);
  vertexColor = color;
  vUV = out_uv;
  vFlowData = out_flow;
}
