/*************************************************************************/
/*  navigation_mesh_generator.cpp                                        */
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

#include "navigation_mesh_generator.h"

#include "core/math/quick_hull.h"
#include "core/method_bind.h"
#include "core/deque.h"
#include "core/string.h"
#include "core/os/thread.h"
#include "core/translation_helpers.h"
#include "editor/editor_settings.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/box_shape_3d.h"
#include "scene/resources/capsule_shape_3d.h"
#include "scene/resources/concave_polygon_shape_3d.h"
#include "scene/resources/convex_polygon_shape_3d.h"
#include "scene/resources/cylinder_shape_3d.h"
#include "scene/resources/plane_shape.h"
#include "scene/resources/primitive_meshes.h"
#include "scene/resources/shape.h"
#include "scene/resources/sphere_shape_3d.h"
#ifdef MODULE_CSG_ENABLED
#include "modules/csg/csg_shape.h"
#endif

#ifdef MODULE_GRIDMAP_ENABLED
#include "modules/gridmap/grid_map.h"
#endif

#include <Recast.h>

IMPL_GDCLASS(EditorNavigationMeshGenerator)

EditorNavigationMeshGenerator *EditorNavigationMeshGenerator::singleton = nullptr;

namespace {
void _add_vertex(const Vector3 &p_vec3, Vector<float> &p_verticies) {
    p_verticies.push_back(p_vec3.x);
    p_verticies.push_back(p_vec3.y);
    p_verticies.push_back(p_vec3.z);
}
}

void EditorNavigationMeshGenerator::_add_mesh(const Ref<Mesh> &p_mesh, const Transform &p_xform, Vector<float> &p_verticies, Vector<int> &p_indices) {

    for (int i = 0; i < p_mesh->get_surface_count(); i++) {
        int current_vertex_count = p_verticies.size() / 3;

        if (p_mesh->surface_get_primitive_type(i) != Mesh::PRIMITIVE_TRIANGLES)
            continue;

        int index_count = 0;
        if (p_mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_INDEX) {
            index_count = p_mesh->surface_get_array_index_len(i);
        } else {
            index_count = p_mesh->surface_get_array_len(i);
        }

        ERR_CONTINUE((index_count == 0 || (index_count % 3) != 0));

        int face_count = index_count / 3;

        SurfaceArrays a = p_mesh->surface_get_arrays(i);

        Span<const Vector3> mesh_vertices = a.positions3();

        p_verticies.reserve(p_verticies.size()+mesh_vertices.size());
        if (p_mesh->surface_get_format(i) & Mesh::ARRAY_FORMAT_INDEX) {

            const auto &mesh_indices = a.m_indices;

            for (int j = 0; j < mesh_vertices.size(); j++) {
                _add_vertex(p_xform.xform(mesh_vertices[j]), p_verticies);
            }

            p_indices.reserve(p_indices.size()+face_count);
            for (int j = 0; j < face_count; j++) {
                // CCW
                p_indices.push_back(current_vertex_count + (mesh_indices[j * 3 + 0]));
                p_indices.push_back(current_vertex_count + (mesh_indices[j * 3 + 2]));
                p_indices.push_back(current_vertex_count + (mesh_indices[j * 3 + 1]));
            }
        } else {
            face_count = mesh_vertices.size() / 3;

            p_indices.reserve(p_indices.size()+face_count);
            for (int j = 0; j < face_count; j++) {
                _add_vertex(p_xform.xform(mesh_vertices[j * 3 + 0]), p_verticies);
                _add_vertex(p_xform.xform(mesh_vertices[j * 3 + 2]), p_verticies);
                _add_vertex(p_xform.xform(mesh_vertices[j * 3 + 1]), p_verticies);

                p_indices.push_back(current_vertex_count + (j * 3 + 0));
                p_indices.push_back(current_vertex_count + (j * 3 + 1));
                p_indices.push_back(current_vertex_count + (j * 3 + 2));
            }
        }
    }
}

