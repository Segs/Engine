/*************************************************************************/
/*  immediate_geometry_3d.h                                              */
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

#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"

class GODOT_EXPORT ImmediateGeometry3D : public GeometryInstance {

    GDCLASS(ImmediateGeometry3D,GeometryInstance)

    RenderingEntity im;
    //a list of textures drawn need to be kept, to avoid references
    // in RenderingServer from becoming invalid if the texture is no longer used
    Vector<Ref<Texture> > cached_textures;
    bool empty;
    AABB aabb;

protected:
    static void _bind_methods();

public:
    void begin(Mesh::PrimitiveType p_primitive, const Ref<Texture> &p_texture = Ref<Texture>());
    void set_normal(const Vector3 &p_normal);
    void set_tangent(const Plane &p_tangent);
    void set_color(const Color &p_color);
    void set_uv(const Vector2 &p_uv);
    void set_uv2(const Vector2 &p_uv2);

    void add_vertex(const Vector3 &p_vertex);

    void end();
    void clear();

    void add_sphere(int p_lats, int p_lons, float p_radius, bool p_add_uv = true);

    AABB get_aabb() const override;
    Vector<Face3> get_faces(uint32_t p_usage_flags) const override;

    ImmediateGeometry3D();
    ~ImmediateGeometry3D() override;
};
