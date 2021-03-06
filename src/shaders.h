#pragma once

namespace mf_Shaders
{

extern const char* const text_shader_v;
extern const char* const text_shader_f;

extern const char* const naviball_icon_shader_v;
extern const char* const naviball_icon_shader_f;

extern const char* const gui_shader_v;
extern const char* const gui_shader_f;
extern const char* const machinegun_circle_shader_v;
extern const char* const machinegun_circle_shader_f;

extern const char* const terrain_shader_v;
extern const char* const terrain_shader_f;

extern const char* const terrain_shadowmap_shader_v;

extern const char* const water_shader_v;
extern const char* const water_shader_f;

//extern const char* const water_shadowmap_shader_v;

extern const char* const aircrafts_shader_v;
extern const char* const aircrafts_shader_f;

extern const char* const static_models_shader_v;
extern const char* const static_models_shader_f;

extern const char* const static_models_shadowmap_shader_v;
extern const char* const static_models_shadowmap_shader_f;

extern const char* const powerups_shader_v;
extern const char* const powerups_shader_f;

extern const char* const forcefield_shader_v;
extern const char* const forcefield_shader_f;

extern const char* const naviball_shader_v;
extern const char* const naviball_shader_f;

extern const char* const aircrafts_shadowmap_shader_v;

extern const char* const aircrafts_stencil_shadow_shader_v;
extern const char* const aircrafts_stencil_shadow_shader_g;

extern const char* const sun_shader_v;
extern const char* const sun_shader_f;

extern const char* const sky_shader_v;
extern const char* const sky_shader_f;
extern const char* const clouds_shader_v;
extern const char* const clouds_shader_f;

extern const char* const stars_shader_v;
extern const char* const stars_shader_f;

extern const char* const particles_shader_v;
extern const char* const particles_shader_f;

extern const char* const tonemapping_shader_v;
extern const char* const tonemapping_shader_f;
extern const char* const histogram_fetch_shader_v;
extern const char* const histogram_fetch_shader_f;
extern const char* const histogram_write_shader_v;
extern const char* const histogram_write_shader_f;
extern const char* const brightness_computing_shader_v;
extern const char* const brightness_computing_shader_f;
extern const char* const tonemapping_factor_accumulate_shader_v;
extern const char* const tonemapping_factor_accumulate_shader_f;
#ifdef MF_DEBUG
extern const char* const histogram_show_shader_f;
extern const char* const histogram_show_shader_v;
extern const char* const histogram_buffer_show_shader_v;
extern const char* const histogram_buffer_show_shader_f;
#endif

extern const char* const clouds_gen_shader_v;
extern const char* const clouds_gen_shader_f;
extern const char* const clouds_downscale_shader_v;
extern const char* const clouds_downscale_shader_f;
} // namespace mf_Shaders