void EditorNavigationMeshGenerator::_add_faces(const PoolVector3Array &p_faces, const Transform &p_xform, Vector<float> &p_verticies, Vector<int> &p_indices) {
    int face_count = p_faces.size() / 3;
    int current_vertex_count = p_verticies.size() / 3;

    for (int j = 0; j < face_count; j++) {
        _add_vertex(p_xform.xform(p_faces[j * 3 + 0]), p_verticies);
        _add_vertex(p_xform.xform(p_faces[j * 3 + 1]), p_verticies);
        _add_vertex(p_xform.xform(p_faces[j * 3 + 2]), p_verticies);

        p_indices.push_back(current_vertex_count + (j * 3 + 0));
        p_indices.push_back(current_vertex_count + (j * 3 + 2));
        p_indices.push_back(current_vertex_count + (j * 3 + 1));
    }
}

void EditorNavigationMeshGenerator::_parse_geometry(Transform p_accumulated_transform, Node *p_node, Vector<float> &p_verticies, Vector<int> &p_indices, int p_generate_from, uint32_t p_collision_mask, bool p_recurse_children) {

    if (object_cast<MeshInstance3D>(p_node) && p_generate_from != NavigationMesh::PARSED_GEOMETRY_STATIC_COLLIDERS) {

        MeshInstance3D *mesh_instance = object_cast<MeshInstance3D>(p_node);
        Ref<Mesh> mesh = mesh_instance->get_mesh();
        if (mesh) {
            _add_mesh(mesh, p_accumulated_transform * mesh_instance->get_transform(), p_verticies, p_indices);
        }
    }

#ifdef MODULE_CSG_ENABLED
    if (object_cast<CSGShape>(p_node) && p_generate_from != NavigationMesh::PARSED_GEOMETRY_STATIC_COLLIDERS) {

        CSGShape *csg_shape = object_cast<CSGShape>(p_node);
        Array meshes = csg_shape->get_meshes();
        if (!meshes.empty()) {
            Ref<Mesh> mesh = refFromRefPtr<Mesh>(meshes[1]);
            if (mesh) {
                _add_mesh(mesh, p_accumulated_transform * csg_shape->get_transform(), p_verticies, p_indices);
            }
        }
    }
#endif

    if (object_cast<StaticBody3D>(p_node) && p_generate_from != NavigationMesh::PARSED_GEOMETRY_MESH_INSTANCES) {
        StaticBody3D *static_body = object_cast<StaticBody3D>(p_node);

        if (static_body->get_collision_layer() & p_collision_mask) {

            for (int i = 0; i < p_node->get_child_count(); ++i) {
                Node *child = p_node->get_child(i);
                if (object_cast<CollisionShape3D>(child)) {
                    CollisionShape3D *col_shape = object_cast<CollisionShape3D>(child);

                    Transform transform = p_accumulated_transform * static_body->get_transform() * col_shape->get_transform();

                    Ref<Mesh> mesh;
                    Ref<Shape> s = col_shape->get_shape();

                    BoxShape3D *box = object_cast<BoxShape3D>(s.get());
                    if (box) {
                        Ref<CubeMesh> cube_mesh(make_ref_counted<CubeMesh>());
                        cube_mesh->set_size(box->get_extents() * 2.0);
                        mesh = cube_mesh;
                    }

                    CapsuleShape3D *capsule = object_cast<CapsuleShape3D>(s.get());
                    if (capsule) {
                        Ref<CapsuleMesh> capsule_mesh(make_ref_counted<CapsuleMesh>());
                        capsule_mesh->set_radius(capsule->get_radius());
                        capsule_mesh->set_mid_height(capsule->get_height() / 2.0f);
                        mesh = capsule_mesh;
                    }

                    CylinderShape3D *cylinder = object_cast<CylinderShape3D>(s.get());
                    if (cylinder) {
                        Ref<CylinderMesh> cylinder_mesh(make_ref_counted<CylinderMesh>());
                        cylinder_mesh->set_height(cylinder->get_height());
                        cylinder_mesh->set_bottom_radius(cylinder->get_radius());
                        cylinder_mesh->set_top_radius(cylinder->get_radius());
                        mesh = cylinder_mesh;
                    }

                    SphereShape3D *sphere = object_cast<SphereShape3D>(s.get());
                    if (sphere) {
                        Ref<SphereMesh> sphere_mesh(make_ref_counted<SphereMesh>());
                        sphere_mesh->set_radius(sphere->get_radius());
                        sphere_mesh->set_height(sphere->get_radius() * 2.0f);
                        mesh = sphere_mesh;
                    }

                    ConcavePolygonShape3D *concave_polygon = object_cast<ConcavePolygonShape3D>(s.get());
                    if (concave_polygon) {
                        _add_faces(concave_polygon->get_faces(), transform, p_verticies, p_indices);
                    }

                    ConvexPolygonShape3D *convex_polygon = object_cast<ConvexPolygonShape3D>(s.get());
                    if (convex_polygon) {
                        const Vector<Vector3> &varr(convex_polygon->get_points());
                        Geometry::MeshData md;

                        Error err = QuickHull::build(varr, md);

                        if (err == OK) {
                            PoolVector3Array faces;

                            for (int j = 0; j < md.faces.size(); ++j) {
                                Geometry::MeshData::Face face = md.faces[j];

                                for (int k = 2; k < face.indices.size(); ++k) {
                                    faces.push_back(md.vertices[face.indices[0]]);
                                    faces.push_back(md.vertices[face.indices[k - 1]]);
                                    faces.push_back(md.vertices[face.indices[k]]);
                                }
                            }

                            _add_faces(faces, transform, p_verticies, p_indices);
                        }
                    }

                    if (mesh) {
                        _add_mesh(mesh, transform, p_verticies, p_indices);
                    }
                }
            }
        }
    }

#ifdef MODULE_GRIDMAP_ENABLED
    if (object_cast<GridMap>(p_node) && p_generate_from != NavigationMesh::PARSED_GEOMETRY_STATIC_COLLIDERS) {
        GridMap *gridmap_instance = object_cast<GridMap>(p_node);
        Array meshes = gridmap_instance->get_meshes();
        Transform xform = gridmap_instance->get_transform();
        for (int i = 0; i < meshes.size(); i += 2) {
            Ref<Mesh> mesh = refFromRefPtr<Mesh>(meshes[i + 1]);
            if (mesh) {
                _add_mesh(mesh, p_accumulated_transform * xform * meshes[i], p_verticies, p_indices);
            }
        }
    }
#endif
    if (object_cast<Node3D>(p_node)) {

        Node3D *spatial = object_cast<Node3D>(p_node);
        p_accumulated_transform = p_accumulated_transform * spatial->get_transform();
    }

    if (p_recurse_children) {
        for (int i = 0; i < p_node->get_child_count(); i++) {
            _parse_geometry(p_accumulated_transform, p_node->get_child(i), p_verticies, p_indices, p_generate_from,
                    p_collision_mask, p_recurse_children);
        }
    }
}

