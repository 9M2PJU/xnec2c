#version 120
uniform float u_alpha;
varying vec4 vertexColor;
varying vec3 viewNormal;
varying vec3 viewPos;
varying vec2 vUV;
varying vec2 vFlowData;

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

/* Chevron parameters */
const float CHEVRON_FREQ = 6.0;
const float CHEVRON_MIN_CONTRAST = 0.12;
const float CHEVRON_MAX_CONTRAST = 0.55;

void main() {
  vec3 lightDir = normalize(LIGHT_DIR_RAW);
  vec3 norm = normalize(viewNormal);
  vec3 viewDir = normalize(-viewPos);
  vec3 baseColor = vertexColor.rgb;

  /* Two-sided lighting: flip normal for back-facing fragments so
   * thin surfaces (patches) illuminate identically from both sides.
   * For closed surfaces (cylinders), back faces are depth-occluded
   * by front faces, so the flip has no visual effect. */
  if (dot(norm, viewDir) < 0.0)
    norm = -norm;

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

  /* Chevron pattern applied AFTER lighting so that contrast is
   * independent of viewing angle.  When flow_data is unbound
   * (e.g. lit_color_point_t vertices with 3-attrib config),
   * GLSL defaults it to (0,0) and this block is skipped. */
  float mag_ratio = vFlowData.y;
  if (mag_ratio > 0.001) {
    float angle = vFlowData.x;
    vec2 centered = vUV - vec2(0.5);
    float ca = cos(angle);
    float sa = sin(angle);
    vec2 rotated = vec2(
        centered.x * ca - centered.y * sa,
        centered.x * sa + centered.y * ca);

    /* Repeating V-shape: 6 chevron marks across the patch */
    float row = fract(rotated.x * CHEVRON_FREQ);
    float chevron = abs(rotated.y) * 2.0 + row;
    chevron = 1.0 - smoothstep(0.45, 0.55, fract(chevron));

    /* Darken lit color at chevron marks, scaled by magnitude.
     * Floor at MIN so weak currents still show faint chevrons. */
    float contrast = mix(CHEVRON_MIN_CONTRAST,
                         CHEVRON_MAX_CONTRAST, mag_ratio);
    color = mix(color, color * 0.4, chevron * contrast);
  }

  gl_FragColor = vec4(clamp(color, 0.0, 1.0),
                      vertexColor.a * u_alpha);
}
