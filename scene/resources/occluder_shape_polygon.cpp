/*************************************************************************/
/*  occluder_shape_polygon.cpp                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "occluder_shape_polygon.h"

#include "core/callable_method_pointer.h"
#include "core/math/transform.h"
#include "core/method_bind.h"
#include "core/math/geometry.h"
#include "servers/rendering_server.h"

IMPL_GDCLASS(OccluderShapePolygon)

#ifdef TOOLS_ENABLED
void OccluderShapePolygon::_update_aabb() {
    _aabb_local = AABB();

    if (_poly_pts_local.empty())
    {
        return;
    }
    Vector3 begin = _vec2to3(_poly_pts_local[0]);
    Vector3 end = begin;

    for (int n = 1; n < _poly_pts_local.size(); n++) {
        Vector3 pt = _vec2to3(_poly_pts_local[n]);
        begin.x = MIN(begin.x, pt.x);
        begin.y = MIN(begin.y, pt.y);
        begin.z = MIN(begin.z, pt.z);
        end.x = M_MAX(end.x, pt.x);
        end.y = M_MAX(end.y, pt.y);
        end.z = M_MAX(end.z, pt.z);
    }

    for (int n = 0; n < _hole_pts_local.size(); n++) {
        Vector3 pt = _vec2to3(_hole_pts_local[n]);
        begin.x = MIN(begin.x, pt.x);
        begin.y = MIN(begin.y, pt.y);
        begin.z = MIN(begin.z, pt.z);
        end.x = M_MAX(end.x, pt.x);
        end.y = M_MAX(end.y, pt.y);
        end.z = M_MAX(end.z, pt.z);
    }

    _aabb_local.position = begin;
    _aabb_local.size = end - begin;
}

AABB OccluderShapePolygon::get_fallback_gizmo_aabb() const {
    return _aabb_local;
}

#endif

void OccluderShapePolygon::_sanitize_points_internal(const Vector<Vector2> &p_from, Vector<Vector2> &r_to) {
    // this function may get rid of some concave points due to user editing ..
    // may not be necessary, no idea how fast it is
    r_to = Geometry::convex_hull_2d(p_from);

    // some peculiarity of convex_hull_2d function, it duplicates the last point for some reason
    if (r_to.size() > 1) {
        r_to.resize(r_to.size() - 1);
    }

    // sort winding, the system expects counter clockwise polys
    Geometry::sort_polygon_winding(r_to, false);
}

void OccluderShapePolygon::_sanitize_points() {
    _sanitize_points_internal(_poly_pts_local_raw, _poly_pts_local);
    _sanitize_points_internal(_hole_pts_local_raw, _hole_pts_local);

#ifdef TOOLS_ENABLED
    _update_aabb();
#endif
}

void OccluderShapePolygon::set_polygon_point(int p_idx, const Vector2 &p_point) {
    if (p_idx >= _poly_pts_local_raw.size()) {
        return;
    }

    _poly_pts_local_raw[p_idx] = p_point;
    _sanitize_points();
    update_shape_to_visual_server();
    notify_change_to_owners();
}

void OccluderShapePolygon::set_hole_point(int p_idx, const Vector2 &p_point) {
    if (p_idx >= _hole_pts_local_raw.size()) {
        return;
    }

    _hole_pts_local_raw[p_idx] = p_point;
    _sanitize_points();
    update_shape_to_visual_server();
    notify_change_to_owners();
}

void OccluderShapePolygon::set_polygon_points(Vector<Vector2> &&p_points) {
    _poly_pts_local_raw = eastl::move(p_points);
    _sanitize_points();
    update_shape_to_visual_server();
    notify_change_to_owners();
}

void OccluderShapePolygon::set_hole_points(const Vector<Vector2> &p_points) {
    _hole_pts_local_raw = p_points;
    _sanitize_points();
    update_shape_to_visual_server();
    notify_change_to_owners();
}

const Vector<Vector2> &OccluderShapePolygon::get_polygon_points() const {
    return _poly_pts_local_raw;
}

const Vector<Vector2> &OccluderShapePolygon::get_hole_points() const {
    return _hole_pts_local_raw;
}

void OccluderShapePolygon::update_shape_to_visual_server() {
    if (_poly_pts_local.size() < 3)
        return;

    OccluderMeshData md;
    md.faces.resize(1);

    OccluderMeshData::Face &face = md.faces[0];
    face.two_way = is_two_way();

    md.vertices.resize(_poly_pts_local.size() + _hole_pts_local.size());
    face.indices.resize(_poly_pts_local.size());

    for (int n = 0; n < _poly_pts_local.size(); n++) {
        md.vertices[n] = _vec2to3(_poly_pts_local[n]);
        face.indices[n] = n;
    }

    // hole points
    if (_hole_pts_local.size()) {
        face.holes.resize(1);
        OccluderMeshData::Hole &hole = face.holes[0];
        hole.indices.resize(_hole_pts_local.size());

        for (int n = 0; n < _hole_pts_local.size(); n++) {
            int dest_idx = n + _poly_pts_local.size();

            hole.indices[n] = dest_idx;
            md.vertices[dest_idx] = _vec2to3(_hole_pts_local[n]);
        }
    }

    face.plane = Plane(Vector3(0, 0, 0), Vector3(0, 0, -1));

    RenderingServer::get_singleton()->occluder_resource_mesh_update(get_shape(), md);
}

void OccluderShapePolygon::set_two_way(bool p_two_way) {
    _settings_two_way = p_two_way;
    update_shape_to_visual_server();
    notify_change_to_owners();
}

Transform OccluderShapePolygon::center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap) {
    return Transform();
}

void OccluderShapePolygon::clear() {
    _poly_pts_local.clear();
    _poly_pts_local_raw.resize(0);
    _hole_pts_local.clear();
    _hole_pts_local_raw.resize(0);
#ifdef TOOLS_ENABLED
    _aabb_local = AABB();
#endif
}

void OccluderShapePolygon::_bind_methods() {
    SE_BIND_METHOD(OccluderShapePolygon,set_two_way);
    SE_BIND_METHOD(OccluderShapePolygon,is_two_way);

    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, {"two_way"}), "set_two_way", "is_two_way");

    SE_BIND_METHOD(OccluderShapePolygon,set_polygon_points);
    SE_BIND_METHOD(OccluderShapePolygon,get_polygon_points);

    SE_BIND_METHOD(OccluderShapePolygon,set_polygon_point);

    ADD_PROPERTY(PropertyInfo(VariantType::POOL_VECTOR2_ARRAY, "polygon_points"), "set_polygon_points", "get_polygon_points");

    SE_BIND_METHOD(OccluderShapePolygon,set_hole_points);
    SE_BIND_METHOD(OccluderShapePolygon,get_hole_points);
    SE_BIND_METHOD(OccluderShapePolygon,set_hole_point);

    ADD_PROPERTY(PropertyInfo(VariantType::POOL_VECTOR2_ARRAY, "hole_points"), "set_hole_points", "get_hole_points");
}

OccluderShapePolygon::OccluderShapePolygon() {
    if (get_shape()!=entt::null) {
        RenderingServer::get_singleton()->occluder_resource_prepare(get_shape(), RS::OCCLUDER_TYPE_MESH);
    }

    clear();

    Vector<Vector2> points = {
        Vector2(1, -1),
        Vector2(1, 1),
        Vector2(-1, 1),
        Vector2(-1, -1)
    };

    set_polygon_points(eastl::move(points)); // default shape
}