void EditorNavigationMeshGenerator::_convert_detail_mesh_to_native_navigation_mesh(const rcPolyMeshDetail *p_detail_mesh, Ref<NavigationMesh> p_nav_mesh) {

    Vector<Vector3> nav_vertices;

    for (int i = 0; i < p_detail_mesh->nverts; i++) {
        const float *v = &p_detail_mesh->verts[i * 3];
        nav_vertices.emplace_back(v[0], v[1], v[2]);
    }
    p_nav_mesh->set_vertices(eastl::move(nav_vertices));

    for (int i = 0; i < p_detail_mesh->nmeshes; i++) {
        const unsigned int *m = &p_detail_mesh->meshes[i * 4];
        const unsigned int bverts = m[0];
        const unsigned int btris = m[2];
        const unsigned int ntris = m[3];
        const unsigned char *tris = &p_detail_mesh->tris[btris * 4];
        for (unsigned int j = 0; j < ntris; j++) {
            // Polygon order in recast is opposite than godot's
            Vector<int> nav_indices {
                (int)(bverts + tris[j * 4 + 0]),
                (int)(bverts + tris[j * 4 + 2]),
                (int)(bverts + tris[j * 4 + 1]),
            };
            p_nav_mesh->add_polygon(eastl::move(nav_indices));
        }
    }
}

