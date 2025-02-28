/*************************************************************************/
/*  graph_edit.h                                                         */
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

#include "scene/gui/box_container.h"
#include "scene/gui/graph_node.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tool_button.h"
#include "core/list.h"

class GraphEdit;

class GraphEditFilter : public Control {

    GDCLASS(GraphEditFilter,Control)

    friend class GraphEdit;
    friend class GraphEditMinimap;
    GraphEdit *ge;
    bool has_point(const Point2 &p_point) const override;

public:
    GraphEditFilter(GraphEdit *p_edit);
};

class GODOT_EXPORT GraphEditMinimap : public Control {
    GDCLASS(GraphEditMinimap, Control);

    friend class GraphEdit;
    friend class GraphEditFilter;
    GraphEdit *ge;

protected:
    static void _bind_methods();

public:
    GraphEditMinimap(GraphEdit *p_edit);

    void update_minimap();
    Rect2 get_camera_rect();

private:
    Vector2 minimap_padding;
    Vector2 minimap_offset;
    Vector2 graph_proportions;
    Vector2 graph_padding;
    Vector2 camera_position;
    Vector2 camera_size;

    bool is_pressing;
    bool is_resizing;

    Vector2 _get_render_size();
    Vector2 _get_graph_offset();
    Vector2 _get_graph_size();

    Vector2 _convert_from_graph_position(const Vector2 &p_position);
    Vector2 _convert_to_graph_position(const Vector2 &p_position);

    void _gui_input(const Ref<InputEvent> &p_ev);

    void _adjust_graph_scroll(const Vector2 &p_offset);
};

class GODOT_EXPORT GraphEdit : public Control {

    GDCLASS(GraphEdit,Control)

public:
    struct Connection {
        StringName from;
        StringName to;
        int from_port;
        int to_port;
        float activity;
    };

private:
    Label *zoom_label;
    ToolButton *zoom_minus;
    ToolButton *zoom_reset;
    ToolButton *zoom_plus;

    ToolButton *snap_button;
    SpinBox *snap_amount;
    Button *minimap_button;

    HScrollBar *h_scroll;
    VScrollBar *v_scroll;

    float port_grab_distance_horizontal;
    float port_grab_distance_vertical;

    StringName connecting_from;
    int connecting_index;
    int connecting_type;
    Color connecting_color;
    Vector2 connecting_to;
    StringName connecting_target_to;
    int connecting_target_index;

    Vector2 drag_accum;
    Vector2 click_pos;
    Point2 drag_origin; // Workaround for GH-5907

    float zoom = 1.0f;
    float zoom_step = 1.2f;
    float zoom_min;
    float zoom_max;

    Point2 box_selecting_from;
    Point2 box_selecting_to;
    Rect2 box_selecting_rect;
    Vector<GraphNode *> previous_selected;
    List<Connection> connections;
    Control *connections_layer;
    GraphEditFilter *top_layer;
    GraphEditMinimap *minimap;
    bool connecting=false;
    bool connecting_out=false;
    bool connecting_target=false;
    bool just_disconnected=false;
    bool connecting_valid = false;
    bool dragging = false;
    bool just_selected = false;
    bool moving_selection = false;
    bool box_selecting = false;
    bool box_selection_mode_additive = false;
    bool setting_scroll_ofs = false;
    bool right_disconnects = false;
    bool updating = false;
    bool awaiting_scroll_offset_update = false;
    bool lines_on_bg;

    struct ConnType {

        union {
            struct {
                uint32_t type_a;
                uint32_t type_b;
            };
            uint64_t key;
        };

        bool operator==(ConnType p_type) const {
            return key == p_type.key;
        }

        ConnType(uint32_t a = 0, uint32_t b = 0) {
            type_a = a;
            type_b = b;
        }
    private:
        friend eastl::hash<ConnType>;
        explicit operator size_t() const {
            return key;
        }
    };

    HashSet<ConnType> valid_connection_types;
    HashSet<int> valid_left_disconnect_types;
    HashSet<int> valid_right_disconnect_types;

