/*************************************************************************/
/*  particles_material.h                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#pragma once

#include "core/rid.h"
#include "scene/resources/material.h"
#include "core/hash_map.h"

class CurveTexture;
class GradientTexture;

class GODOT_EXPORT ParticlesMaterial : public Material {

    GDCLASS(ParticlesMaterial,Material)

public:
    enum Parameter : uint8_t {

        PARAM_INITIAL_LINEAR_VELOCITY=0,
        PARAM_ANGULAR_VELOCITY,
        PARAM_ORBIT_VELOCITY,
        PARAM_LINEAR_ACCEL,
        PARAM_RADIAL_ACCEL,
        PARAM_TANGENTIAL_ACCEL,
        PARAM_DAMPING,
        PARAM_ANGLE,
        PARAM_SCALE,
        PARAM_HUE_VARIATION,
        PARAM_ANIM_SPEED,
        PARAM_ANIM_OFFSET,
        PARAM_MAX
    };

    // When extending, make sure not to overflow the size of the MaterialKey below.
    enum Flags {
        FLAG_ALIGN_Y_TO_VELOCITY,
        FLAG_ROTATE_Y,
        FLAG_DISABLE_Z,
        FLAG_MAX
    };

    // When extending, make sure not to overflow the size of the MaterialKey below.
    enum EmissionShape {
        EMISSION_SHAPE_POINT,
        EMISSION_SHAPE_SPHERE,
        EMISSION_SHAPE_BOX,
        EMISSION_SHAPE_POINTS,
        EMISSION_SHAPE_DIRECTED_POINTS,
        EMISSION_SHAPE_RING,
        EMISSION_SHAPE_MAX
    };

private:
    // The bit size of the struct must be kept below or equal to 32 bits.
    // Consider this when extending Flags, or EmissionShape.
    union MaterialKey {

        struct {
            uint32_t texture_mask : 16;
            uint32_t texture_color : 1;
            uint32_t texture_initial_color : 1;
            uint32_t flags : 4;
            uint32_t emission_shape : 3;
            uint32_t trail_size_texture : 1;
            uint32_t trail_color_texture : 1;
            uint32_t invalid_key : 1;
            uint32_t has_emission_color : 1;
        };

        uint32_t key;

        bool operator<(const MaterialKey &p_key) const {
            return key < p_key.key;
        }
        // for shader_map eastl::hash key
        constexpr operator size_t() const { return key; }
    };

    struct ShaderData {
        RenderingEntity shader;
        int users;
    };

    static HashMap<MaterialKey, ShaderData> shader_map;

    MaterialKey current_key;

    _FORCE_INLINE_ MaterialKey _compute_key() const {

        MaterialKey mk;
        mk.key = 0;
        for (int i = 0; i < PARAM_MAX; i++) {
            if (tex_parameters[i]) {
                mk.texture_mask |= (1 << i);
            }
        }
        for (int i = 0; i < FLAG_MAX; i++) {
            if (flags[i]) {
                mk.flags |= (1 << i);
            }
        }

        mk.texture_color = color_ramp ? 1 : 0;
        mk.texture_initial_color = color_initial_ramp ? 1 : 0;
        mk.emission_shape = emission_shape;
        mk.trail_color_texture = trail_color_modifier ? 1 : 0;
        mk.trail_size_texture = trail_size_modifier ? 1 : 0;
        mk.has_emission_color = emission_shape >= EMISSION_SHAPE_POINTS && emission_color_texture;

        return mk;
    }

    static Mutex material_mutex;

    bool is_dirty_element; //!< this value is set when this material is waiting to be updated by \fn _update_shader

    void _update_shader();
    _FORCE_INLINE_ void _queue_shader_change();
    //_FORCE_INLINE_ bool _is_shader_dirty() const;

    bool is_initialized = false;
    Vector3 direction;
    float spread;
    float flatness;

    float parameters[PARAM_MAX];
    float randomness[PARAM_MAX];

    Ref<Texture> tex_parameters[PARAM_MAX];
    Color color;
    Ref<Texture> color_ramp;
    Ref<Texture> color_initial_ramp;

    bool flags[FLAG_MAX];

    EmissionShape emission_shape;
    float emission_sphere_radius;
    Vector3 emission_box_extents;
    Ref<Texture> emission_point_texture;
    Ref<Texture> emission_normal_texture;
    Ref<Texture> emission_color_texture;
    int emission_point_count;
    float emission_ring_height;
    float emission_ring_radius;
    float emission_ring_inner_radius;
    Vector3 emission_ring_axis;

    bool anim_loop;

    int trail_divisor;

    Ref<CurveTexture> trail_size_modifier;
    Ref<GradientTexture> trail_color_modifier;

    Vector3 gravity;

    float lifetime_randomness;

    //do not save emission points here

protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &property) const override;

public:
    void set_direction(Vector3 p_direction);
    Vector3 get_direction() const;

    void set_spread(float p_spread);
    float get_spread() const;

    void set_flatness(float p_flatness);
    float get_flatness() const;

    void set_param(Parameter p_param, float p_value);
    float get_param(Parameter p_param) const;

    void set_param_randomness(Parameter p_param, float p_value);
    float get_param_randomness(Parameter p_param) const;

    void set_param_texture(Parameter p_param, const Ref<Texture> &p_texture);
    Ref<Texture> get_param_texture(Parameter p_param) const;

    void set_color(const Color &p_color);
    Color get_color() const;

    void set_color_ramp(const Ref<Texture> &p_texture);
    Ref<Texture> get_color_ramp() const;

    void set_color_initial_ramp(const Ref<Texture> &p_texture);
    Ref<Texture> get_color_initial_ramp() const;
    void set_flag(Flags p_flag, bool p_enable);
    bool get_flag(Flags p_flag) const;

    void set_emission_shape(EmissionShape p_shape);
    void set_emission_sphere_radius(float p_radius);
    void set_emission_box_extents(Vector3 p_extents);
    void set_emission_point_texture(const Ref<Texture> &p_points);
    void set_emission_normal_texture(const Ref<Texture> &p_normals);
    void set_emission_color_texture(const Ref<Texture> &p_colors);
    void set_emission_point_count(int p_count);
    void set_emission_ring_radius(float p_radius);
    void set_emission_ring_inner_radius(float p_offset);
    void set_emission_ring_height(float p_height);
    void set_emission_ring_axis(Vector3 p_axis);

    EmissionShape get_emission_shape() const;
    float get_emission_sphere_radius() const;
    Vector3 get_emission_box_extents() const;
    Ref<Texture> get_emission_point_texture() const;
    Ref<Texture> get_emission_normal_texture() const;
    Ref<Texture> get_emission_color_texture() const;
    int get_emission_point_count() const;
    float get_emission_ring_radius() const;
    float get_emission_ring_inner_radius() const;
    float get_emission_ring_height() const;
    Vector3 get_emission_ring_axis() const;

    void set_trail_divisor(int p_divisor);
    int get_trail_divisor() const;

    void set_trail_size_modifier(const Ref<CurveTexture> &p_trail_size_modifier);
    Ref<CurveTexture> get_trail_size_modifier() const;

    void set_trail_color_modifier(const Ref<GradientTexture> &p_trail_color_modifier);
    Ref<GradientTexture> get_trail_color_modifier() const;

    void set_gravity(const Vector3 &p_gravity);
    Vector3 get_gravity() const;

    void set_lifetime_randomness(float p_lifetime);
    float get_lifetime_randomness() const;

    static void init_shaders();
    static void finish_shaders();
    static void flush_changes();

    RenderingEntity get_shader_rid() const;

    RenderingServerEnums::ShaderMode get_shader_mode() const override;

    ParticlesMaterial();
    ~ParticlesMaterial() override;
};
struct CurveRange {
    float curve_min,curve_max;
    constexpr CurveRange(float mn,float mx) : curve_min(mn),curve_max(mx) {}
    constexpr bool valid() const { return curve_min!=curve_max;}
};


constexpr CurveRange c_default_curve_ranges[ParticlesMaterial::PARAM_MAX]{
    { 0.0f, 0.0f }, // PARAM_INITIAL_LINEAR_VELOCITY
    { -360.0f, 360.0f }, // PARAM_ANGULAR_VELOCITY
    { -500.0f, 500.0f }, // PARAM_ORBIT_VELOCITY
    { -200.0f, 200.0f }, // PARAM_LINEAR_ACCEL
    { -200.0f, 200.0f }, // PARAM_RADIAL_ACCEL
    { -200.0f, 200.0f }, // PARAM_TANGENTIAL_ACCEL
    { -0.0f, 100.0f }, // PARAM_DAMPING
    { -360.0f, 360.0f }, // PARAM_ANGLE
    { 0.0f, 1.0f }, // PARAM_SCALE
    { -1.0f, 1.0f }, // PARAM_HUE_VARIATION
    { 0.0f, 200.0f }, // PARAM_ANIM_SPEED
    { 0.0f, 0.0f } // PARAM_ANIM_OFFSET
};

namespace ParticleUtils {
// Functions usedby particle systems.
constexpr inline uint32_t idhash(uint32_t x) {

    x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
    x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
    x = (x >> uint32_t(16)) ^ x;
    return x;
}

constexpr inline float rand_from_seed(uint32_t &seed) {
    int s = int(seed);
    if (s == 0)
        s = 305420679;
    int k = s / 127773;
    s = 16807 * (s - k * 127773) - 2836 * k;
    if (s < 0)
        s += 2147483647;
    seed = uint32_t(s);
    return float(seed % uint32_t(65536)) / 65535.0f;
}
}