void EditorNavigationMeshGenerator::_build_recast_navigation_mesh(Ref<NavigationMesh> p_nav_mesh, EditorProgress *ep,
        rcHeightfield *hf, rcCompactHeightfield *chf, rcContourSet *cset, rcPolyMesh *poly_mesh, rcPolyMeshDetail *detail_mesh,
        Vector<float> &vertices, Vector<int> &indices) {
    rcContext ctx;
    ep->step(TTR("Setting up Configuration..."), 1);

    const float *verts = vertices.data();
    const int nverts = vertices.size() / 3;
    const int *tris = indices.data();
    const int ntris = indices.size() / 3;

    float bmin[3], bmax[3];
    rcCalcBounds(verts, nverts, bmin, bmax);

    rcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.cs = p_nav_mesh->get_cell_size();
    cfg.ch = p_nav_mesh->get_cell_height();
    cfg.walkableSlopeAngle = p_nav_mesh->get_agent_max_slope();
    cfg.walkableHeight = (int)Math::ceil(p_nav_mesh->get_agent_height() / cfg.ch);
    cfg.walkableClimb = (int)Math::floor(p_nav_mesh->get_agent_max_climb() / cfg.ch);
    cfg.walkableRadius = (int)Math::ceil(p_nav_mesh->get_agent_radius() / cfg.cs);
    cfg.maxEdgeLen = (int)(p_nav_mesh->get_edge_max_length() / p_nav_mesh->get_cell_size());
    cfg.maxSimplificationError = p_nav_mesh->get_edge_max_error();
    cfg.minRegionArea = (int)(p_nav_mesh->get_region_min_size() * p_nav_mesh->get_region_min_size());
    cfg.mergeRegionArea = (int)(p_nav_mesh->get_region_merge_size() * p_nav_mesh->get_region_merge_size());
    cfg.maxVertsPerPoly = (int)p_nav_mesh->get_verts_per_poly();
    cfg.detailSampleDist = p_nav_mesh->get_detail_sample_distance() < 0.9f ? 0 : p_nav_mesh->get_cell_size() * p_nav_mesh->get_detail_sample_distance();
    cfg.detailSampleMaxError = p_nav_mesh->get_cell_height() * p_nav_mesh->get_detail_sample_max_error();

    cfg.bmin[0] = bmin[0];
    cfg.bmin[1] = bmin[1];
    cfg.bmin[2] = bmin[2];
    cfg.bmax[0] = bmax[0];
    cfg.bmax[1] = bmax[1];
    cfg.bmax[2] = bmax[2];

    ep->step(TTR("Calculating grid size..."), 2);
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    ep->step(TTR("Creating heightfield..."), 3);
    hf = rcAllocHeightfield();

    ERR_FAIL_COND(!hf);
    auto hf_res=rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch);
    ERR_FAIL_COND(!hf_res);

    ep->step(TTR("Marking walkable triangles..."), 4);
    {
        ERR_FAIL_COND(ntris> 1<<20); // sanity check

        auto tri_areas = eastl::make_unique<uint8_t[]>(ntris);

        memset(tri_areas.get(), 0, ntris * sizeof(unsigned char));
        rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, tri_areas.get());
        auto rasterize_res=rcRasterizeTriangles(&ctx, verts, nverts, tris, tri_areas.get(), ntris, *hf, cfg.walkableClimb);
        ERR_FAIL_COND(!rasterize_res);
    }

    if (p_nav_mesh->get_filter_low_hanging_obstacles())
        rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
    if (p_nav_mesh->get_filter_ledge_spans())
        rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
    if (p_nav_mesh->get_filter_walkable_low_height_spans())
        rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);

    ep->step(TTR("Constructing compact heightfield..."), 5);

    chf = rcAllocCompactHeightfield();

    ERR_FAIL_COND(!chf);
    ERR_FAIL_COND(!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf));

    rcFreeHeightField(hf);
    hf = nullptr;

    bool step_res=false;
    ep->step(TTR("Eroding walkable area..."), 6);
    step_res = rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf);
    ERR_FAIL_COND(!step_res);

    ep->step(TTR("Partitioning..."), 7);
    if (p_nav_mesh->get_sample_partition_type() == NavigationMesh::SAMPLE_PARTITION_WATERSHED) {
        step_res = rcBuildDistanceField(&ctx, *chf);
        ERR_FAIL_COND(!step_res);
        step_res = rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea);
        ERR_FAIL_COND(!step_res);
    } else if (p_nav_mesh->get_sample_partition_type() == NavigationMesh::SAMPLE_PARTITION_MONOTONE) {
        step_res = rcBuildRegionsMonotone(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea);
        ERR_FAIL_COND(!step_res);
    } else {
        step_res = rcBuildLayerRegions(&ctx, *chf, 0, cfg.minRegionArea);
        ERR_FAIL_COND(!step_res);
    }

    ep->step(TTR("Creating contours..."), 8);

    cset = rcAllocContourSet();

    ERR_FAIL_COND(!cset);
    step_res = rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset);
    ERR_FAIL_COND(!step_res);

    ep->step(TTR("Creating polymesh..."), 9);

    poly_mesh = rcAllocPolyMesh();
    ERR_FAIL_COND(!poly_mesh);
    step_res = rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *poly_mesh);
    ERR_FAIL_COND(!step_res);

    detail_mesh = rcAllocPolyMeshDetail();
    ERR_FAIL_COND(!detail_mesh);
    step_res = rcBuildPolyMeshDetail(&ctx, *poly_mesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *detail_mesh);
    ERR_FAIL_COND(!step_res);

    rcFreeCompactHeightfield(chf);
    chf = nullptr;
    rcFreeContourSet(cset);
    cset = nullptr;

    ep->step(TTR("Converting to native navigation mesh..."), 10);

    _convert_detail_mesh_to_native_navigation_mesh(detail_mesh, p_nav_mesh);

    rcFreePolyMesh(poly_mesh);
    poly_mesh = nullptr;
    rcFreePolyMeshDetail(detail_mesh);
    detail_mesh = nullptr;
}

