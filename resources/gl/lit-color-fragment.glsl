#version 120
uniform float u_alpha;
varying vec4 vertexColor;
varying vec3 viewNormal;
varying vec3 viewPos;

/* Lighting constants */
const vec3 LIGHT_DIR_RAW = vec3(-0.3, 0.5, 1.0);
const float SPECULAR_POWER = 32.0;
const float SPECULAR_INTENSITY = 0.7;
const float RIM_POWER = 2.0;
const float RIM_INTENSITY = 0.5;
const float SHADE_MIN = 0.5;
const float SHADE_RANGE = 0.5;
const float SATURATION_BOOST = 1.6;
const float BRIGHTNESS_BOOST = 1.15;
const vec3 LUMA_WEIGHTS = vec3(0.299, 0.587, 0.114);

void main() {
  vec3 lightDir = normalize(LIGHT_DIR_RAW);
  vec3 norm = normalize(viewNormal);
  vec3 viewDir = normalize(-viewPos);
  vec3 baseColor = vertexColor.rgb;

  /* Diffuse lighting */
  float diff = max(dot(norm, lightDir), 0.0);

  /* Specular highlight */
  vec3 halfDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfDir), 0.0), SPECULAR_POWER);

  /* Rim lighting */
  float rim = 1.0 - max(dot(viewDir, norm), 0.0);
  rim = pow(rim, RIM_POWER) * RIM_INTENSITY;

  /* Shading - never below SHADE_MIN to preserve vibrancy */
  float shade = SHADE_MIN + diff * SHADE_RANGE;
  vec3 color = baseColor * shade;

  /* Saturation boost */
  float luma = dot(color, LUMA_WEIGHTS);
  color = mix(vec3(luma), color, SATURATION_BOOST);

  /* Add specular and rim */
  color += vec3(1.0) * spec * SPECULAR_INTENSITY;
  color += baseColor * rim;

  /* Final brightness boost */
  color *= BRIGHTNESS_BOOST;

  gl_FragColor = vec4(clamp(color, 0.0, 1.0), vertexColor.a * u_alpha);
}
