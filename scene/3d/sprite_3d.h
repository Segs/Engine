/*************************************************************************/
/*  sprite_3d.h                                                          */
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

#ifndef SPRITE_3D_H
#define SPRITE_3D_H

#include "scene/2d/animated_sprite_2d.h"
#include "scene/3d/visual_instance_3d.h"

class TriangleMesh;

class GODOT_EXPORT SpriteBase3D : public GeometryInstance {

    GDCLASS(SpriteBase3D,GeometryInstance)

    mutable Ref<TriangleMesh> triangle_mesh; //cached

public:
    enum DrawFlags {
        FLAG_TRANSPARENT,
        FLAG_SHADED,
        FLAG_DOUBLE_SIDED,
        FLAG_DISABLE_DEPTH_TEST,
        FLAG_FIXED_SIZE,
        FLAG_MAX

    };

    enum AlphaCutMode {
        ALPHA_CUT_DISABLED,
        ALPHA_CUT_DISCARD,
        ALPHA_CUT_OPAQUE_PREPASS
    };

private:
    bool color_dirty;
    Color color_accum;

    SpriteBase3D *parent_sprite;
    Vector<SpriteBase3D *> children;

    bool centered;
    Point2 offset;

    bool hflip;
    bool vflip;

    Color modulate;
    int render_priority = 0;
    float opacity;

    Vector3::Axis axis;
    float pixel_size;
    AABB aabb;

    RenderingEntity mesh;
    RenderingEntity material;

    bool flags[FLAG_MAX];
    AlphaCutMode alpha_cut;
    SpatialMaterial::BillboardMode billboard_mode;
    bool pending_update;
    void _im_update();

    void _propagate_color_changed();

protected:
    Color _get_color_accum();
    void _notification(int p_what);
    static void _bind_methods();
    virtual void _draw() = 0;
    void draw_texture_rect(const Ref<Texture> &p_texture, Rect2 p_dst_rect, Rect2 p_src_rect);
    _FORCE_INLINE_ void set_aabb(const AABB &p_aabb) { aabb = p_aabb; }
    RenderingEntity get_mesh() const { return mesh; }
    RenderingEntity get_material() const { return material; }

    uint32_t mesh_surface_offsets[RS::ARRAY_MAX];
    uint32_t mesh_stride[RS::ARRAY_MAX];
    PoolByteArray mesh_buffer;
    uint32_t mesh_surface_format;

    void _queue_update();

public:
    void set_centered(bool p_center);
    bool is_centered() const;

    void set_offset(const Point2 &p_offset);
    Point2 get_offset() const;

    void set_flip_h(bool p_flip);
    bool is_flipped_h() const;

    void set_flip_v(bool p_flip);
    bool is_flipped_v() const;

    void set_modulate(const Color &p_color);
    Color get_modulate() const;

    void set_opacity(float p_amount);
    float get_opacity() const;
    void set_render_priority(int p_priority);
    int get_render_priority() const;

    void set_pixel_size(float p_amount);
    float get_pixel_size() const;

    void set_axis(Vector3::Axis p_axis);
    Vector3::Axis get_axis() const;

    void set_draw_flag(DrawFlags p_flag, bool p_enable);
    bool get_draw_flag(DrawFlags p_flag) const;

    void set_alpha_cut_mode(AlphaCutMode p_mode);
    AlphaCutMode get_alpha_cut_mode() const;
    void set_billboard_mode(SpatialMaterial::BillboardMode p_mode);
    SpatialMaterial::BillboardMode get_billboard_mode() const;

    virtual Rect2 get_item_rect() const = 0;

    AABB get_aabb() const override;
    Vector<Face3> get_faces(uint32_t p_usage_flags) const override;
    Ref<TriangleMesh> generate_triangle_mesh() const;

    SpriteBase3D();
    ~SpriteBase3D() override;
};

class GODOT_EXPORT Sprite3D : public SpriteBase3D {

    GDCLASS(Sprite3D,SpriteBase3D)

    Ref<Texture> texture;

    bool region;
    Rect2 region_rect;

    int frame;

    int vframes;
    int hframes;

protected:
    void _draw() override;
    static void _bind_methods();

    void _validate_property(PropertyInfo &property) const override;
private /*slots*/:
    void _texture_changed();

public:
    void set_texture(const Ref<Texture> &p_texture);
    Ref<Texture> get_texture() const;

    void set_region(bool p_region);
    bool is_region() const;

    void set_region_rect(const Rect2 &p_region_rect);
    Rect2 get_region_rect() const;

    void set_frame(int p_frame);
    int get_frame() const;

    void set_frame_coords(const Vector2 &p_coord);
    Vector2 get_frame_coords() const;

    void set_vframes(int p_amount);
    int get_vframes() const;

    void set_hframes(int p_amount);
    int get_hframes() const;

    Rect2 get_item_rect() const override;

    Sprite3D();
    //~Sprite3D();
};

class GODOT_EXPORT AnimatedSprite3D : public SpriteBase3D {

    GDCLASS(AnimatedSprite3D,SpriteBase3D)

    Ref<SpriteFrames> frames;
    bool playing;
    StringName animation;
    int frame;

    bool centered;

    float timeout;

    bool hflip;
    bool vflip;

    Color modulate;

    void _res_changed();
public:
    void _reset_timeout();
    void _set_playing(bool p_playing);
    bool _is_playing() const;

protected:
    void _draw() override;
    static void _bind_methods();
    void _notification(int p_what);
    void _validate_property(PropertyInfo &property) const override;

public:
    void set_sprite_frames(const Ref<SpriteFrames> &p_frames);
    Ref<SpriteFrames> get_sprite_frames() const;

    void play(const StringName &p_animation = StringName());
    void stop();
    bool is_playing() const;

    void set_animation(const StringName &p_animation);
    StringName get_animation() const;

    void set_frame(int p_frame);
    int get_frame() const;

    Rect2 get_item_rect() const override;

    String get_configuration_warning() const override;
    AnimatedSprite3D();
};

#endif // SPRITE_3D_H