EditorNavigationMeshGenerator *EditorNavigationMeshGenerator::get_singleton() {
    return singleton;
}

EditorNavigationMeshGenerator::EditorNavigationMeshGenerator() {
    singleton = this;
}

EditorNavigationMeshGenerator::~EditorNavigationMeshGenerator() {
}

void EditorNavigationMeshGenerator::bake(Ref<NavigationMesh> p_nav_mesh, Node *p_node) {

    ERR_FAIL_COND(not p_nav_mesh);

    EditorProgress ep(("bake"), TTR("Navigation Mesh Generator Setup:"), 11);
    ep.step(TTR("Parsing Geometry..."), 0);

    Vector<float> vertices;
    Vector<int> indices;
    Dequeue<Node *> parse_nodes;
    if (p_nav_mesh->get_source_geometry_mode() == NavigationMesh::SOURCE_GEOMETRY_NAVMESH_CHILDREN) {
        parse_nodes.push_back(p_node);
    } else {
        p_node->get_tree()->get_nodes_in_group(p_nav_mesh->get_source_group_name(), &parse_nodes);
    }

    Transform navmesh_xform = object_cast<Node3D>(p_node)->get_transform().affine_inverse();
    for (Node * E : parse_nodes) {
        int geometry_type = p_nav_mesh->get_parsed_geometry_type();
        uint32_t collision_mask = p_nav_mesh->get_collision_mask();
        bool recurse_children = p_nav_mesh->get_source_geometry_mode() != NavigationMesh::SOURCE_GEOMETRY_GROUPS_EXPLICIT;
        _parse_geometry(navmesh_xform, E, vertices, indices, geometry_type, collision_mask, recurse_children);
    }

    if (!vertices.empty() && !indices.empty()) {

        rcHeightfield *hf = nullptr;
        rcCompactHeightfield *chf = nullptr;
        rcContourSet *cset = nullptr;
        rcPolyMesh *poly_mesh = nullptr;
        rcPolyMeshDetail *detail_mesh = nullptr;

        _build_recast_navigation_mesh(p_nav_mesh, &ep, hf, chf, cset, poly_mesh, detail_mesh, vertices, indices);

        rcFreeHeightField(hf);
        hf = nullptr;

        rcFreeCompactHeightfield(chf);
        chf = nullptr;

        rcFreeContourSet(cset);
        cset = nullptr;

        rcFreePolyMesh(poly_mesh);
        poly_mesh = nullptr;

        rcFreePolyMeshDetail(detail_mesh);
        detail_mesh = nullptr;
    }
    ep.step(TTR("Done!"), 11);
}

void EditorNavigationMeshGenerator::clear(Ref<NavigationMesh> p_nav_mesh) {
    if (p_nav_mesh) {
        p_nav_mesh->clear_polygons();
        p_nav_mesh->set_vertices({});
    }
}

void EditorNavigationMeshGenerator::_bind_methods() {
    MethodBinder::bind_method(D_METHOD("bake", {"nav_mesh", "root_node"}), &EditorNavigationMeshGenerator::bake);
    MethodBinder::bind_method(D_METHOD("clear", {"nav_mesh"}), &EditorNavigationMeshGenerator::clear);
}