    HBoxContainer *zoom_hb;
public:
    void _bake_segment2d(Vector<Vector2> &points, Vector<Color> &colors, float p_begin, float p_end, const Vector2 &p_a, const Vector2 &p_out, const Vector2 &p_b, const Vector2 &p_in, int p_depth, int p_min_depth, int p_max_depth, float p_tol, const Color &p_color, const Color &p_to_color, int &lines) const;

    void _draw_cos_line(CanvasItem *p_where, const Vector2 &p_from, const Vector2 &p_to, const Color &p_color, const Color &p_to_color, float p_width = 2.0f, float p_bezier_ratio = 1.0f);

    void _graph_node_raised(Node *p_gn);
    void _graph_node_moved(Node *p_gn);
    void _graph_node_slot_updated(int p_index, Node *p_gn);

    void _update_scroll();
    void _scroll_moved(double);
    void _gui_input(const Ref<InputEvent> &p_ev);

    void _top_layer_input(const Ref<InputEvent> &p_ev);

    bool is_in_hot_zone(const Vector2 &pos, const Vector2 &p_mouse_pos, const Vector2i &p_port_size, bool p_left);

    void _top_layer_draw();
    void _connections_layer_draw();
    void _minimap_draw();
    void _update_scroll_offset();

    Array _get_connection_list() const;
private:

    friend class GraphEditFilter;
    friend class GraphEditMinimap;

    bool _filter_input(const Point2 &p_point);
    void _snap_toggled();
    void _snap_value_changed(double);

    void _minimap_toggled();
    bool _check_clickable_control(Control *p_control, const Vector2 &pos);

    void _zoom_minus();
    void _zoom_reset();
    void _zoom_plus();
    void _update_zoom_label();
protected:
    static void _bind_methods();
    void add_child_notify(Node *p_child) override;
    void remove_child_notify(Node *p_child) override;
    void _notification(int p_what);
    bool clips_input() const override;

public:
    Error connect_node(const StringName &p_from, int p_from_port, const StringName &p_to, int p_to_port);
    bool is_node_connected(const StringName &p_from, int p_from_port, const StringName &p_to, int p_to_port);
    void disconnect_node(const StringName &p_from, int p_from_port, const StringName &p_to, int p_to_port);
    void clear_connections();

    void set_connection_activity(const StringName &p_from, int p_from_port, const StringName &p_to, int p_to_port, float p_activity);

    void add_valid_connection_type(int p_type, int p_with_type);
    void remove_valid_connection_type(int p_type, int p_with_type);
    bool is_valid_connection_type(int p_type, int p_with_type) const;

    void set_zoom(float p_zoom);
    void set_zoom_custom(float p_zoom, const Vector2 &p_center);
    float get_zoom() const;
    void set_zoom_min(float p_zoom_min);
    float get_zoom_min() const;

    void set_zoom_max(float p_zoom_max);
    float get_zoom_max() const;

    void set_zoom_step(float p_zoom_step);
    float get_zoom_step() const;

    void set_show_zoom_label(bool p_enable);
    bool is_showing_zoom_label() const;

    void set_minimap_size(Vector2 p_size);
    Vector2 get_minimap_size() const;
    void set_minimap_opacity(float p_opacity);
    float get_minimap_opacity() const;

    void set_minimap_enabled(bool p_enable);
    bool is_minimap_enabled() const;

    GraphEditFilter *get_top_layer() const { return top_layer; }
    GraphEditMinimap *get_minimap() const { return minimap; }
    void get_connection_list(List<Connection> *r_connections) const;

    void set_right_disconnects(bool p_enable);
    bool is_right_disconnects_enabled() const;

    void add_valid_right_disconnect_type(int p_type);
    void remove_valid_right_disconnect_type(int p_type);

    void add_valid_left_disconnect_type(int p_type);
    void remove_valid_left_disconnect_type(int p_type);

    void set_scroll_ofs(const Vector2 &p_ofs);
    Vector2 get_scroll_ofs() const;

    void set_selected(Node *p_child);

    void set_use_snap(bool p_enable);
    bool is_using_snap() const;

    int get_snap() const;
    void set_snap(int p_snap);

    HBoxContainer *get_zoom_hbox();

    GraphEdit();
};
