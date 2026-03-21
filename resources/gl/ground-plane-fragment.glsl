#version 120
uniform float u_alpha;
uniform sampler2D u_peel_depth;
uniform vec2 u_viewport_size;
uniform int u_peel_pass;

/* Depth-peel epsilon bias — must match lit-color-fragment.glsl */
const float PEEL_DEPTH_EPSILON = 0.00001;
varying vec4 vertexColor;
varying vec3 viewNormal;
varying vec3 viewPos;
varying vec3 worldPos;

/* Lighting constants */
const vec3 LIGHT_DIR_RAW = vec3(-0.3, 0.5, 1.0);
const float SPECULAR_POWER = 32.0;
const float SPECULAR_INTENSITY = 0.3;
const float SHADE_MIN = 0.6;
const float SHADE_RANGE = 0.4;

/* Checkerboard pattern constants */
const float TILE_SIZE = 0.5;
const vec3 DARK_GREEN = vec3(0.15, 0.25, 0.12);
const vec3 LIGHT_GREEN = vec3(0.20, 0.32, 0.15);

void main() {
  /* Depth-peel discard: for passes > 0, reject fragments at or
   * nearer than the previous layer's depth.  Derivative-based
   * epsilon covers both 24-bit quantization and MSAA resolve
   * offset (see lit-color-fragment.glsl for details). */
  if (u_peel_pass > 0) {
    float prev_z = texture2D(u_peel_depth,
        gl_FragCoord.xy / u_viewport_size).r;
    float dz = max(abs(dFdx(gl_FragCoord.z)),
                   abs(dFdy(gl_FragCoord.z)));
    float eps = max(PEEL_DEPTH_EPSILON, dz);
    if (gl_FragCoord.z <= prev_z + eps) discard;
  }

  vec3 lightDir = normalize(LIGHT_DIR_RAW);
  vec3 norm = normalize(viewNormal);
  vec3 viewDir = normalize(-viewPos);

  /* Compute checkerboard pattern from world position */
  float tx = floor(worldPos.x / TILE_SIZE);
  float ty = floor(worldPos.y / TILE_SIZE);
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
  color += vec3(1.0) * spec * SPECULAR_INTENSITY;

  gl_FragColor = vec4(clamp(color, 0.0, 1.0), u_alpha);
}
