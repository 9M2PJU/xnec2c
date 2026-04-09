#version 150
uniform float u_alpha;
uniform float u_color_dim;
uniform sampler2D u_peel_depth;
uniform int u_peel_pass;

/* Depth-peel epsilon bias — must match lit-color-fragment.glsl */
const float PEEL_DEPTH_EPSILON = 0.00001;
centroid in vec4 vertexColor;
centroid in vec3 viewNormal;
centroid in vec3 viewPos;
centroid in vec3 worldPos;
out vec4 fragColor;

/* Lighting constants — normalize(vec3(-0.3, 0.5, 1.0)) precomputed */
const vec3 LIGHT_DIR = vec3(-0.2592106738356498, 0.4320177897260830, 0.8640355794521660);
const float SPECULAR_POWER = 32.0;
const float SPECULAR_INTENSITY = 0.3;
const float SHADE_MIN = 0.6;
const float SHADE_RANGE = 0.4;

/* Checkerboard pattern constants */
const float TILE_SIZE = 0.5;
const float INV_TILE_SIZE = 1.0 / TILE_SIZE;
const vec3 DARK_GREEN = vec3(0.15, 0.25, 0.12);
const vec3 LIGHT_GREEN = vec3(0.20, 0.32, 0.15);

void main() {
  /* Depth-peel discard: for passes > 0, reject fragments at or
   * nearer than the previous layer's depth.  Derivative-based
   * epsilon covers both 24-bit quantization and MSAA resolve
   * offset (see lit-color-fragment.glsl for details). */
  if (u_peel_pass > 0) {
    float prev_z = texelFetch(u_peel_depth,
        ivec2(gl_FragCoord.xy), 0).r;
    float dz = max(abs(dFdx(gl_FragCoord.z)),
                   abs(dFdy(gl_FragCoord.z)));
    float eps = max(PEEL_DEPTH_EPSILON, dz);
    if (gl_FragCoord.z <= prev_z + eps) discard;
  }

  vec3 lightDir = LIGHT_DIR;
  vec3 norm = normalize(viewNormal);
  vec3 viewDir = normalize(-viewPos);

  /* Compute checkerboard pattern from world position */
  float tx = floor(worldPos.x * INV_TILE_SIZE);
  float ty = floor(worldPos.y * INV_TILE_SIZE);
  float checker = mod(tx + ty, 2.0);

  vec3 baseColor = mix(DARK_GREEN, LIGHT_GREEN, checker);

  /* Diffuse lighting */
  float diff = max(dot(norm, lightDir), 0.0);

  /* Specular highlight */
  vec3 halfDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfDir), 0.0), SPECULAR_POWER);

  /* Shading */
  float shade = SHADE_MIN + diff * SHADE_RANGE;
  vec3 color = baseColor * shade;

  /* Add subtle specular */
  color += vec3(spec * SPECULAR_INTENSITY);

  fragColor = vec4(clamp(color * u_color_dim, 0.0, 1.0), u_alpha);
}
