
/**
 * Denoise raytrace result using ratio estimator.
 *
 * Following "Stochastic All The Things: Raytracing in Hybrid Real-Time Rendering"
 * by Tomasz Stachowiak
 * https://www.ea.com/seed/news/seed-dd18-presentation-slides-raytracing
 */

#pragma BLENDER_REQUIRE(common_math_lib.glsl)
#pragma BLENDER_REQUIRE(common_view_lib.glsl)
#pragma BLENDER_REQUIRE(eevee_bsdf_microfacet_lib.glsl)
#pragma BLENDER_REQUIRE(eevee_gbuffer_lib.glsl)
#pragma BLENDER_REQUIRE(eevee_raytrace_resolve_lib.glsl)
#pragma BLENDER_REQUIRE(eevee_sampling_lib.glsl)
#pragma BLENDER_REQUIRE(eevee_shader_shared.hh)

layout(std140) uniform raytrace_block
{
  RaytraceData raytrace;
};

layout(std140) uniform hiz_block
{
  HiZData hiz;
};

uniform sampler2D ray_data_tx;
uniform sampler2D ray_radiance_tx;
#ifdef REFRACTION
uniform sampler2D transmit_color_tx;
uniform sampler2D transmit_normal_tx;
uniform sampler2D transmit_data_tx;
#else
uniform sampler2D reflect_color_tx;
uniform sampler2D reflect_normal_tx;
#endif

in vec4 uvcoordsvar;

layout(location = 0) out vec4 out_combined;
layout(location = 2) out vec3 out_specular;

void main(void)
{
  vec2 uv = uvcoordsvar.xy;
  vec3 vP = get_view_space_from_depth(uv, 0.5);
  vec3 vV = viewCameraVec(vP);

  /* Blue noise categorised into 4 sets of samples.
   * See "Stochastic all the things" presentation slide 32-37. */
  int sample_pool = int((uint(gl_FragCoord.x) & 1u) + (uint(gl_FragCoord.y) & 1u) * 2u);
  sample_pool = (sample_pool + raytrace.pool_offset) % 4;
  int sample_id = sample_pool * resolve_sample_max;
  int sample_count = resolve_sample_max;

#ifdef REFRACTION
  vec4 tra_col_in = texture(transmit_color_tx, uv);
  vec4 tra_nor_in = texture(transmit_normal_tx, uv);
  vec4 tra_dat_in = texture(transmit_data_tx, uv);

  ClosureRefraction refraction = gbuffer_load_refraction_data(tra_col_in, tra_nor_in, tra_dat_in);

  float thickness;
  gbuffer_load_global_data(tra_nor_in, thickness);

  vec3 vN = transform_direction(ViewMatrix, refraction.N);
  float roughness_sqr = max(1e-3, sqr(refraction.roughness));

  /* Unfortunately Refraction ray reuse does not work great because the assumption that. */
  sample_count = 1;
#else
  ClosureReflection reflection = gbuffer_load_reflection_data(
      reflect_color_tx, reflect_normal_tx, uv);

  vec3 vN = transform_direction(ViewMatrix, reflection.N);
  float roughness_sqr = max(1e-3, sqr(reflection.roughness));

  if (roughness_sqr == 1e-3) {
    sample_count = 1;
  }
#endif

  vec3 radiance_accum = vec3(0.0);
  float weight_accum = 0.0;
  for (int i = 0; i < sample_count; i++, sample_id++) {
    vec2 sample_uv = uv + resolve_sample_offsets[sample_id] * hiz.pixel_to_ndc * 0.5;

    vec4 ray_data = texture(ray_data_tx, sample_uv);
    vec4 ray_radiance = texture(ray_radiance_tx, sample_uv);

    vec3 vR = normalize(ray_data.xyz);
    float ray_pdf_inv = ray_data.w;
    /* Skip invalid pixels. */
    if (ray_pdf_inv == 0.0) {
      continue;
    }

    /* Slide 54. */
#ifdef REFRACTION
    float bsdf = btdf_ggx(vN, vR, vV, roughness_sqr, refraction.ior);
#else
    float bsdf = bsdf_ggx(vN, vR, vV, roughness_sqr);
#endif
    float weight = bsdf * ray_pdf_inv;

    radiance_accum += ray_radiance.rgb * weight;
    weight_accum += weight;
  }
  radiance_accum *= safe_rcp(weight_accum);
#ifdef REFRACTION
  radiance_accum *= refraction.color;
  /* Transmit twice if thickness is set. */
  if (thickness > 0.0) {
    radiance_accum *= refraction.color;
  }
#else
  radiance_accum *= reflection.color;
#endif

  out_combined = vec4(radiance_accum, 0.0);
  out_specular = out_combined.rgb;
}