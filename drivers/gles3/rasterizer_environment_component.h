#pragma once

#include "core/color.h"
#include "core/engine_entities.h"
#include "core/math/basis.h"
#include "servers/rendering_server_enums.h"
#include "servers/rendering/render_entity_helpers.h"

struct RasterizerEnvironmentComponent {
    MoveOnlyEntityHandle sky;
    float sky_custom_fov = 0.0;
    Basis sky_orientation;

    Color bg_color;
    float bg_energy = 1.0;
    float sky_ambient = 0;

    int camera_feed_id = 0;

    Color ambient_color;
    float ambient_energy = 1.0;
    float ambient_sky_contribution = 0.0;

    int canvas_max_layer = 0;

    int ssr_max_steps = 64;
    float ssr_fade_in = 0.15f;
    float ssr_fade_out = 2.0f;
    float ssr_depth_tolerance = 0.2f;

    float ssao_intensity = 1.0f;
    float ssao_radius = 1.0f;
    float ssao_intensity2 = 1.0f;
    float ssao_radius2 = 0.0;
    float ssao_bias = 0.01f;
    float ssao_light_affect = 0;
    float ssao_ao_channel_affect = 0;
    Color ssao_color;
    float ssao_bilateral_sharpness = 4;

    int glow_levels = (1 << 2) | (1 << 4);
    float glow_intensity = 0.8f;
    float glow_strength = 1.0f;
    float glow_bloom = 0.0;
    float glow_hdr_bleed_threshold = 1.0f;
    float glow_hdr_bleed_scale = 2.0f;
    float glow_hdr_luminance_cap = 12.0f;

    float tone_mapper_exposure = 1.0f;
    float tone_mapper_exposure_white = 1.0f;
    float auto_exposure_speed = 0.5f;
    float auto_exposure_min = 0.05f;
    float auto_exposure_max = 8;
    float auto_exposure_grey = 0.4f;

    float dof_blur_far_distance = 10;
    float dof_blur_far_transition = 5;
    float dof_blur_far_amount = 0.1f;

    float dof_blur_near_distance = 2;
    float dof_blur_near_transition = 1;
    float dof_blur_near_amount = 0.1f;

    float adjustments_brightness = 1.0f;
    float adjustments_contrast = 1.0f;
    float adjustments_saturation = 1.0f;
    MoveOnlyEntityHandle color_correction;

    Color fog_color = Color(0.5f, 0.5f, 0.5f);
    Color fog_sun_color = Color(0.8f, 0.8f, 0.0);
    float fog_sun_amount = 0;

    RS::EnvironmentBG bg_mode = RS::ENV_BG_CLEAR_COLOR;
    RS::EnvironmentSSAOQuality ssao_quality = RS::ENV_SSAO_QUALITY_LOW;
    RS::EnvironmentSSAOBlur ssao_filter = RS::ENV_SSAO_BLUR_3x3;
    RS::EnvironmentGlowBlendMode glow_blend_mode = RS::GLOW_BLEND_MODE_SOFTLIGHT;
    RS::EnvironmentToneMapper tone_mapper = RS::ENV_TONE_MAPPER_LINEAR;
    RS::EnvironmentDOFBlurQuality dof_blur_far_quality = RS::ENV_DOF_BLUR_QUALITY_MEDIUM;
    RS::EnvironmentDOFBlurQuality dof_blur_near_quality = RS::ENV_DOF_BLUR_QUALITY_MEDIUM;

    float fog_depth_begin = 10;
    float fog_depth_end = 0;
    float fog_depth_curve = 1;
    float fog_transmit_curve = 1;
    float fog_height_min = 10;
    float fog_height_max = 0;
    float fog_height_curve = 1;

    bool ssr_enabled = false;
    bool ssr_roughness = true;
    bool ssao_enabled = false;
    bool glow_enabled = false;
    bool glow_bicubic_upscale = false;
    bool glow_high_quality = false;
    bool auto_exposure = false;
    bool dof_blur_far_enabled = false;
    bool dof_blur_near_enabled = false;
    bool adjustments_enabled = false;
    bool fog_enabled = false;
    bool fog_depth_enabled = true;
    bool fog_transmit_enabled = true;
    bool fog_height_enabled = false;
};

bool is_environment(RenderingEntity p_env);

