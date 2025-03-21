/*************************************************************************/
/*  area_2d_sw.h                                                         */
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

#include "collision_object_2d_sw.h"

#include "core/map.h"
#include "core/rid.h"
#include "core/self_list.h"
#include "servers/physics_server_2d.h"


class Space2DSW;
class Body2DSW;
class Constraint2DSW;

class Area2DSW : public CollisionObject2DSW {

    PhysicsServer2D::AreaSpaceOverrideMode space_override_mode;
    float gravity;
    Vector2 gravity_vector;
    bool gravity_is_point;
    float gravity_distance_scale;
    float point_attenuation;
    float linear_damp;
    float angular_damp;
    int priority;
    bool monitorable;

    Callable monitor_callback;

    Callable area_monitor_callback;

    IntrusiveListNode<Area2DSW> monitor_query_list;
    IntrusiveListNode<Area2DSW> moved_list;

    struct BodyKey {

        RID rid;
        GameEntity instance_id;
        uint32_t body_shape;
        uint32_t area_shape;

        _FORCE_INLINE_ bool operator<(const BodyKey &p_key) const {

            if (rid == p_key.rid) {

                if (body_shape == p_key.body_shape) {

                    return area_shape < p_key.area_shape;
                } else
                    return body_shape < p_key.body_shape;
            } else
                return rid < p_key.rid;
        }

        _FORCE_INLINE_ BodyKey() {}
        BodyKey(Body2DSW *p_body, uint32_t p_body_shape, uint32_t p_area_shape);
        BodyKey(Area2DSW *p_body, uint32_t p_body_shape, uint32_t p_area_shape);
    };

    struct BodyState {

        int state;
        _FORCE_INLINE_ void inc() { state++; }
        _FORCE_INLINE_ void dec() { state--; }
        _FORCE_INLINE_ BodyState() { state = 0; }
    };

    Map<BodyKey, BodyState> monitored_bodies;
    Map<BodyKey, BodyState> monitored_areas;

    //virtual void shape_changed_notify(Shape2DSW *p_shape);
    //virtual void shape_deleted_notify(Shape2DSW *p_shape);
    HashSet<Constraint2DSW *> constraints;

    void _shapes_changed() override;
    void _queue_monitor_update();

public:
    //_FORCE_INLINE_ const Matrix32& get_inverse_transform() const { return inverse_transform; }
    //_FORCE_INLINE_ SpaceSW* get_owner() { return owner; }

    void set_monitor_callback(Callable&& cb);
    _FORCE_INLINE_ bool has_monitor_callback() const { return monitor_callback.is_valid(); }

    void set_area_monitor_callback(Callable&& cb);
    _FORCE_INLINE_ bool has_area_monitor_callback() const { return area_monitor_callback.is_valid(); }

    _FORCE_INLINE_ void add_body_to_query(Body2DSW *p_body, uint32_t p_body_shape, uint32_t p_area_shape);
    _FORCE_INLINE_ void remove_body_from_query(Body2DSW *p_body, uint32_t p_body_shape, uint32_t p_area_shape);

    _FORCE_INLINE_ void add_area_to_query(Area2DSW *p_area, uint32_t p_area_shape, uint32_t p_self_shape);
    _FORCE_INLINE_ void remove_area_from_query(Area2DSW *p_area, uint32_t p_area_shape, uint32_t p_self_shape);

    void set_param(PhysicsServer2D::AreaParameter p_param, const Variant &p_value);
    Variant get_param(PhysicsServer2D::AreaParameter p_param) const;

    void set_space_override_mode(PhysicsServer2D::AreaSpaceOverrideMode p_mode);
    PhysicsServer2D::AreaSpaceOverrideMode get_space_override_mode() const { return space_override_mode; }

    _FORCE_INLINE_ void set_gravity(float p_gravity) { gravity = p_gravity; }
    _FORCE_INLINE_ float get_gravity() const { return gravity; }

    _FORCE_INLINE_ void set_gravity_vector(const Vector2 &p_gravity) { gravity_vector = p_gravity; }
    _FORCE_INLINE_ Vector2 get_gravity_vector() const { return gravity_vector; }

    _FORCE_INLINE_ void set_gravity_as_point(bool p_enable) { gravity_is_point = p_enable; }
    _FORCE_INLINE_ bool is_gravity_point() const { return gravity_is_point; }

    _FORCE_INLINE_ void set_gravity_distance_scale(float scale) { gravity_distance_scale = scale; }
    _FORCE_INLINE_ float get_gravity_distance_scale() const { return gravity_distance_scale; }

    _FORCE_INLINE_ void set_point_attenuation(float p_point_attenuation) { point_attenuation = p_point_attenuation; }
    _FORCE_INLINE_ float get_point_attenuation() const { return point_attenuation; }

    _FORCE_INLINE_ void set_linear_damp(float p_linear_damp) { linear_damp = p_linear_damp; }
    _FORCE_INLINE_ float get_linear_damp() const { return linear_damp; }

    _FORCE_INLINE_ void set_angular_damp(float p_angular_damp) { angular_damp = p_angular_damp; }
    _FORCE_INLINE_ float get_angular_damp() const { return angular_damp; }

    _FORCE_INLINE_ void set_priority(int p_priority) { priority = p_priority; }
    _FORCE_INLINE_ int get_priority() const { return priority; }

    _FORCE_INLINE_ void add_constraint(Constraint2DSW *p_constraint) { constraints.insert(p_constraint); }
    _FORCE_INLINE_ void remove_constraint(Constraint2DSW *p_constraint) { constraints.erase(p_constraint); }
    const HashSet<Constraint2DSW *> &get_constraints() const { return constraints; }
    _FORCE_INLINE_ void clear_constraints() { constraints.clear(); }

    void set_monitorable(bool p_monitorable);
    _FORCE_INLINE_ bool is_monitorable() const { return monitorable; }

    void set_transform(const Transform2D &p_transform);

    void set_space(Space2DSW *p_space) override;

    void call_queries();

    Area2DSW();
    ~Area2DSW() override;
};

void Area2DSW::add_body_to_query(Body2DSW *p_body, uint32_t p_body_shape, uint32_t p_area_shape) {

    BodyKey bk(p_body, p_body_shape, p_area_shape);
    monitored_bodies[bk].inc();
    if (!monitor_query_list.in_list())
        _queue_monitor_update();
}
void Area2DSW::remove_body_from_query(Body2DSW *p_body, uint32_t p_body_shape, uint32_t p_area_shape) {

    BodyKey bk(p_body, p_body_shape, p_area_shape);
    monitored_bodies[bk].dec();
    if (!monitor_query_list.in_list())
        _queue_monitor_update();
}

void Area2DSW::add_area_to_query(Area2DSW *p_area, uint32_t p_area_shape, uint32_t p_self_shape) {

    BodyKey bk(p_area, p_area_shape, p_self_shape);
    monitored_areas[bk].inc();
    if (!monitor_query_list.in_list())
        _queue_monitor_update();
}
void Area2DSW::remove_area_from_query(Area2DSW *p_area, uint32_t p_area_shape, uint32_t p_self_shape) {

    BodyKey bk(p_area, p_area_shape, p_self_shape);
    monitored_areas[bk].dec();
    if (!monitor_query_list.in_list())
        _queue_monitor_update();
}
