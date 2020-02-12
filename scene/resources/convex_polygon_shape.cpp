/*************************************************************************/
/*  convex_polygon_shape.cpp                                             */
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

#include "convex_polygon_shape.h"
#include "core/math/quick_hull.h"
#include "servers/physics_server.h"
#include "core/method_bind.h"

IMPL_GDCLASS(ConvexPolygonShape)

PODVector<Vector3> ConvexPolygonShape::get_debug_mesh_lines() {

    PODVector<Vector3> points = get_points();

    if (points.size() > 3) {

        Geometry::MeshData md;
        Error err = QuickHull::build(points, md);
        if (err == OK) {
            PODVector<Vector3> lines;
            lines.resize(md.edges.size() * 2);
            for (int i = 0; i < md.edges.size(); i++) {
                lines[i * 2 + 0] = md.vertices[md.edges[i].a];
                lines[i * 2 + 1] = md.vertices[md.edges[i].b];
            }
            return lines;
        }
    }

    return PODVector<Vector3>();
}

void ConvexPolygonShape::_update_shape() {

    PhysicsServer::get_singleton()->shape_set_data(get_shape(), points);
    Shape::_update_shape();
}

void ConvexPolygonShape::set_points(PODVector<Vector3> &&p_points) {
    points = eastl::move(p_points);
    _update_shape();
    notify_change_to_owners();
}

real_t ConvexPolygonShape::get_enclosing_radius() const {
    const auto &data = get_points();
    real_t r = 0;
    for (int i(0); i < data.size(); i++) {
        r = MAX(data[i].length_squared(), r);
    }
    return Math::sqrt(r);
}


void ConvexPolygonShape::_bind_methods() {

    MethodBinder::bind_method(D_METHOD("set_points", {"points"}), &ConvexPolygonShape::set_points);
    MethodBinder::bind_method(D_METHOD("get_points"), &ConvexPolygonShape::get_points);

    ADD_PROPERTY(PropertyInfo(VariantType::ARRAY, "points"), "set_points", "get_points");
}

ConvexPolygonShape::ConvexPolygonShape() :
        Shape(PhysicsServer::get_singleton()->shape_create(PhysicsServer::SHAPE_CONVEX_POLYGON)) {
}
