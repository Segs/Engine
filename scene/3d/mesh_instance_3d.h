/*************************************************************************/
/*  mesh_instance_3d.h                                                   */
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
#include "core/node_path.h"

class Skin;
class SkinReference;

class GODOT_EXPORT MeshInstance3D : public GeometryInstance {

    GDCLASS(MeshInstance3D,GeometryInstance)

protected:
    Ref<Mesh> mesh;
    Ref<Skin> skin;
    Ref<Skin> skin_internal;
    Ref<SkinReference> skin_ref;
    NodePath skeleton_path;

    struct SoftwareSkinning {
        enum Flags {
            // Data flags.
            FLAG_TRANSFORM_NORMALS = 1 << 0,

            // Runtime flags.
            FLAG_BONES_READY = 1 << 1,
        };

        struct SurfaceData {
            PoolByteArray source_buffer;
            PoolByteArray buffer;
            PoolByteArray::Write buffer_write;
            uint32_t source_format;
            bool transform_tangents;
            bool ensure_correct_normals;
        };

        Ref<Mesh> mesh_instance;
        Vector<SurfaceData> surface_data;
    };

    SoftwareSkinning *software_skinning;
    uint32_t software_skinning_flags;

    struct BlendShapeTrack {

        int idx;
        float value;
        BlendShapeTrack() {
            idx = 0;
            value = 0;
        }
    };

    HashMap<StringName, BlendShapeTrack> blend_shape_tracks;
    Vector<Ref<Material> > materials;

    void _mesh_changed();
    void _resolve_skeleton_path();

    bool _is_software_skinning_enabled() const;
    static bool _is_global_software_skinning_enabled();

    void _initialize_skinning(bool p_force_reset = false, bool p_call_attach_skeleton=true);
    void _update_skinning();

private:
    // merging
    void _merge_into_mesh_data(const MeshInstance3D &p_mi, const Transform &p_dest_tr_inv, int p_surface_id,
            Vector<Vector3> &r_verts,
            Vector<Vector3> &r_norms, Vector<real_t> &r_tangents, Vector<Color> &r_colors, Vector<Vector2> &r_uvs,
            Vector<Vector2> &r_uv2s, Vector<int> &r_inds);
    bool _is_mergeable_with(const MeshInstance3D &p_other) const;
    bool _ensure_indices_valid(Vector<int> &r_indices, Span<const Vector3> p_verts) const;
    bool _check_for_valid_indices(Span<const int> p_inds, Span<const Vector3> p_verts, Vector<int32_t> *r_inds=nullptr) const;
    bool _merge_meshes(Span<MeshInstance3D *> p_list, bool p_use_global_space, bool p_check_compatibility);
protected:
    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_ret) const;
    void _get_property_list(Vector<PropertyInfo> *p_list) const;

    void _notification(int p_what);
    static void _bind_methods();

public:
    void set_mesh(const Ref<Mesh> &p_mesh);
    Ref<Mesh> get_mesh() const;

    void set_skin(const Ref<Skin> &p_skin);
    Ref<Skin> get_skin() const;

    void set_skeleton_path(const NodePath &p_skeleton);
    NodePath get_skeleton_path();

    int get_surface_material_count() const;
    void set_surface_material(int p_surface, const Ref<Material> &p_material);
    Ref<Material> get_surface_material(int p_surface) const;
    Ref<Material> get_active_material(int p_surface) const;

    void set_material_override(const Ref<Material> &p_material) override;
    void set_material_overlay(const Ref<Material> &p_material) override;

    void set_software_skinning_transform_normals(bool p_enabled);
    bool is_software_skinning_transform_normals_enabled() const;

    Node *create_trimesh_collision_node();
    void create_trimesh_collision();

    Node *create_multiple_convex_collisions_node();
    void create_multiple_convex_collisions();

    Node *create_convex_collision_node(bool p_clean = true, bool p_simplify = false);
    void create_convex_collision(bool p_clean = true, bool p_simplify = false);

    void create_debug_tangents();
    // merging
    bool is_mergeable_with(const MeshInstance3D &p_other) const;
    bool create_by_merging(Vector<MeshInstance3D *> p_list);
    bool merge_meshes(Vector<MeshInstance3D *> p_list, bool p_use_global_space, bool p_check_compatibility) {
        return _merge_meshes(p_list, p_use_global_space, p_check_compatibility);
    }

    AABB get_aabb() const override;
    Vector<Face3> get_faces(uint32_t p_usage_flags) const override;

    MeshInstance3D();
    ~MeshInstance3D() override;
};
