/*************************************************************************/
/*  style_box.h                                                          */
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

#include "core/resource.h"
#include "scene/resources/texture.h"
#include "servers/rendering_server.h"

class CanvasItem;

class GODOT_EXPORT StyleBox : public Resource {

    GDCLASS(StyleBox,Resource)

    RES_BASE_EXTENSION("stylebox")
    OBJ_SAVE_TYPE(StyleBox)
    float margin[4];

protected:
    virtual float get_style_margin(Margin p_margin) const = 0;
    static void _bind_methods();

public:
    virtual bool test_mask(const Point2 &p_point, const Rect2 &p_rect) const;

    void set_default_margin(Margin p_margin, float p_value);
    float get_default_margin(Margin p_margin) const;
    float get_margin(Margin p_margin) const;
    virtual Size2 get_center_size() const;

    virtual Rect2 get_draw_rect(const Rect2 &p_rect) const;
    virtual void draw(RenderingEntity p_canvas_item, const Rect2 &p_rect) const = 0;


    Size2 get_minimum_size() const;
    Point2 get_offset() const;

    StyleBox();
};

class StyleBoxEmpty : public StyleBox {

    GDCLASS(StyleBoxEmpty,StyleBox)

    float get_style_margin(Margin /*p_margin*/) const override { return 0; }

public:
    void draw(RenderingEntity /*p_canvas_item*/, const Rect2 & /*p_rect*/) const override {}
    StyleBoxEmpty() {}
};

class GODOT_EXPORT StyleBoxTexture : public StyleBox {

    GDCLASS(StyleBoxTexture,StyleBox)

public:
    enum AxisStretchMode {
        AXIS_STRETCH_MODE_STRETCH,
        AXIS_STRETCH_MODE_TILE,
        AXIS_STRETCH_MODE_TILE_FIT,
    };

private:
    float expand_margin[4];
    float margin[4];
    Rect2 region_rect;
    Ref<Texture> texture;
    Ref<Texture> normal_map;
    bool draw_center;
    Color modulate;
    AxisStretchMode axis_h;
    AxisStretchMode axis_v;

protected:
    float get_style_margin(Margin p_margin) const override;
    static void _bind_methods();

public:
    void set_expand_margin_size(Margin p_expand_margin, float p_size);
    void set_expand_margin_size_all(float p_expand_margin_size);
    void set_expand_margin_size_individual(float p_left, float p_top, float p_right, float p_bottom);
    float get_expand_margin_size(Margin p_expand_margin) const;

    void set_margin_size(Margin p_margin, float p_size);
    float get_margin_size(Margin p_margin) const;

    void set_region_rect(const Rect2 &p_region_rect);
    Rect2 get_region_rect() const;

    void set_texture(const Ref<Texture>& p_texture);
    Ref<Texture> get_texture() const;

    void set_normal_map(const Ref<Texture>& p_normal_map);
    Ref<Texture> get_normal_map() const;

    void set_draw_center(bool p_enabled);
    bool is_draw_center_enabled() const { return draw_center; }
    Size2 get_center_size() const override;

    void set_h_axis_stretch_mode(AxisStretchMode p_mode);
    AxisStretchMode get_h_axis_stretch_mode() const;

    void set_v_axis_stretch_mode(AxisStretchMode p_mode);
    AxisStretchMode get_v_axis_stretch_mode() const;

    void set_modulate(const Color &p_modulate);
    Color get_modulate() const { return modulate; }

    Rect2 get_draw_rect(const Rect2 &p_rect) const override;
    void draw(RenderingEntity p_canvas_item, const Rect2 &p_rect) const override;

    StyleBoxTexture();
    ~StyleBoxTexture() override;
};


class GODOT_EXPORT StyleBoxFlat : public StyleBox {

    GDCLASS(StyleBoxFlat,StyleBox)

    Color bg_color;
    Color shadow_color;
    Color border_color;

    float border_width[4];
    float expand_margin[4];
    float corner_radius[4];

    bool draw_center;
    bool blend_border;
    bool anti_aliased;

    int corner_detail;
    int shadow_size;
    Point2 shadow_offset;
    float aa_size;

protected:
    float get_style_margin(Margin p_margin) const override;
    static void _bind_methods();

public:
    void set_bg_color(const Color &p_color);
    Color get_bg_color() const;

    void set_border_color(const Color &p_color);
    Color get_border_color() const;

    void set_border_width_all(int p_size);
    int get_border_width_min() const;

    void set_border_width(Margin p_margin, int p_width);
    int get_border_width(Margin p_margin) const;

    void set_border_blend(bool p_blend);
    bool get_border_blend() const;

    void set_corner_radius_all(int radius);
    void set_corner_radius_individual(const int radius_top_left, const int radius_top_right, const int radius_bottom_right, const int radius_bottom_left);
    int get_corner_radius_min() const;

    void set_corner_radius(Corner p_corner, const int radius);
    int get_corner_radius(Corner p_corner) const;

    void set_corner_detail(const int &p_corner_detail);
    int get_corner_detail() const;

    void set_expand_margin_size(Margin p_expand_margin, float p_size);
    void set_expand_margin_size_all(float p_expand_margin_size);
    void set_expand_margin_size_individual(float p_left, float p_top, float p_right, float p_bottom);
    float get_expand_margin_size(Margin p_expand_margin) const;

    void set_draw_center(bool p_enabled);
    bool is_draw_center_enabled() const;

    void set_shadow_color(const Color &p_color);
    Color get_shadow_color() const;

    void set_shadow_size(const int &p_size);
    int get_shadow_size() const;

    void set_shadow_offset(const Point2 &p_offset);
    Point2 get_shadow_offset() const;

    void set_anti_aliased(const bool &p_anti_aliased);
    bool is_anti_aliased() const;

    void set_aa_size(const float &p_aa_size);
    float get_aa_size() const;

    Size2 get_center_size() const override;

    Rect2 get_draw_rect(const Rect2 &p_rect) const override;
    void draw(RenderingEntity p_canvas_item, const Rect2 &p_rect) const override;

    StyleBoxFlat();
    ~StyleBoxFlat() override;
};

// Just used to draw lines.
class GODOT_EXPORT StyleBoxLine : public StyleBox {

    GDCLASS(StyleBoxLine,StyleBox)

    Color color;
    int thickness;
    bool vertical;
    float grow_begin;
    float grow_end;

protected:
    float get_style_margin(Margin p_margin) const override;
    static void _bind_methods();

public:
    void set_color(const Color &p_color);
    Color get_color() const;

    void set_thickness(int p_thickness);
    int get_thickness() const;

    void set_vertical(bool p_vertical);
    bool is_vertical() const;

    void set_grow_begin(float p_grow);
    float get_grow_begin() const;

    void set_grow_end(float p_grow);
    float get_grow_end() const;

    Size2 get_center_size() const override;

    void draw(RenderingEntity p_canvas_item, const Rect2 &p_rect) const override;

    StyleBoxLine();
    ~StyleBoxLine() override;
};
