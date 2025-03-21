/*************************************************************************/
/*  box_shape_3d.cpp                                                        */
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

#include "box_shape_3d.h"

#include "core/method_bind.h"
#include "core/object_tooling.h"
#include "servers/physics_server_3d.h"

IMPL_GDCLASS(BoxShape3D)

Vector<Vector3> BoxShape3D::get_debug_mesh_lines() {

    Vector<Vector3> lines;
    AABB aabb;
    aabb.position = -get_extents();
    aabb.size = aabb.position * -2;

    for (int i = 0; i < 12; i++) {
        Vector3 a, b;
        aabb.get_edge(i, a, b);
        lines.push_back(a);
        lines.push_back(b);
    }

    return lines;
}

void BoxShape3D::_update_shape() {

    PhysicsServer3D::get_singleton()->shape_set_data(get_shape(), extents);
    Shape::_update_shape();
}

void BoxShape3D::set_extents(const Vector3 &p_extents) {

    extents = p_extents;
    _update_shape();
    notify_change_to_owners();
    Object_change_notify(this,"extents");
}

Vector3 BoxShape3D::get_extents() const {

    return extents;
}

void BoxShape3D::_bind_methods() {

    SE_BIND_METHOD(BoxShape3D,set_extents);
    SE_BIND_METHOD(BoxShape3D,get_extents);

    ADD_PROPERTY(PropertyInfo(VariantType::VECTOR3, "extents"), "set_extents", "get_extents");
}

BoxShape3D::BoxShape3D() :
        Shape(PhysicsServer3D::get_singleton()->shape_create(PhysicsServer3D::SHAPE_BOX)) {

    set_extents(Vector3(1, 1, 1));
}
