/*************************************************************************/
/*  collision_polygon_3d.cpp                                             */
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

#include "collision_polygon_3d.h"

#include "scene/3d/collision_object_3d.h"
#include "scene/resources/concave_polygon_shape_3d.h"
#include "scene/resources/convex_polygon_shape_3d.h"
#include "core/method_bind.h"
#include "core/math/geometry.h"
#include "core/translation_helpers.h"

IMPL_GDCLASS(CollisionPolygon3D)

void CollisionPolygon3D::_build_polygon() {

    if (!parent)
        return;

    parent->shape_owner_clear_shapes(owner_id);

    if (polygon.empty())
        return;

    Vector<Vector<Vector2> > decomp = Geometry::decompose_polygon_in_convex(polygon);
    if (decomp.empty())
        return;

    //here comes the sun, lalalala
    //decompose concave into multiple convex polygons and add them

    for (size_t i = 0; i < decomp.size(); i++) {
        Ref<ConvexPolygonShape3D> convex(make_ref_counted<ConvexPolygonShape3D>());
        Vector<Vector3> cp;
        size_t cs = decomp[i].size();
        cp.reserve(cs * 2);
        {
            for (size_t j = 0; j < cs; j++) {

                Vector2 d = decomp[i][j];
                cp.emplace_back(d.x, d.y, depth * 0.5f);
                cp.emplace_back(d.x, d.y, -depth * 0.5f);
            }
        }

        convex->set_points(eastl::move(cp));
        convex->set_margin(margin);
        parent->shape_owner_add_shape(owner_id, convex);
        parent->shape_owner_set_disabled(owner_id, disabled);
    }
}

void CollisionPolygon3D::_update_in_shape_owner(bool p_xform_only) {

    parent->shape_owner_set_transform(owner_id, get_transform());
    if (p_xform_only)
        return;
    parent->shape_owner_set_disabled(owner_id, disabled);
}

void CollisionPolygon3D::_notification(int p_what) {

    switch (p_what) {

        case NOTIFICATION_PARENTED: {
            parent = object_cast<CollisionObject3D>(get_parent());
            if (parent) {
                owner_id = parent->create_shape_owner(this);
                _build_polygon();
                _update_in_shape_owner();
            }
        } break;
        case NOTIFICATION_ENTER_TREE: {

            if (parent) {
                _update_in_shape_owner();
            }

        } break;
        case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {

            if (parent) {
                _update_in_shape_owner(true);
            }

        } break;
        case NOTIFICATION_UNPARENTED: {
            if (parent) {
                parent->remove_shape_owner(owner_id);
            }
            owner_id = 0;
            parent = nullptr;
        } break;
    }
}

void CollisionPolygon3D::set_polygon(const Vector<Vector2> &p_polygon) {

    polygon = p_polygon;
    if (parent) {
        _build_polygon();
    }
    update_configuration_warning();
    update_gizmo();
}

const Vector<Vector2> &CollisionPolygon3D::get_polygon() const {

    return polygon;
}

AABB CollisionPolygon3D::get_item_rect() const {

    return aabb;
}

void CollisionPolygon3D::set_depth(float p_depth) {

    depth = p_depth;
    _build_polygon();
    update_gizmo();
}

float CollisionPolygon3D::get_depth() const {

    return depth;
}

void CollisionPolygon3D::set_disabled(bool p_disabled) {
    disabled = p_disabled;
    update_gizmo();

    if (parent) {
        parent->shape_owner_set_disabled(owner_id, p_disabled);
    }
}

bool CollisionPolygon3D::is_disabled() const {
    return disabled;
}

float CollisionPolygon3D::get_margin() const {
    return margin;
}

void CollisionPolygon3D::set_margin(float p_margin) {
    margin = p_margin;
    if (parent) {
        _build_polygon();
    }
}
String CollisionPolygon3D::get_configuration_warning() const {


    String warning = BaseClassName::get_configuration_warning();
    if (!object_cast<CollisionObject3D>(get_parent())) {
        if (!warning.empty()) {
            warning += "\n\n";
        }
        warning += TTR("CollisionPolygon only serves to provide a collision shape to a CollisionObject derived node. Please only use it as a child of Area, StaticBody, RigidBody, KinematicBody, etc. to give them a shape.");
    }

    if (polygon.empty()) {
        if (!warning.empty()) {
            warning += "\n\n";
        }
        warning += TTR("An empty CollisionPolygon has no effect on collision.");
    }

    return warning;
}

bool CollisionPolygon3D::_is_editable_3d_polygon() const {
    return true;
}
void CollisionPolygon3D::_bind_methods() {

    SE_BIND_METHOD(CollisionPolygon3D,set_depth);
    SE_BIND_METHOD(CollisionPolygon3D,get_depth);

    SE_BIND_METHOD(CollisionPolygon3D,set_polygon);
    SE_BIND_METHOD(CollisionPolygon3D,get_polygon);

    SE_BIND_METHOD(CollisionPolygon3D,set_disabled);
    SE_BIND_METHOD(CollisionPolygon3D,is_disabled);
    SE_BIND_METHOD(CollisionPolygon3D,set_margin);
    SE_BIND_METHOD(CollisionPolygon3D,get_margin);

    SE_BIND_METHOD(CollisionPolygon3D,_is_editable_3d_polygon);

    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "depth"), "set_depth", "get_depth");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "disabled"), "set_disabled", "is_disabled");
    ADD_PROPERTY(PropertyInfo(VariantType::POOL_VECTOR2_ARRAY, "polygon"), "set_polygon", "get_polygon");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "margin", PropertyHint::Range, "0.001,10,0.001"), "set_margin", "get_margin");
}

CollisionPolygon3D::CollisionPolygon3D() {

    aabb = AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
    depth = 1.0;
    set_notify_local_transform(true);
    parent = nullptr;
    owner_id = 0;
    disabled = false;
}
