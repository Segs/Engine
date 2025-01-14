/*************************************************************************/
/*  immediate_geometry_3d.cpp                                               */
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

#include "immediate_geometry_3d.h"
#include "core/method_bind.h"
#include "scene/resources/texture.h"
#include "servers/rendering_server.h"

IMPL_GDCLASS(ImmediateGeometry3D)
//TODO: SEGS: copied from mesh.cpp
VARIANT_ENUM_CAST(Mesh::PrimitiveType);

void ImmediateGeometry3D::begin(Mesh::PrimitiveType p_primitive, const Ref<Texture> &p_texture) {

    RenderingServer::get_singleton()->immediate_begin(im, (RS::PrimitiveType)p_primitive, p_texture ? p_texture->get_rid() : entt::null);
    if (p_texture)
        cached_textures.emplace_back(p_texture);
}

void ImmediateGeometry3D::set_normal(const Vector3 &p_normal) {

    RenderingServer::get_singleton()->immediate_normal(im, p_normal);
}

void ImmediateGeometry3D::set_tangent(const Plane &p_tangent) {

    RenderingServer::get_singleton()->immediate_tangent(im, p_tangent);
}

void ImmediateGeometry3D::set_color(const Color &p_color) {

    RenderingServer::get_singleton()->immediate_color(im, p_color);
}

void ImmediateGeometry3D::set_uv(const Vector2 &p_uv) {

    RenderingServer::get_singleton()->immediate_uv(im, p_uv);
}

void ImmediateGeometry3D::set_uv2(const Vector2 &p_uv2) {

    RenderingServer::get_singleton()->immediate_uv2(im, p_uv2);
}

void ImmediateGeometry3D::add_vertex(const Vector3 &p_vertex) {

    RenderingServer::get_singleton()->immediate_vertex(im, p_vertex);
    if (empty) {
        aabb.position = p_vertex;
        aabb.size = Vector3();
        empty = false;
    } else {
        aabb.expand_to(p_vertex);
    }
}

void ImmediateGeometry3D::end() {

    RenderingServer::get_singleton()->immediate_end(im);
}

void ImmediateGeometry3D::clear() {

    RenderingServer::get_singleton()->immediate_clear(im);
    empty = true;
    cached_textures.clear();
}

AABB ImmediateGeometry3D::get_aabb() const {

    return aabb;
}
Vector<Face3> ImmediateGeometry3D::get_faces(uint32_t p_usage_flags) const {

    return {};
}

void ImmediateGeometry3D::add_sphere(int p_lats, int p_lons, float p_radius, bool p_add_uv) {

    for (int i = 1; i <= p_lats; i++) {
        double lat0 = Math_PI * (-0.5 + (double)(i - 1) / p_lats);
        double z0 = Math::sin(lat0);
        double zr0 = Math::cos(lat0);

        double lat1 = Math_PI * (-0.5 + (double)i / p_lats);
        double z1 = Math::sin(lat1);
        double zr1 = Math::cos(lat1);

        for (int j = p_lons; j >= 1; j--) {

            double lng0 = 2 * Math_PI * (double)(j - 1) / p_lons;
            double x0 = Math::cos(lng0);
            double y0 = Math::sin(lng0);

            double lng1 = 2 * Math_PI * (double)(j) / p_lons;
            double x1 = Math::cos(lng1);
            double y1 = Math::sin(lng1);

            Vector3 v[4] = {
                Vector3(x1 * zr0, z0, y1 * zr0),
                Vector3(x1 * zr1, z1, y1 * zr1),
                Vector3(x0 * zr1, z1, y0 * zr1),
                Vector3(x0 * zr0, z0, y0 * zr0)
            };

#define ADD_POINT(m_idx)                                                                                    \
    if (p_add_uv) {                                                                                         \
        set_uv(Vector2(Math::atan2(v[m_idx].x, v[m_idx].z) / Math_PI * 0.5f + 0.5f, v[m_idx].y * 0.5f + 0.5f)); \
        set_tangent(Plane(Vector3(-v[m_idx].z, v[m_idx].y, v[m_idx].x), 1));                                \
    }                                                                                                       \
    set_normal(v[m_idx]);                                                                                   \
    add_vertex(v[m_idx] * p_radius)

            ADD_POINT(0);
            ADD_POINT(1);
            ADD_POINT(2);

            ADD_POINT(2);
            ADD_POINT(3);
            ADD_POINT(0);
        }
    }
}

void ImmediateGeometry3D::_bind_methods() {

    MethodBinder::bind_method(D_METHOD("begin", {"primitive", "texture"}), &ImmediateGeometry3D::begin, {DEFVAL(Ref<Texture>())});
    SE_BIND_METHOD(ImmediateGeometry3D,set_normal);
    SE_BIND_METHOD(ImmediateGeometry3D,set_tangent);
    SE_BIND_METHOD(ImmediateGeometry3D,set_color);
    SE_BIND_METHOD(ImmediateGeometry3D,set_uv);
    SE_BIND_METHOD(ImmediateGeometry3D,set_uv2);
    SE_BIND_METHOD(ImmediateGeometry3D,add_vertex);
    MethodBinder::bind_method(D_METHOD("add_sphere", {"lats", "lons", "radius", "add_uv"}), &ImmediateGeometry3D::add_sphere, {DEFVAL(true)});
    SE_BIND_METHOD(ImmediateGeometry3D,end);
    SE_BIND_METHOD(ImmediateGeometry3D,clear);
}

ImmediateGeometry3D::ImmediateGeometry3D() {

    im = RenderingServer::get_singleton()->immediate_create();
    set_base(im);
    empty = true;
}

ImmediateGeometry3D::~ImmediateGeometry3D() {

    RenderingServer::get_singleton()->free_rid(im);
}
