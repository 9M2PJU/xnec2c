#version 120
uniform float u_alpha;
uniform int flow_mode;
uniform float u_phase;
uniform sampler2D noise_tex;
varying vec4 vertexColor;
varying vec3 viewNormal;
varying vec3 viewPos;
varying vec2 vUV;
varying vec4 vFlowData;

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

/* Flow direction mode constants (match flow_direction_mode_t in opengl_structure.h) */
const int FLOW_DIR_REFERENCE_PHASE = 0;
const int FLOW_DIR_POLARIZATION_TILT = 1;
const int FLOW_DIR_PEAK_MAGNITUDE = 2;
const int FLOW_DIR_LIC = 3;

/* LIC parameters: screen-space integration with 1-pixel steps.
 * 41 samples (±20 steps) produce streaks ~40 pixels long.
 * DLIC_SPEED scales phase offset for flow animation.
 * NOISE_TEX_SIZE must match NOISE_SIZE in opengl_view_scene.c. */
const float NOISE_TEX_SIZE = 256.0;
const int LIC_STEPS = 20;
const float DLIC_SPEED = 0.5;

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
   * GLSL defaults it to (0,0,0,0) and this block is skipped.
   *
   * flow_data contains {Re(ct1), Im(ct1), Re(ct2), Im(ct2)} scaled
   * by mag_ratio.  The shader computes instantaneous direction at
   * u_phase, enabling all visualization modes without CPU recomputation. */
  float mag_sq = dot(vFlowData, vFlowData);
  if (mag_sq > 0.000001) {
    float mag_ratio = sqrt(mag_sq);

    float angle;

    /* Mode 1 (polarization tilt): tilt axis of current ellipse.
     * psi = 0.5 * atan2(2*Re(ct1*conj(ct2)), |ct1|^2 - |ct2|^2) */
    if (flow_mode == FLOW_DIR_POLARIZATION_TILT) {
      float ct1_sq = vFlowData.x * vFlowData.x + vFlowData.y * vFlowData.y;
      float ct2_sq = vFlowData.z * vFlowData.z + vFlowData.w * vFlowData.w;

      /* Re(ct1 * conj(ct2)) = Re1*Re2 + Im1*Im2 */
      float cross = vFlowData.x * vFlowData.z + vFlowData.y * vFlowData.w;
      angle = 0.5 * atan(2.0 * cross, ct1_sq - ct2_sq);
    }
    /* Mode 2 (peak magnitude): phase maximizing |j(alpha)|.
     * P = ct1^2 + ct2^2, alpha0 = -0.5 * arg(P)
     * ct^2 = (Re + j*Im)^2 = Re^2 - Im^2 + j*2*Re*Im */
    else if (flow_mode == FLOW_DIR_PEAK_MAGNITUDE) {
      float p_re = (vFlowData.x * vFlowData.x - vFlowData.y * vFlowData.y)
                 + (vFlowData.z * vFlowData.z - vFlowData.w * vFlowData.w);
      float p_im = 2.0 * (vFlowData.x * vFlowData.y + vFlowData.z * vFlowData.w);
      float alpha0 = -0.5 * atan(p_im, p_re);

      float ca0 = cos(alpha0);
      float sa0 = sin(alpha0);
      float peak_re1 = vFlowData.x * ca0 - vFlowData.y * sa0;
      float peak_re2 = vFlowData.z * ca0 - vFlowData.w * sa0;
      angle = atan(peak_re2, peak_re1);
    }
    /* Modes 0 and 3 (reference phase / LIC): instantaneous direction at u_phase.
     * Re(ct * e^(j*phi)) = Re(ct)*cos(phi) - Im(ct)*sin(phi) */
    else {
      float cp = cos(u_phase);
      float sp = sin(u_phase);
      float re1 = vFlowData.x * cp - vFlowData.y * sp;
      float re2 = vFlowData.z * cp - vFlowData.w * sp;
      angle = atan(re2, re1);
    }

    vec2 centered = vUV - vec2(0.5);
    float ca = cos(angle);
    float sa = sin(angle);

    /* Clockwise pattern rotation to align chevrons with flow direction.
     * Transposed rotation matrix: pattern rotates in same sense as angle. */
    vec2 rotated = vec2(
        centered.x * ca + centered.y * sa,
       -centered.x * sa + centered.y * ca);

    float chevron;
    if (flow_mode == FLOW_DIR_POLARIZATION_TILT) {
      /* Bidirectional: mirrored V-marks for oscillation axis */
      float row_fwd = fract( rotated.x * CHEVRON_FREQ);
      float row_rev = fract(-rotated.x * CHEVRON_FREQ);
      float chev_fwd = abs(rotated.y) * 2.0 + row_fwd;
      float chev_rev = abs(rotated.y) * 2.0 + row_rev;
      chev_fwd = 1.0 - smoothstep(0.45, 0.55, fract(chev_fwd));
      chev_rev = 1.0 - smoothstep(0.45, 0.55, fract(chev_rev));
      chevron = max(chev_fwd, chev_rev);
    }
    else {
      /* Unidirectional: repeating V-shape, 6 chevron marks */
      float row = fract(rotated.x * CHEVRON_FREQ);
      chevron = abs(rotated.y) * 2.0 + row;
      chevron = 1.0 - smoothstep(0.45, 0.55, fract(chevron));
    }

    if (flow_mode == FLOW_DIR_LIC) {
      /* LIC: line integral convolution in screen space.
       * Noise sampled at gl_FragCoord (window pixels) so adjacent
       * patches sharing a screen edge produce continuous streaks.
       *
       * The tangent-plane flow direction (angle) is converted to
       * screen space via the UV-to-screen Jacobian inverse so
       * streak orientation matches the physical current direction. */
      vec2 flow_uv = vec2(cos(angle), sin(angle));

      /* UV-to-screen Jacobian: how UV changes per screen pixel */
      vec2 duv_dx = dFdx(vUV);
      vec2 duv_dy = dFdy(vUV);

      /* Invert 2x2 Jacobian to get screen-space flow direction */
      float det = duv_dx.x * duv_dy.y - duv_dx.y * duv_dy.x;
      vec2 flow_screen;
      if (abs(det) > 1e-8)
      {
        flow_screen = vec2(
            duv_dy.y * flow_uv.x - duv_dx.y * flow_uv.y,
           -duv_dy.x * flow_uv.x + duv_dx.x * flow_uv.y
        ) / det;
        flow_screen = normalize(flow_screen);
      }
      else
      {
        flow_screen = flow_uv;
      }

      /* Step 1 noise texel per iteration in screen-pixel space */
      vec2 noise_uv = gl_FragCoord.xy / NOISE_TEX_SIZE;
      vec2 step = flow_screen / NOISE_TEX_SIZE;
      vec2 phase_offset = flow_screen * u_phase * DLIC_SPEED / 256.0;
      float acc = 0.0;

      for (int k = -LIC_STEPS; k <= LIC_STEPS; k++)
      {
        acc += texture2D(noise_tex,
            noise_uv + phase_offset + step * float(k)).r;
      }

      acc /= float(2 * LIC_STEPS + 1);

      /* Enhance contrast: stretch LIC output around 0.5 so
       * streak-to-gap variation is perceptible on colored surfaces */
      acc = clamp((acc - 0.5) * 3.0 + 0.5, 0.0, 1.0);

      /* Modulate lit color by LIC intensity, scaled by magnitude */
      float lic_strength = mix(0.3, 0.9, mag_ratio);
      color *= mix(1.0 - lic_strength, 1.0 + lic_strength, acc);
    }
    else {
    /* Darken lit color at chevron marks, scaled by magnitude.
     * Floor at MIN so weak currents still show faint chevrons. */
    float contrast = mix(CHEVRON_MIN_CONTRAST,
                         CHEVRON_MAX_CONTRAST, mag_ratio);
    color = mix(color, color * 0.4, chevron * contrast);
    }
  }

  gl_FragColor = vec4(clamp(color, 0.0, 1.0),
                      vertexColor.a * u_alpha);
}
